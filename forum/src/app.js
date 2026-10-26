const path = require("node:path");
const express = require("express");
const { securityHeaders } = require("fatum-kit/security");
const { createRenderer } = require("./views.js");
const { permissionsFor, promoteOwner } = require("./permissions.js");
const { browseRoutes } = require("./routes/browse.js");

function createApp({ config, pool, auth }) {
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
    next();
  });

  forum.use(browseRoutes({ pool, render }));

  forum.use("/api", (req, res) => res.status(404).json({ error: "NOT_FOUND" }));
  forum.use((req, res) => render(req, res, "error", { title: "Page not found", status: 404, noindex: true }, 404));
  forum.use((err, req, res, next) => {
    console.error("request failed:", err?.message);
    if (res.headersSent) return next(err);
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
