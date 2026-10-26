const { escapeHtml } = require("./markdown.js");
const { ForumError } = require("./compose.js");

const PER_PAGE = 20;
const snippet = (raw) => escapeHtml(raw).replaceAll("\u0001", "<b>").replaceAll("\u0002", "</b>");

async function searchForum(db, { q, categoryId, tag, author, from, to, page = 1 }) {
  const text = String(q ?? "").trim();
  if (text.length < 2 || text.length > 200) throw new ForumError(422, "Search needs 2 to 200 characters.", "q");
  const args = [text];
  const arg = (v) => (args.push(v), `$${args.length}`);
  const where = [
    "p.deleted_at IS NULL", "NOT p.held", "t.deleted_at IS NULL",
    "(p.search @@ query OR (t.search @@ query AND p.id = (SELECT min(id) FROM posts WHERE thread_id = t.id)))",
  ];
  if (categoryId) where.push(`t.category_id = ${arg(Number(categoryId))}`);
  if (tag) where.push(`EXISTS (SELECT 1 FROM thread_tags tt JOIN tags g ON g.id = tt.tag_id WHERE tt.thread_id = t.id AND g.slug = ${arg(String(tag))})`);
  if (author) where.push(`p.author_id = (SELECT id FROM aura.public_profiles WHERE lower(username) = lower(${arg(String(author))}))`);
  if (from) where.push(`p.created_at >= ${arg(from)}::date`);
  if (to) where.push(`p.created_at < ${arg(to)}::date + 1`);
  const base = `FROM posts p JOIN threads t ON t.id = p.thread_id, websearch_to_tsquery('fatrainer.fts', $1) query WHERE ${where.join(" AND ")}`;
  const total = (await db.query(`SELECT count(*)::int AS n ${base}`, args)).rows[0].n;
  const pages = Math.max(1, Math.ceil(total / PER_PAGE));
  const current = Math.min(Math.max(1, Number(page) || 1), pages);
  const { rows } = await db.query(
    `SELECT t.id AS thread_id, t.slug, t.title, p.id AS post_id, p.created_at,
            ts_headline('fatrainer.fts', translate(p.body_md, chr(1) || chr(2), ''), query, 'StartSel=' || chr(1) || ', StopSel=' || chr(2) || ', MaxWords=30, MinWords=10, MaxFragments=2, FragmentDelimiter=" … "') AS headline
     ${base}
     ORDER BY ts_rank(p.search, query) + ts_rank(t.search, query) DESC, p.created_at DESC
     LIMIT ${PER_PAGE} OFFSET ${(current - 1) * PER_PAGE}`,
    args,
  );
  return {
    items: rows.map((r) => ({ threadId: r.thread_id, postId: r.post_id, slug: r.slug, title: r.title, snippetHtml: snippet(r.headline), at: r.created_at })),
    page: current,
    pages,
    total,
  };
}

module.exports = { searchForum };
