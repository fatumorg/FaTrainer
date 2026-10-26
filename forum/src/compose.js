const { renderMarkdown, RENDERER_VERSION } = require("./markdown.js");
const { permissionsFor } = require("./permissions.js");
const { logAction } = require("./audit.js");
const { attachUploads, checkImageCount, MAX_PER_POST } = require("./uploads.js");
const { notifyOnPost } = require("./notifications.js");
const { checkPostAllowed } = require("./abuse.js");

const MAX_BODY = 20000;
const MAX_TAGS = 5;

class ForumError extends Error {
  constructor(status, message, field = null) {
    super(message);
    this.status = status;
    this.field = field;
  }
}

function slugify(title) {
  const slug = title.normalize("NFKD").replace(/[̀-ͯ]/g, "").replace(/ł/gi, "l").toLowerCase().replace(/[^a-z0-9]+/g, "-").replace(/^-+|-+$/g, "").slice(0, 80).replace(/-+$/, "");
  return slug || "thread";
}

function checkTitle(title) {
  const t = String(title ?? "").trim().replace(/\s+/g, " ");
  if (t.length < 5) throw new ForumError(422, "Title needs at least 5 characters.", "title");
  if (t.length > 140) throw new ForumError(422, `Title is too long (${t.length}/140 characters).`, "title");
  return t;
}

function checkBody(body) {
  const b = String(body ?? "").replace(/\r\n?/g, "\n");
  if (!b.trim()) throw new ForumError(422, "Write something before posting.", "body");
  if (b.length > MAX_BODY) throw new ForumError(422, `Post is too long (${b.length}/${MAX_BODY} characters).`, "body");
  if (!checkImageCount(b)) throw new ForumError(422, `Up to ${MAX_PER_POST} images per post.`, "body");
  return b;
}

async function renderPost(db, md) {
  const names = [...new Set([...md.matchAll(/@([A-Za-z0-9_]{3,32})/g)].map((m) => m[1].toLowerCase()))];
  const found = new Map();
  if (names.length) {
    const { rows } = await db.query("SELECT username FROM aura.public_profiles WHERE discoverable AND lower(username) = ANY($1)", [names]);
    for (const r of rows) found.set(r.username.toLowerCase(), { username: r.username });
  }
  return renderMarkdown(md, { mentionResolver: (name) => found.get(name.toLowerCase()) || null });
}

async function tx(pool, fn) {
  const client = await pool.connect();
  try {
    await client.query("BEGIN");
    const out = await fn(client);
    await client.query("COMMIT");
    return out;
  } catch (err) {
    await client.query("ROLLBACK");
    throw err;
  } finally {
    client.release();
  }
}

async function tagIds(db, slugs) {
  const list = [...new Set((Array.isArray(slugs) ? slugs : slugs ? [slugs] : []).map(String))];
  if (list.length > MAX_TAGS) throw new ForumError(422, `Choose up to ${MAX_TAGS} tags.`, "tags");
  if (!list.length) return [];
  const { rows } = await db.query("SELECT id FROM tags WHERE slug = ANY($1)", [list]);
  if (rows.length !== list.length) throw new ForumError(422, "Choose tags from the list.", "tags");
  return rows.map((r) => r.id);
}

async function joinForum(db, userId) {
  const { rows } = await db.query("INSERT INTO forum_prefs (user_id) VALUES ($1) ON CONFLICT (user_id) DO UPDATE SET user_id = EXCLUDED.user_id RETURNING auto_watch", [userId]);
  return rows[0].auto_watch;
}

async function watch(db, userId, threadId) {
  await db.query("INSERT INTO subscriptions (user_id, thread_id) VALUES ($1, $2) ON CONFLICT DO NOTHING", [userId, threadId]);
}

async function allowed(pool, user, body, threadId) {
  const check = await checkPostAllowed(pool, user, { body, threadId });
  if (!check.ok) throw new ForumError(check.reason === "suspended" ? 403 : 429, check.message);
  return check.hold;
}

async function createThread(pool, user, { categoryId, title, body, tags }) {
  const cleanTitle = checkTitle(title);
  const md = checkBody(body);
  const { rows } = await pool.query("SELECT id, post_permission FROM categories WHERE id = $1", [Number(categoryId) || 0]);
  const category = rows[0];
  if (!category) throw new ForumError(422, "Choose a category.", "category");
  const perms = await permissionsFor(pool, user.id, category.id);
  if (!perms.has("thread.create") || !perms.has(category.post_permission)) throw new ForumError(403, "You cannot start threads in this category.", "category");
  const ids = await tagIds(pool, tags);
  const held = await allowed(pool, user, md, null);
  const rendered = await renderPost(pool, md);
  const slug = slugify(cleanTitle);
  return tx(pool, async (db) => {
    const t = await db.query("INSERT INTO threads (category_id, author_id, title, slug) VALUES ($1, $2, $3, $4) RETURNING id", [category.id, user.id, cleanTitle, slug]);
    const id = t.rows[0].id;
    const p = await db.query("INSERT INTO posts (thread_id, author_id, body_md, body_html, renderer_version, held) VALUES ($1, $2, $3, $4, $5, $6) RETURNING id", [id, user.id, md, rendered.html, RENDERER_VERSION, held]);
    for (const tag of ids) await db.query("INSERT INTO thread_tags (thread_id, tag_id) VALUES ($1, $2)", [id, tag]);
    await attachUploads(db, user.id, p.rows[0].id, md);
    if (await joinForum(db, user.id)) await watch(db, user.id, id);
    if (!held) await notifyOnPost(db, { id: p.rows[0].id, threadId: id, authorId: user.id, mentions: rendered.mentions, isReply: false });
    return { id, slug, postId: p.rows[0].id, mentions: rendered.mentions, held };
  });
}

async function createPost(pool, user, threadId, body) {
  const { rows } = await pool.query(
    "SELECT t.id, t.slug, t.locked, t.category_id, c.post_permission FROM threads t JOIN categories c ON c.id = t.category_id WHERE t.id = $1 AND t.deleted_at IS NULL",
    [Number(threadId) || 0],
  );
  const thread = rows[0];
  if (!thread) throw new ForumError(404, "This thread does not exist.");
  const perms = await permissionsFor(pool, user.id, thread.category_id);
  if (thread.locked && !perms.has("thread.lock")) throw new ForumError(403, "This thread is locked. New replies are closed.");
  if (!perms.has("post.create") || !perms.has(thread.post_permission)) throw new ForumError(403, "You cannot reply in this category.");
  const md = checkBody(body);
  const held = await allowed(pool, user, md, thread.id);
  const rendered = await renderPost(pool, md);
  return tx(pool, async (db) => {
    const p = await db.query("INSERT INTO posts (thread_id, author_id, body_md, body_html, renderer_version, held) VALUES ($1, $2, $3, $4, $5, $6) RETURNING id", [thread.id, user.id, md, rendered.html, RENDERER_VERSION, held]);
    if (!held) await db.query("UPDATE threads SET last_post_at = now(), reply_count = reply_count + 1 WHERE id = $1", [thread.id]);
    await attachUploads(db, user.id, p.rows[0].id, md);
    if (await joinForum(db, user.id)) await watch(db, user.id, thread.id);
    if (!held) await notifyOnPost(db, { id: p.rows[0].id, threadId: thread.id, authorId: user.id, mentions: rendered.mentions });
    const position = (await db.query("SELECT count(*)::int AS n FROM posts WHERE thread_id = $1 AND (NOT held OR id = $2) AND id <= $2", [thread.id, p.rows[0].id])).rows[0].n;
    return { id: p.rows[0].id, threadId: thread.id, slug: thread.slug, page: Math.ceil(position / 20), mentions: rendered.mentions, held };
  });
}

async function loadPost(pool, postId) {
  const { rows } = await pool.query(
    `SELECT po.id, po.author_id, po.body_md, po.thread_id, t.slug, t.title, t.category_id, t.author_id AS thread_author,
            po.id = (SELECT min(id) FROM posts WHERE thread_id = t.id) AS first
     FROM posts po JOIN threads t ON t.id = po.thread_id
     WHERE po.id = $1 AND po.deleted_at IS NULL AND t.deleted_at IS NULL`,
    [Number(postId) || 0],
  );
  if (!rows[0]) throw new ForumError(404, "This post does not exist.");
  return rows[0];
}

async function mayChange(pool, user, post, own, any) {
  const perms = await permissionsFor(pool, user.id, post.category_id);
  const mine = post.author_id === user.id;
  if (mine && perms.has(own)) return { mine, perms };
  if (perms.has(any)) return { mine, perms };
  throw new ForumError(403, "You do not have permission to change this post.");
}

async function editablePost(pool, user, postId) {
  const post = await loadPost(pool, postId);
  const { mine, perms } = await mayChange(pool, user, post, "post.edit.own", "post.edit.any");
  return { post, mine, perms };
}

async function editPost(pool, user, postId, body, reason = null) {
  const { post, mine } = await editablePost(pool, user, postId);
  const md = checkBody(body);
  if (!mine && !String(reason ?? "").trim()) throw new ForumError(422, "Give a reason for editing someone else's post.", "reason");
  const rendered = await renderPost(pool, md);
  await tx(pool, async (db) => {
    await db.query("INSERT INTO post_revisions (post_id, body_md, edited_by) VALUES ($1, $2, $3)", [post.id, post.body_md, user.id]);
    await db.query("UPDATE posts SET body_md = $2, body_html = $3, renderer_version = $4, edited_at = now() WHERE id = $1", [post.id, md, rendered.html, RENDERER_VERSION]);
    await attachUploads(db, user.id, post.id, md);
    if (!mine) await logAction(db, { actorId: user.id, action: "post.edit", targetType: "post", targetId: post.id, reason: reason.trim(), meta: { before: post.body_md.slice(0, 200), after: md.slice(0, 200) } });
  });
  return { id: post.id, threadId: post.thread_id, slug: post.slug, mentions: rendered.mentions };
}

async function deletePost(pool, user, postId, reason = null) {
  const post = await loadPost(pool, postId);
  const { mine } = await mayChange(pool, user, post, "post.delete.own", "post.delete.any");
  if (!mine && !String(reason ?? "").trim()) throw new ForumError(422, "Give a reason for removing someone else's post.", "reason");
  await tx(pool, async (db) => {
    await db.query("UPDATE posts SET deleted_at = now(), deleted_by = $2 WHERE id = $1", [post.id, user.id]);
    if (!mine) await logAction(db, { actorId: user.id, action: "post.delete", targetType: "post", targetId: post.id, reason: reason.trim(), meta: { before: post.body_md.slice(0, 200) } });
  });
  return { id: post.id, threadId: post.thread_id, slug: post.slug };
}

async function editThreadMeta(pool, user, threadId, { title, tags }) {
  const { rows } = await pool.query("SELECT id, author_id, category_id, title FROM threads WHERE id = $1 AND deleted_at IS NULL", [Number(threadId) || 0]);
  const thread = rows[0];
  if (!thread) throw new ForumError(404, "This thread does not exist.");
  const perms = await permissionsFor(pool, user.id, thread.category_id);
  const mine = thread.author_id === user.id;
  if (!(mine && perms.has("post.edit.own")) && !perms.has("post.edit.any")) throw new ForumError(403, "You do not have permission to change this thread.");
  const cleanTitle = checkTitle(title);
  const ids = await tagIds(pool, tags);
  const slug = slugify(cleanTitle);
  await tx(pool, async (db) => {
    await db.query("UPDATE threads SET title = $2, slug = $3 WHERE id = $1", [thread.id, cleanTitle, slug]);
    await db.query("DELETE FROM thread_tags WHERE thread_id = $1", [thread.id]);
    for (const tag of ids) await db.query("INSERT INTO thread_tags (thread_id, tag_id) VALUES ($1, $2)", [thread.id, tag]);
    if (!mine && cleanTitle !== thread.title) await logAction(db, { actorId: user.id, action: "thread.edit", targetType: "thread", targetId: thread.id, meta: { before: thread.title, after: cleanTitle } });
  });
  return { id: thread.id, slug };
}

module.exports = { createThread, createPost, editPost, deletePost, editThreadMeta, editablePost, renderPost, slugify, checkTitle, checkBody, tx, ForumError, MAX_BODY, MAX_TAGS };
