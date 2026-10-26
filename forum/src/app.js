const path = require("node:path");
const express = require("express");
const { securityHeaders, rateLimiter } = require("fatum-kit/security");
const { createRenderer } = require("./views.js");
const { permissionsFor, promoteOwner } = require("./permissions.js");
const { browseRoutes } = require("./routes/browse.js");
const { composeRoutes } = require("./routes/compose.js");
const { mediaRoutes } = require("./routes/media.js");
const { engagementRoutes } = require("./routes/engagement.js");
const { notificationRoutes } = require("./routes/notifications.js");
const { moderationRoutes } = require("./routes/moderation.js");
const { adminRoutes } = require("./routes/admin.js");
const { unreadCount } = require("./notifications.js");

function createApp({ config, pool, auth, redis }) {
  const app = express();
  app.set("trust proxy", 1);
  app.disable("x-powered-by");
  app.use(securityHeaders({ formAction: [config.issuer] }));
  const render = createRenderer({ config, auth });
  const forum = express.Router();

  forum.use("/static", express.static(path.join(__dirname, "..", "public"), { index: false, maxAge: "7d" }));
  forum.use(express.urlencoded({ extended: false, limit: "64kb" }));
  forum.use(express.json({ limit: "64kb" }));
  forum.use(auth.session);
  forum.use("/auth", auth.router);
  forum.use(async (req, res, next) => {
    const user = req.session?.user;
    req.perms = user ? await permissionsFor(pool, user.id) : new Set();
    req.unread = user ? await unreadCount(pool, user.id) : 0;
    next();
  });

  if (redis) forum.post(["/new", "/t/:id/reply", "/p/:id/edit", "/api/uploads", "/report", "/appeal/:id"], rateLimiter({ redis, prefix: "ft:rl:write:", windowMs: 600_000, limit: 60 }));
  forum.use(browseRoutes({ pool, render }));
  forum.use(composeRoutes({ pool, render, auth, config }));
  forum.use(mediaRoutes({ pool, config, auth }));
  forum.use(engagementRoutes({ pool, render, auth }));
  forum.use(notificationRoutes({ pool, render, auth, config }));
  forum.use(moderationRoutes({ pool, render, auth }));
  forum.use(adminRoutes({ pool, render, auth }));

  forum.use("/api", (req, res) => res.status(404).json({ error: "NOT_FOUND" }));
  forum.use((req, res) => render(req, res, "error", { title: "Page not found", status: 404, noindex: true }, 404));
  forum.use((err, req, res, next) => {
    console.error("request failed:", err?.message);
    if (res.headersSent) return next(err);
    if (err?.type === "entity.too.large") return res.status(413).json({ error: "too_large" });
    if (req.path.startsWith("/api/")) return res.status(500).json({ error: "SERVER_ERROR" });
    render(req, res, "error", { title: "Something went wrong", status: 500, noindex: true }, 500);
  });

  app.use("/forum", forum);
  return app;
}

async function boot({ config, pool }) {
  if (config.ownerSub && (await promoteOwner(pool, config.ownerSub))) console.log("forum owner promoted to administrator");
}

module.exports = { createApp, boot };
