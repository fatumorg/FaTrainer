const express = require("express");
const { permissionsFor } = require("../permissions.js");
const c = require("../compose.js");
const { moderate } = require("../moderation.js");
const { stampAge } = require("../abuse.js");

function composeRoutes({ pool, render, auth, config }) {
  const r = express.Router();

  function checkHuman(req) {
    if (req.body.website) return new c.ForumError(422, "Your post could not be sent. Reload the page and try again.");
    const min = config.minComposeMs ?? 3000;
    const age = stampAge(config.secret, req.body.started);
    if (min > 0 && age < min) return new c.ForumError(422, "Take a moment before posting: wait a few seconds after opening the form.");
    return null;
  }

  const signIn = (returnTo) => (req, res, next) => {
    if (req.session?.user) return next();
    res.redirect(req.method === "GET" ? 302 : 303, `/forum/auth/login?return_to=${encodeURIComponent(returnTo(req))}`);
  };

  async function postableCategories(userId) {
    const { rows } = await pool.query("SELECT id, slug, name, post_permission FROM categories ORDER BY position, id");
    const out = [];
    for (const cat of rows) {
      const perms = await permissionsFor(pool, userId, cat.id);
      if (perms.has("thread.create") && perms.has(cat.post_permission)) out.push(cat);
    }
    return out;
  }

  const allTags = async () => (await pool.query("SELECT slug, name FROM tags ORDER BY name")).rows;
  const asList = (v) => (Array.isArray(v) ? v : v ? [v] : []);

  function fail(req, res, err) {
    if (!(err instanceof c.ForumError)) throw err;
    const titles = { 403: "Not allowed", 404: "Page not found" };
    render(req, res, "error", { title: titles[err.status] || "Something went wrong", status: err.status, message: err.message, noindex: true }, err.status);
  }

  async function showNew(req, res, { values = {}, error = null, preview = null } = {}, status = 200) {
    render(req, res, "compose", {
      title: "Start a thread",
      heading: "Start a thread",
      mode: "new",
      action: "/forum/new",
      submit: "Post thread",
      categories: await postableCategories(req.session.user.id),
      tags: await allTags(),
      values: { tags: [], ...values },
      error,
      preview,
      noindex: true,
      scripts: ["/forum/static/js/editor.js"],
    }, status);
  }

  r.get("/new", signIn((req) => req.originalUrl), async (req, res) => {
    await showNew(req, res, { values: { category: req.query.category || req.query.c || "" } });
  });

  r.post("/new", signIn((req) => `/forum/new?category=${encodeURIComponent(req.body?.category || "")}`), auth.csrf.verify, async (req, res) => {
    const values = { category: String(req.body.category || ""), title: String(req.body.title || ""), body: String(req.body.body || ""), tags: asList(req.body.tags) };
    if (req.body.preview) return showNew(req, res, { values, preview: (await c.renderPost(pool, values.body.slice(0, c.MAX_BODY))).html });
    const bot = checkHuman(req);
    if (bot) return showNew(req, res, { values, error: bot }, bot.status);
    try {
      const cat = (await pool.query("SELECT id FROM categories WHERE slug = $1", [values.category])).rows[0];
      const t = await c.createThread(pool, req.session.user, { categoryId: cat?.id, title: values.title, body: values.body, tags: values.tags });
      res.redirect(303, `/forum/t/${t.id}/${t.slug}?posted=1#p${t.postId}`);
    } catch (err) {
      if (!(err instanceof c.ForumError)) throw err;
      await showNew(req, res, { values, error: err }, err.status);
    }
  });

  async function showReply(req, res, threadId, { body = "", error = null, preview = null }, status = 200) {
    const t = (await pool.query("SELECT id, slug, title FROM threads WHERE id = $1 AND deleted_at IS NULL", [threadId])).rows[0];
    if (!t) return fail(req, res, new c.ForumError(404, "This thread does not exist."));
    render(req, res, "compose", {
      title: `Reply: ${t.title}`,
      heading: `Reply to "${t.title}"`,
      thread: t,
      mode: "reply",
      action: `/forum/t/${t.id}/reply`,
      submit: "Post reply",
      values: { body },
      error,
      preview,
      noindex: true,
      scripts: ["/forum/static/js/editor.js"],
    }, status);
  }

  r.post("/t/:id/reply", signIn((req) => `/forum/t/${Number(req.params.id) || 0}`), auth.csrf.verify, async (req, res) => {
    const id = Number(req.params.id) || 0;
    const body = String(req.body.body || "");
    if (req.body.preview) return showReply(req, res, id, { body, preview: (await c.renderPost(pool, body.slice(0, c.MAX_BODY))).html });
    const bot = checkHuman(req);
    if (bot) return showReply(req, res, id, { body, error: bot }, bot.status);
    try {
      const p = await c.createPost(pool, req.session.user, id, body);
      res.redirect(303, `/forum/t/${p.threadId}/${p.slug}?${p.page > 1 ? `page=${p.page}&` : ""}posted=1#p${p.id}`);
    } catch (err) {
      if (!(err instanceof c.ForumError)) throw err;
      if (err.status === 404) return fail(req, res, err);
      await showReply(req, res, id, { body, error: err }, err.status);
    }
  });

  async function showEdit(req, res, { post, mine, perms }, { values, error = null, preview = null } = {}, status = 200) {
    const meta = post.first && ((post.thread_author === req.session.user.id && perms.has("post.edit.own")) || perms.has("post.edit.any"));
    const current = meta ? (await pool.query("SELECT g.slug FROM thread_tags tt JOIN tags g ON g.id = tt.tag_id WHERE tt.thread_id = $1", [post.thread_id])).rows.map((x) => x.slug) : [];
    render(req, res, "compose", {
      title: "Edit post",
      heading: "Edit post",
      thread: { id: post.thread_id, slug: post.slug, title: post.title },
      mode: "edit",
      meta,
      needsReason: !mine,
      action: `/forum/p/${post.id}/edit`,
      submit: "Save changes",
      tags: meta ? await allTags() : [],
      values: values || { title: post.title, body: post.body_md, tags: current },
      error,
      preview,
      noindex: true,
      scripts: ["/forum/static/js/editor.js"],
    }, status);
  }

  r.get("/p/:id/edit", signIn((req) => req.originalUrl), async (req, res) => {
    try {
      await showEdit(req, res, await c.editablePost(pool, req.session.user, req.params.id));
    } catch (err) {
      fail(req, res, err);
    }
  });

  r.post("/p/:id/edit", signIn((req) => `/forum/p/${Number(req.params.id) || 0}/edit`), auth.csrf.verify, async (req, res) => {
    let ctx;
    try {
      ctx = await c.editablePost(pool, req.session.user, req.params.id);
    } catch (err) {
      return fail(req, res, err);
    }
    const values = { title: String(req.body.title ?? ctx.post.title), body: String(req.body.body || ""), tags: asList(req.body.tags), reason: String(req.body.reason || "") };
    if (req.body.preview) return showEdit(req, res, ctx, { values, preview: (await c.renderPost(pool, values.body.slice(0, c.MAX_BODY))).html });
    try {
      c.checkBody(values.body);
      if (!ctx.mine && !values.reason.trim()) throw new c.ForumError(422, "Give a reason for editing someone else's post.", "reason");
      let slug = ctx.post.slug;
      const meta = ctx.post.first && ((ctx.post.thread_author === req.session.user.id && ctx.perms.has("post.edit.own")) || ctx.perms.has("post.edit.any"));
      if (meta) slug = (await c.editThreadMeta(pool, req.session.user, ctx.post.thread_id, { title: values.title, tags: values.tags })).slug;
      await c.editPost(pool, req.session.user, ctx.post.id, values.body, values.reason);
      res.redirect(303, `/forum/t/${ctx.post.thread_id}/${slug}#p${ctx.post.id}`);
    } catch (err) {
      if (!(err instanceof c.ForumError)) throw err;
      if (!err.field) return fail(req, res, err);
      await showEdit(req, res, ctx, { values, error: err }, err.status);
    }
  });

  r.post("/p/:id/delete", signIn((req) => "/forum"), auth.csrf.verify, async (req, res) => {
    try {
      const post = (await pool.query("SELECT p.author_id, p.thread_id, t.slug FROM posts p JOIN threads t ON t.id = p.thread_id WHERE p.id = $1", [Number(req.params.id) || 0])).rows[0];
      if (post && post.author_id !== req.session.user.id && req.perms.has("post.delete.any")) {
        await moderate(pool, req.session.user, { targetType: "post", targetId: Number(req.params.id), action: "remove", rule: req.body.rule, reason: req.body.reason });
        return res.redirect(303, `/forum/t/${post.thread_id}/${post.slug}#p${Number(req.params.id)}`);
      }
      const d = await c.deletePost(pool, req.session.user, req.params.id, req.body.reason);
      res.redirect(303, `/forum/t/${d.threadId}/${d.slug}#p${d.id}`);
    } catch (err) {
      fail(req, res, err);
    }
  });

  r.post("/preview", (req, res, next) => (req.session?.user ? next() : res.status(401).json({ error: "AUTH_REQUIRED" })), auth.csrf.verify, async (req, res) => {
    const body = String(req.body?.body || "").slice(0, c.MAX_BODY);
    res.set("cache-control", "no-store").json({ html: (await c.renderPost(pool, body)).html });
  });

  return r;
}

module.exports = { composeRoutes };
