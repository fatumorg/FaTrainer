const express = require("express");
const e = require("../engagement.js");
const { ForumError } = require("../compose.js");

function engagementRoutes({ pool, render, auth }) {
  const r = express.Router();
  const wantsJson = (req) => (req.get("accept") || "").includes("application/json");
  const mustSignIn = (req, res, next) => {
    if (req.session?.user) return next();
    if (req.method === "GET") return res.redirect(302, `/forum/auth/login?return_to=${encodeURIComponent(req.originalUrl)}`);
    res.status(401).json({ error: "AUTH_REQUIRED" });
  };
  const write = [mustSignIn, auth.csrf.verify];

  const respond = (fn) => async (req, res) => {
    try {
      const { data, location } = await fn(req);
      if (wantsJson(req)) return res.json(data);
      res.redirect(303, location);
    } catch (err) {
      if (!(err instanceof ForumError)) throw err;
      if (wantsJson(req)) return res.status(err.status).json({ error: "REFUSED", message: err.message });
      render(req, res, "error", { title: err.status === 404 ? "Page not found" : "Not allowed", status: err.status, message: err.message, noindex: true }, err.status);
    }
  };

  const threadUrl = async (threadId, postId) => {
    const t = (await pool.query("SELECT id, slug FROM threads WHERE id = $1", [threadId])).rows[0];
    return t ? `/forum/t/${t.id}/${t.slug}${postId ? `#p${postId}` : ""}` : "/forum";
  };

  r.post("/p/:id/helpful", write, respond(async (req) => {
    const data = await e.toggleHelpful(pool, req.session.user.id, req.params.id);
    const { rows } = await pool.query("SELECT thread_id FROM posts WHERE id = $1", [req.params.id]);
    return { data, location: await threadUrl(rows[0].thread_id, Number(req.params.id)) };
  }));

  r.post("/t/:id/accept", write, respond(async (req) => {
    const data = await e.acceptAnswer(pool, req.session.user, req.params.id, req.body.post);
    req.accepted = data;
    return { data: { solvedPostId: data.solvedPostId }, location: await threadUrl(Number(req.params.id), Number(req.body.post)) };
  }));

  r.post("/t/:id/bookmark", write, respond(async (req) => ({
    data: await e.toggleBookmark(pool, req.session.user.id, Number(req.params.id) || 0),
    location: await threadUrl(Number(req.params.id) || 0),
  })));

  r.post("/t/:id/watch", write, respond(async (req) => {
    const id = Number(req.params.id) || 0;
    if (!(await pool.query("SELECT 1 FROM threads WHERE id = $1 AND deleted_at IS NULL", [id])).rowCount) throw new ForumError(404, "This thread does not exist.");
    const muted = req.body.muted === "1";
    await e.setSubscription(pool, req.session.user.id, { threadId: id, muted });
    return { data: { watching: !muted }, location: await threadUrl(id) };
  }));

  r.post("/c/:slug/watch", write, respond(async (req) => {
    const cat = (await pool.query("SELECT id, slug FROM categories WHERE slug = $1", [req.params.slug])).rows[0];
    if (!cat) throw new ForumError(404, "This category does not exist.");
    const muted = req.body.muted === "1";
    await e.setSubscription(pool, req.session.user.id, { categoryId: cat.id, muted });
    return { data: { watching: !muted }, location: `/forum/c/${cat.slug}` };
  }));

  r.get("/saved", mustSignIn, async (req, res) => {
    const list = await e.listBookmarks(pool, req.session.user.id, req.query.page);
    render(req, res, "saved", { title: "Saved", list, href: (n) => (n > 1 ? `/forum/saved?page=${n}` : "/forum/saved"), noindex: true });
  });

  r.get("/watching", mustSignIn, async (req, res) => {
    render(req, res, "watching", { title: "Watching", ...(await e.listWatching(pool, req.session.user.id)), noindex: true });
  });

  return r;
}

module.exports = { engagementRoutes };
