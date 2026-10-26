const express = require("express");
const a = require("../admin.js");
const { ForumError } = require("../compose.js");

function adminRoutes({ pool, render, auth }) {
  const r = express.Router();
  const mustSignIn = (req, res, next) => {
    if (req.session?.user) return next();
    res.redirect(req.method === "GET" ? 302 : 303, `/forum/auth/login?return_to=${encodeURIComponent(req.method === "GET" ? req.originalUrl : "/forum/admin")}`);
  };
  const isAdmin = (req) => ["category.manage", "tag.manage", "role.assign"].some((p) => req.perms.has(p));
  const act = (fn) => [mustSignIn, auth.csrf.verify, async (req, res) => {
    try {
      await fn(req);
      res.redirect(303, "/forum/admin?saved=1");
    } catch (err) {
      if (!(err instanceof ForumError)) throw err;
      await show(req, res, err, err.status);
    }
  }];

  async function show(req, res, error = null, status = 200) {
    if (!isAdmin(req)) return render(req, res, "error", { title: "Not allowed", status: 403, noindex: true }, 403);
    render(req, res, "admin/index", { title: "Admin", ...(await a.overview(pool)), error, saved: req.query.saved === "1", noindex: true }, status);
  }

  r.get("/admin", mustSignIn, (req, res) => show(req, res));

  r.post("/admin/categories", ...act((req) => a.saveCategory(pool, req.session.user, {
    id: req.body.id || null, slug: req.body.slug, name: req.body.name, description: req.body.description,
    position: req.body.position, parentId: req.body.parent || null, postPermission: req.body.post_permission,
  })));
  r.post("/admin/categories/:id/delete", ...act((req) => a.deleteCategory(pool, req.session.user, req.params.id)));
  r.post("/admin/tags", ...act((req) => a.saveTag(pool, req.session.user, { id: req.body.id || null, slug: req.body.slug, name: req.body.name })));
  r.post("/admin/tags/:id/delete", ...act((req) => a.deleteTag(pool, req.session.user, req.params.id)));
  r.post("/admin/roles/:name/permissions", ...act((req) => a.setRolePermissions(pool, req.session.user, req.params.name, req.body.permissions)));
  r.post("/admin/roles/assign", ...act((req) => a.assignRole(pool, req.session.user, { nickname: req.body.nickname, role: req.body.role, categoryId: req.body.category || null })));
  r.post("/admin/roles/remove", ...act((req) => a.removeRole(pool, req.session.user, { userId: req.body.user, role: req.body.role, categoryId: req.body.category || null })));

  return r;
}

module.exports = { adminRoutes };
