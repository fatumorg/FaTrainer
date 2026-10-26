const path = require("node:path");
const { Eta } = require("eta");
const { composeStamp } = require("./abuse.js");

const eta = new Eta({ views: path.join(__dirname, "..", "views"), cache: process.env.NODE_ENV === "production" });

function createRenderer({ config, auth }) {
  return function render(req, res, view, data = {}, status = 200) {
    const user = req.session?.user || null;
    const html = eta.render(view, {
      user,
      theme: ["light", "dark"].includes(user?.theme) ? user.theme : "system",
      site: { appUrl: config.appUrl, issuer: config.issuer },
      csrf: user ? auth.csrf.token(req) : "",
      stamp: user ? composeStamp(config.secret) : "",
      path: req.originalUrl,
      loginHref: "/forum/auth/login?return_to=" + encodeURIComponent(req.originalUrl),
      accountHref: auth.accountSettingsUrl(config.appUrl + "/forum"),
      perms: req.perms || new Set(),
      unread: req.unread || 0,
      noindex: false,
      day: (d) => (d ? new Date(d).toISOString().slice(0, 10) : ""),
      iso: (d) => (d ? new Date(d).toISOString() : ""),
      description: "The FaTrainer | Dying Light forum: questions, ideas, configs and announcements.",
      ...data,
    });
    res.status(status).type("html").set("cache-control", user ? "private, no-store" : "no-cache").send(html);
  };
}

module.exports = { createRenderer };
