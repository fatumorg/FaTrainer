const express = require("express");
const n = require("../notifications.js");

const LABELS = { replies: "replies", mentions: "mentions", all: "replies and mentions" };

function notificationRoutes({ pool, render, auth, config }) {
  const r = express.Router();
  const mustSignIn = (req, res, next) => {
    if (req.session?.user) return next();
    res.redirect(req.method === "GET" ? 302 : 303, `/forum/auth/login?return_to=${encodeURIComponent(req.method === "GET" ? req.originalUrl : "/forum")}`);
  };

  r.get("/notifications", mustSignIn, async (req, res) => {
    const list = await n.listNotifications(pool, req.session.user.id, req.query.page);
    render(req, res, "notifications", { title: "Notifications", list, href: (p) => (p > 1 ? `/forum/notifications?page=${p}` : "/forum/notifications"), noindex: true });
  });

  r.post("/notifications/read", mustSignIn, auth.csrf.verify, async (req, res) => {
    await pool.query("UPDATE notifications SET read_at = now() WHERE user_id = $1 AND read_at IS NULL", [req.session.user.id]);
    res.redirect(303, "/forum/notifications");
  });

  r.get("/notifications/:id", mustSignIn, async (req, res, next) => {
    const to = await n.openNotification(pool, req.session.user.id, req.params.id);
    if (!to) return next();
    res.redirect(303, to);
  });

  const unsubscribePage = (apply) => async (req, res, next) => {
    const t = n.readToken(config.secret, req.params.token);
    if (!t) return next();
    if (apply) await n.unsubscribe(pool, t);
    render(req, res, "unsubscribe", { title: "E-mail settings", done: apply, what: LABELS[t.kind], token: req.params.token, noindex: true });
  };
  r.get("/unsubscribe/:token", unsubscribePage(false));
  r.post("/unsubscribe/:token", unsubscribePage(true));

  r.get("/settings", mustSignIn, async (req, res) => {
    const { rows } = await pool.query("SELECT email_replies, email_mentions, auto_watch FROM forum_prefs WHERE user_id = $1", [req.session.user.id]);
    render(req, res, "settings", { title: "Forum settings", prefs: rows[0] || { email_replies: true, email_mentions: true, auto_watch: true }, saved: req.query.saved === "1", noindex: true });
  });

  r.post("/settings", mustSignIn, auth.csrf.verify, async (req, res) => {
    const on = (k) => req.body[k] === "on";
    await pool.query(
      `INSERT INTO forum_prefs (user_id, email_replies, email_mentions, auto_watch) VALUES ($1, $2, $3, $4)
       ON CONFLICT (user_id) DO UPDATE SET email_replies = $2, email_mentions = $3, auto_watch = $4`,
      [req.session.user.id, on("email_replies"), on("email_mentions"), on("auto_watch")],
    );
    res.redirect(303, "/forum/settings?saved=1");
  });

  return r;
}

module.exports = { notificationRoutes };
