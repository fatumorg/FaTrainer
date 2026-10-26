const path = require("node:path");
const { Eta } = require("eta");

const eta = new Eta({ views: path.join(__dirname, "..", "views"), cache: process.env.NODE_ENV === "production" });

function createRenderer({ config, auth }) {
  return function render(req, res, view, data = {}, status = 200) {
    const user = req.session?.user || null;
    const html = eta.render(view, {
      user,
      theme: ["light", "dark"].includes(user?.theme) ? user.theme : "system",
      site: { appUrl: config.appUrl, issuer: config.issuer },
      csrf: user ? auth.csrf.token(req) : "",
      path: req.originalUrl,
      loginHref: "/forum/auth/login?return_to=" + encodeURIComponent(req.originalUrl),
      accountHref: auth.accountSettingsUrl(config.appUrl + "/forum"),
      perms: req.perms || new Set(),
      unread: req.unread || 0,
      noindex: false,
      description: "The FaTrainer | Dying Light forum: questions, ideas, configs and announcements.",
      ...data,
    });
    res.status(status).type("html").set("cache-control", user ? "private, no-store" : "no-cache").send(html);
  };
}

module.exports = { createRenderer };
