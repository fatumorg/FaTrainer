const { permissionsFor } = require("./permissions.js");
const { ForumError } = require("./compose.js");

const PER_PAGE = 25;

async function canWrite(db, userId) {
  if (!(await permissionsFor(db, userId)).has("post.create")) throw new ForumError(403, "You cannot do that right now.");
}

async function toggleHelpful(db, userId, postId) {
  const { rows } = await db.query("SELECT author_id FROM posts WHERE id = $1 AND deleted_at IS NULL AND NOT held", [Number(postId) || 0]);
  if (!rows[0]) throw new ForumError(404, "This post does not exist.");
  if (rows[0].author_id === userId) throw new ForumError(403, "You cannot mark your own post as helpful.");
  await canWrite(db, userId);
  const removed = await db.query("DELETE FROM reactions WHERE post_id = $1 AND user_id = $2", [postId, userId]);
  if (!removed.rowCount) await db.query("INSERT INTO reactions (post_id, user_id) VALUES ($1, $2) ON CONFLICT DO NOTHING", [postId, userId]);
  const count = (await db.query("SELECT count(*)::int AS n FROM reactions WHERE post_id = $1", [postId])).rows[0].n;
  return { helpful: !removed.rowCount, count };
}

async function acceptAnswer(db, user, threadId, postId) {
  const { rows } = await db.query(
    `SELECT t.id, t.author_id, t.category_id, t.solved_post_id, c.slug AS category,
            p.id AS post_id, p.author_id AS post_author, p.id = (SELECT min(id) FROM posts WHERE thread_id = t.id) AS first
     FROM threads t JOIN categories c ON c.id = t.category_id
     LEFT JOIN posts p ON p.id = $2 AND p.thread_id = t.id AND p.deleted_at IS NULL AND NOT p.held
     WHERE t.id = $1 AND t.deleted_at IS NULL`,
    [Number(threadId) || 0, Number(postId) || 0],
  );
  const t = rows[0];
  if (!t || !t.post_id) throw new ForumError(404, "This post does not exist.");
  if (t.category !== "qa") throw new ForumError(422, "Accepted answers exist only in Q&A.");
  if (t.first) throw new ForumError(422, "Choose a reply as the accepted answer.");
  const perms = await permissionsFor(db, user.id, t.category_id);
  if (!((t.author_id === user.id && perms.has("post.edit.own")) || perms.has("post.edit.any"))) throw new ForumError(403, "Only the thread author or a moderator can accept an answer.");
  const solvedPostId = t.solved_post_id === t.post_id ? null : t.post_id;
  await db.query("UPDATE threads SET solved_post_id = $2 WHERE id = $1", [t.id, solvedPostId]);
  return { solvedPostId, authorId: t.post_author };
}

async function toggleBookmark(db, userId, threadId) {
  const { rowCount } = await db.query("SELECT 1 FROM threads WHERE id = $1 AND deleted_at IS NULL", [Number(threadId) || 0]);
  if (!rowCount) throw new ForumError(404, "This thread does not exist.");
  const removed = await db.query("DELETE FROM bookmarks WHERE user_id = $1 AND thread_id = $2", [userId, threadId]);
  if (!removed.rowCount) await db.query("INSERT INTO bookmarks (user_id, thread_id) VALUES ($1, $2) ON CONFLICT DO NOTHING", [userId, threadId]);
  return { saved: !removed.rowCount };
}

async function listBookmarks(db, userId, page = 1) {
  const total = (await db.query("SELECT count(*)::int AS n FROM bookmarks b JOIN threads t ON t.id = b.thread_id WHERE b.user_id = $1 AND t.deleted_at IS NULL", [userId])).rows[0].n;
  const pages = Math.max(1, Math.ceil(total / PER_PAGE));
  const current = Math.min(Math.max(1, Number(page) || 1), pages);
  const { rows } = await db.query(
    `SELECT t.id, t.slug, t.title, t.last_post_at, t.reply_count, b.created_at AS saved_at
     FROM bookmarks b JOIN threads t ON t.id = b.thread_id
     WHERE b.user_id = $1 AND t.deleted_at IS NULL
     ORDER BY b.created_at DESC, t.id DESC LIMIT ${PER_PAGE} OFFSET ${(current - 1) * PER_PAGE}`,
    [userId],
  );
  return { items: rows, page: current, pages, total };
}

async function setSubscription(db, userId, { threadId = null, categoryId = null, muted }) {
  if (!threadId === !categoryId) throw new Error("setSubscription needs a thread or a category");
  await db.query(
    `INSERT INTO subscriptions (user_id, thread_id, category_id, muted) VALUES ($1, $2, $3, $4)
     ON CONFLICT (user_id, thread_id, category_id) DO UPDATE SET muted = EXCLUDED.muted`,
    [userId, threadId, categoryId, Boolean(muted)],
  );
}

async function watchersOf(db, threadId) {
  const { rows } = await db.query(
    `SELECT s.user_id FROM subscriptions s WHERE s.thread_id = $1 AND NOT s.muted
     UNION
     SELECT s.user_id FROM subscriptions s JOIN threads t ON t.category_id = s.category_id
     WHERE t.id = $1 AND NOT s.muted
       AND NOT EXISTS (SELECT 1 FROM subscriptions m WHERE m.user_id = s.user_id AND m.thread_id = $1 AND m.muted)`,
    [threadId],
  );
  return rows.map((r) => r.user_id);
}

async function listWatching(db, userId) {
  const threads = await db.query(
    `SELECT t.id, t.slug, t.title, t.last_post_at FROM subscriptions s JOIN threads t ON t.id = s.thread_id
     WHERE s.user_id = $1 AND NOT s.muted AND t.deleted_at IS NULL ORDER BY t.last_post_at DESC LIMIT 200`,
    [userId],
  );
  const categories = await db.query(
    "SELECT c.id, c.slug, c.name FROM subscriptions s JOIN categories c ON c.id = s.category_id WHERE s.user_id = $1 AND NOT s.muted ORDER BY c.position",
    [userId],
  );
  return { threads: threads.rows, categories: categories.rows };
}

async function viewerState(db, userId, threadId, postIds) {
  if (!userId) return { saved: false, watching: false, helpful: new Set() };
  const saved = (await db.query("SELECT 1 FROM bookmarks WHERE user_id = $1 AND thread_id = $2", [userId, threadId])).rowCount > 0;
  const watching = (await watchersOf(db, threadId)).includes(userId);
  const helpful = new Set((await db.query("SELECT post_id FROM reactions WHERE user_id = $1 AND post_id = ANY($2)", [userId, postIds])).rows.map((r) => r.post_id));
  return { saved, watching, helpful };
}

module.exports = { toggleHelpful, acceptAnswer, toggleBookmark, listBookmarks, setSubscription, watchersOf, listWatching, viewerState };
