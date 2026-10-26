const express = require("express");
const m = require("../moderation.js");
const { ForumError } = require("../compose.js");

function moderationRoutes({ pool, render, auth }) {
  const r = express.Router();
  const mustSignIn = (req, res, next) => {
    if (req.session?.user) return next();
    res.redirect(req.method === "GET" ? 302 : 303, `/forum/auth/login?return_to=${encodeURIComponent(req.method === "GET" ? req.originalUrl : "/forum")}`);
  };
  const write = [mustSignIn, auth.csrf.verify];
  const refuse = (req, res, err) => {
    if (!(err instanceof ForumError)) throw err;
    render(req, res, "error", { title: err.status === 404 ? "Page not found" : err.status === 403 ? "Not allowed" : "Check the form", status: err.status, message: err.message, noindex: true }, err.status);
  };
  const guarded = (fn) => async (req, res) => {
    try {
      await fn(req, res);
    } catch (err) {
      refuse(req, res, err);
    }
  };
  const common = { rules: m.RULES, reasons: m.REASONS, noindex: true };

  async function reportTarget(query) {
    const type = query.post ? "post" : query.thread ? "thread" : null;
    const id = Number(query.post || query.thread) || 0;
    if (!type) return null;
    const sql = type === "post"
      ? "SELECT p.id, p.thread_id, t.slug, t.title, left(p.body_md, 300) AS excerpt FROM posts p JOIN threads t ON t.id = p.thread_id WHERE p.id = $1 AND p.deleted_at IS NULL"
      : "SELECT t.id, t.id AS thread_id, t.slug, t.title, t.title AS excerpt FROM threads t WHERE t.id = $1 AND t.deleted_at IS NULL";
    const row = (await pool.query(sql, [id])).rows[0];
    return row ? { type, ...row } : null;
  }

  r.get("/report", async (req, res) => {
    render(req, res, "report", { ...common, title: "Report content", target: await reportTarget(req.query), values: {}, error: null });
  });

  r.get("/report/sent", (req, res) => render(req, res, "report-sent", { ...common, title: "Report sent" }));

  r.post("/report", write, async (req, res) => {
    const values = { reason: String(req.body.reason || ""), details: String(req.body.details || "") };
    const target = await reportTarget({ [req.body.target_type === "thread" ? "thread" : "post"]: req.body.target_id });
    try {
      if (!target) throw new ForumError(404, "This content does not exist.");
      await m.fileReport(pool, req.session.user, { targetType: target.type, targetId: target.id, reason: values.reason, details: values.details });
      res.redirect(303, "/forum/report/sent");
    } catch (err) {
      if (!(err instanceof ForumError) || !err.field) return refuse(req, res, err);
      render(req, res, "report", { ...common, title: "Report content", target, values, error: err }, err.status);
    }
  });

  r.get("/appeal/:id", mustSignIn, async (req, res, next) => {
    const view = await m.reportView(pool, req.session.user, req.params.id);
    if (!view) return next();
    render(req, res, "appeal", { ...common, title: "Moderation decision", view, error: null, sent: req.query.sent === "1" });
  });

  r.post("/appeal/:id", write, async (req, res, next) => {
    try {
      const out = await m.fileAppeal(pool, req.session.user, req.params.id, req.body.text);
      if (out.error === "not_found") return next();
      if (out.error) throw new ForumError(422, out.error === "window_closed" ? "The six-month appeal window has closed." : "Your appeal is already being reviewed.");
      res.redirect(303, `/forum/appeal/${Number(req.params.id)}?sent=1`);
    } catch (err) {
      if (!(err instanceof ForumError)) throw err;
      const view = await m.reportView(pool, req.session.user, req.params.id);
      if (!view) return next();
      render(req, res, "appeal", { ...common, title: "Moderation decision", view, error: err, sent: false }, err.status);
    }
  });

  r.get("/mod", mustSignIn, guarded(async (req, res) => {
    render(req, res, "mod/queue", { ...common, title: "Moderation", q: await m.queue(pool, req.session.user), actions: m.ACTIONS });
  }));

  r.get("/mod/log", mustSignIn, guarded(async (req, res) => {
    const log = await m.modLog(pool, req.session.user, req.query.page);
    render(req, res, "mod/log", { ...common, title: "Moderation log", log, href: (n) => (n > 1 ? `/forum/mod/log?page=${n}` : "/forum/mod/log") });
  }));

  r.post("/mod/reports/:id/decide", write, guarded(async (req, res) => {
    const out = await m.decide(pool, req.session.user, req.params.id, { action: req.body.action, rule: req.body.rule, reason: req.body.reason, until: req.body.until, body: req.body.body });
    res.redirect(303, out === "ok" ? "/forum/mod" : "/forum/mod?already=1");
  }));

  r.post("/mod/appeals/:id/decide", write, guarded(async (req, res) => {
    await m.decideAppeal(pool, req.session.user, req.params.id, req.body.outcome);
    res.redirect(303, "/forum/mod");
  }));

  r.post("/mod/t/:id/:action", write, guarded(async (req, res) => {
    const t = await m.threadAction(pool, req.session.user, req.params.id, req.params.action, { category: req.body.category, reason: req.body.reason, rule: req.body.rule });
    const row = (await pool.query("SELECT slug, deleted_at FROM threads WHERE id = $1", [t.id])).rows[0];
    res.redirect(303, row.deleted_at ? "/forum/mod" : `/forum/t/${t.id}/${row.slug}`);
  }));

  r.post("/mod/users/:id/suspend", write, guarded(async (req, res) => {
    await m.suspendUser(pool, req.session.user, req.params.id, { until: req.body.until, reason: req.body.reason, rule: req.body.rule });
    res.redirect(303, "/forum/mod");
  }));

  return r;
}

module.exports = { moderationRoutes };
