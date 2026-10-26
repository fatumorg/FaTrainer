const express = require("express");
const { listThreads, getThread, profile, SORTS } = require("../threads.js");

const SORT_LABELS = { activity: "Latest activity", new: "Newest", helpful: "Most helpful", unanswered: "Unanswered" };

function query(params) {
  const q = new URLSearchParams(Object.entries(params).filter(([, v]) => v !== undefined && v !== null && v !== ""));
  const s = q.toString();
  return s ? `?${s}` : "";
}

const pageOf = (n) => (n > 1 ? n : undefined);

function browseRoutes({ pool, render }) {
  const r = express.Router();

  r.get("/", async (req, res) => {
    const { rows } = await pool.query(
      `SELECT c.id, c.slug, c.name, c.description, c.parent_id,
              count(t.id) FILTER (WHERE t.deleted_at IS NULL)::int AS threads,
              max(t.last_post_at) FILTER (WHERE t.deleted_at IS NULL) AS last_post_at
       FROM categories c LEFT JOIN threads t ON t.category_id = c.id
       GROUP BY c.id ORDER BY c.position, c.id`,
    );
    const top = rows.filter((c) => !c.parent_id).map((c) => ({ ...c, children: rows.filter((s) => s.parent_id === c.id) }));
    render(req, res, "index", { title: "Forum", categories: top, canonical: "/forum" });
  });

  r.get("/c/:slug", async (req, res, next) => {
    const { rows } = await pool.query("SELECT id, slug, name, description FROM categories WHERE slug = $1", [req.params.slug]);
    const category = rows[0];
    if (!category) return next();
    const user = req.session?.user;
    const filters = {
      sort: SORTS[req.query.sort] && req.query.sort !== "activity" ? req.query.sort : undefined,
      tag: typeof req.query.tag === "string" && /^[a-z0-9-]{1,40}$/.test(req.query.tag) ? req.query.tag : undefined,
      solved: ["yes", "no"].includes(req.query.solved) ? req.query.solved : undefined,
      mine: req.query.mine === "1" && user ? "1" : undefined,
    };
    const list = await listThreads(pool, {
      categoryId: category.id,
      sort: filters.sort || "activity",
      tag: filters.tag,
      solved: filters.solved === undefined ? undefined : filters.solved === "yes",
      mine: filters.mine ? user.id : undefined,
      page: req.query.page,
    });
    const base = `/forum/c/${category.slug}`;
    const href = (page) => base + query({ ...filters, page: pageOf(page) });
    if (req.query.page !== undefined && String(list.page) !== String(req.query.page)) return res.redirect(301, href(list.page));
    const filtered = Object.values(filters).some(Boolean);
    render(req, res, "category", {
      title: category.name,
      description: category.description || undefined,
      category,
      list,
      filters,
      sorts: SORT_LABELS,
      href,
      filterHref: (change) => base + query({ ...filters, ...change }),
      canonical: base + query({ page: pageOf(list.page) }),
      noindex: filtered,
    });
  });

  r.get("/t/:id{/:slug}", async (req, res, next) => {
    if (!/^\d{1,9}$/.test(req.params.id)) return next();
    const data = await getThread(pool, Number(req.params.id), req.query.page);
    if (!data) return next();
    const base = `/forum/t/${data.thread.id}/${data.thread.slug}`;
    const href = (page) => base + query({ page: pageOf(page) });
    const pageChanged = req.query.page !== undefined && String(data.page) !== String(req.query.page);
    if (req.params.slug !== data.thread.slug || pageChanged) return res.redirect(301, href(data.page));
    render(req, res, "thread", { title: data.thread.title, ...data, href, canonical: href(data.page), scripts: req.session?.user ? ["/forum/static/js/editor.js"] : [] });
  });

  r.get("/u/:nickname", async (req, res, next) => {
    const data = await profile(pool, req.params.nickname, req.query.page, req.session?.user || null);
    if (!data) return next();
    const base = `/forum/u/${encodeURIComponent(data.user.username)}`;
    const href = (page) => base + query({ page: pageOf(page) });
    const pageChanged = req.query.page !== undefined && String(data.page) !== String(req.query.page);
    if (req.params.nickname !== data.user.username || pageChanged) return res.redirect(301, href(data.page));
    render(req, res, "profile", { title: data.user.username, ...data, href, canonical: href(data.page) });
  });

  return r;
}

module.exports = { browseRoutes };
