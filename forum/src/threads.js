const THREADS_PER_PAGE = 25;
const POSTS_PER_PAGE = 20;
const SORTS = {
  activity: "t.last_post_at DESC",
  new: "t.created_at DESC",
  helpful: "helpful DESC, t.last_post_at DESC",
  unanswered: "t.created_at DESC",
};

const pages = (total, per) => Math.max(1, Math.ceil(total / per));
const clamp = (page, last) => Math.min(Math.max(1, Number(page) || 1), last);

const TAGS = `COALESCE((SELECT json_agg(json_build_object('slug', g.slug, 'name', g.name) ORDER BY g.name)
  FROM thread_tags tt JOIN tags g ON g.id = tt.tag_id WHERE tt.thread_id = t.id), '[]') AS tags`;

async function listThreads(db, { categoryId, sort = "activity", tag, solved, mine, page = 1 }) {
  const where = ["t.deleted_at IS NULL", "NOT (SELECT held FROM posts WHERE thread_id = t.id ORDER BY id LIMIT 1)"];
  const args = [];
  const arg = (v) => (args.push(v), `$${args.length}`);
  if (categoryId) where.push(`t.category_id = ${arg(categoryId)}`);
  if (tag) where.push(`EXISTS (SELECT 1 FROM thread_tags tt JOIN tags g ON g.id = tt.tag_id WHERE tt.thread_id = t.id AND g.slug = ${arg(tag)})`);
  if (solved === true) where.push("t.solved_post_id IS NOT NULL");
  if (solved === false) where.push("t.solved_post_id IS NULL");
  if (mine) where.push(`t.author_id = ${arg(mine)}`);
  if (sort === "unanswered") where.push("t.reply_count = 0");
  const order = SORTS[sort] || SORTS.activity;
  const sql = where.join(" AND ");
  const total = (await db.query(`SELECT count(*)::int AS n FROM threads t WHERE ${sql}`, args)).rows[0].n;
  const last = pages(total, THREADS_PER_PAGE);
  const current = clamp(page, last);
  const { rows } = await db.query(
    `SELECT t.id, t.slug, t.title, t.reply_count, t.created_at, t.last_post_at, t.pinned, t.locked,
            t.solved_post_id IS NOT NULL AS solved, p.username AS author, c.slug AS category_slug, ${TAGS},
            (SELECT count(*) FROM reactions r JOIN posts po ON po.id = r.post_id WHERE po.thread_id = t.id AND po.deleted_at IS NULL)::int AS helpful
     FROM threads t
     JOIN categories c ON c.id = t.category_id
     LEFT JOIN aura.public_profiles p ON p.id = t.author_id
     WHERE ${sql}
     ORDER BY t.pinned DESC, ${order}, t.id DESC
     LIMIT ${THREADS_PER_PAGE} OFFSET ${(current - 1) * THREADS_PER_PAGE}`,
    args,
  );
  return { items: rows, page: current, pages: last, total };
}

async function getThread(db, id, page = 1, viewer = {}) {
  const { rows } = await db.query(
    `SELECT t.*, p.username AS author, c.slug AS category_slug, c.name AS category_name, ${TAGS},
            (SELECT min(id) FROM posts WHERE thread_id = t.id) AS first_post_id
     FROM threads t JOIN categories c ON c.id = t.category_id LEFT JOIN aura.public_profiles p ON p.id = t.author_id
     WHERE t.id = $1 AND t.deleted_at IS NULL`,
    [id],
  );
  const thread = rows[0];
  if (!thread) return null;
  const seen = "(NOT po.held OR po.author_id = $2 OR $3)";
  const args = [id, viewer.id || 0, Boolean(viewer.moderator)];
  const first = (await db.query("SELECT held, author_id FROM posts WHERE thread_id = $1 ORDER BY id LIMIT 1", [id])).rows[0];
  if (first?.held && first.author_id !== viewer.id && !viewer.moderator) return null;
  const total = (await db.query(`SELECT count(*)::int AS n FROM posts po WHERE po.thread_id = $1 AND ${seen}`, args)).rows[0].n;
  const last = pages(total, POSTS_PER_PAGE);
  const current = clamp(page, last);
  const posts = await db.query(
    `SELECT po.id, po.author_id, po.created_at, po.edited_at, po.deleted_at, po.held,
            CASE WHEN po.deleted_at IS NULL THEN po.body_html END AS body_html,
            CASE WHEN po.deleted_at IS NULL THEN NULL WHEN po.deleted_by = po.author_id THEN 'author' ELSE 'moderator' END AS deleted,
            p.username AS author, (SELECT count(*) FROM reactions r WHERE r.post_id = po.id)::int AS helpful
     FROM posts po LEFT JOIN aura.public_profiles p ON p.id = po.author_id
     WHERE po.thread_id = $1 AND ${seen}
     ORDER BY po.id
     LIMIT ${POSTS_PER_PAGE} OFFSET ${(current - 1) * POSTS_PER_PAGE}`,
    args,
  );
  return { thread, posts: posts.rows, page: current, pages: last };
}

async function profile(db, nickname, page = 1, viewer = null) {
  const { rows } = await db.query(
    `SELECT p.id, p.username, p.bio, p.avatar_key, p.profile_visibility, f.joined_at,
            (SELECT r.name FROM user_roles ur JOIN roles r ON r.id = ur.role_id WHERE ur.user_id = p.id AND ur.category_id IS NULL
             ORDER BY array_position(ARRAY['administrator', 'moderator'], r.name) LIMIT 1) AS role,
            (SELECT count(*) FROM threads t JOIN posts po ON po.id = t.solved_post_id WHERE po.author_id = p.id AND t.deleted_at IS NULL)::int AS solved
     FROM aura.public_profiles p LEFT JOIN forum_prefs f ON f.user_id = p.id
     WHERE lower(p.username) = lower($1)`,
    [nickname],
  );
  const user = rows[0];
  if (!user) return null;
  if (user.profile_visibility === "members" && !viewer) {
    user.bio = null;
    user.avatar_key = null;
  }
  const visible = "po.author_id = $1 AND po.deleted_at IS NULL AND NOT po.held AND t.deleted_at IS NULL";
  const total = (await db.query(`SELECT count(*)::int AS n FROM posts po JOIN threads t ON t.id = po.thread_id WHERE ${visible}`, [user.id])).rows[0].n;
  const last = pages(total, POSTS_PER_PAGE);
  const current = clamp(page, last);
  const posts = await db.query(
    `SELECT po.id, po.created_at, t.id AS thread_id, t.slug, t.title,
            po.id = (SELECT min(id) FROM posts WHERE thread_id = t.id) AS started
     FROM posts po JOIN threads t ON t.id = po.thread_id
     WHERE ${visible}
     ORDER BY po.id DESC
     LIMIT ${POSTS_PER_PAGE} OFFSET ${(current - 1) * POSTS_PER_PAGE}`,
    [user.id],
  );
  return { user, posts: posts.rows, page: current, pages: last };
}

module.exports = { listThreads, getThread, profile, SORTS, THREADS_PER_PAGE, POSTS_PER_PAGE };
