const { test } = require("node:test");
const assert = require("node:assert/strict");
const { startForum } = require("./server.js");
const { categoryId } = require("./helpers.js");
const a = require("../src/admin.js");

async function setup(t) {
  const h = await startForum(t);
  const as = (name) => ({ id: h.users[name], username: name });
  return { h, as };
}

test("admin cannot remove the last administrator", async (t) => {
  const { h, as } = await setup(t);
  await assert.rejects(a.removeRole(h.pool, as("owner"), { userId: h.users.owner, role: "administrator" }), /last administrator/);
  await a.assignRole(h.pool, as("owner"), { nickname: "MOD", role: "administrator" });
  await a.removeRole(h.pool, as("owner"), { userId: h.users.owner, role: "administrator" });
  await assert.rejects(a.assignRole(h.pool, as("owner"), { nickname: "gracz", role: "moderator" }), /permission/);
  await assert.rejects(a.setRolePermissions(h.pool, as("mod"), "administrator", ["post.create"]), /role\.assign/);
  await a.setRolePermissions(h.pool, as("mod"), "moderator", ["post.create", "report.view", "report.resolve"]);
  const perms = (await h.pool.query("SELECT permission FROM role_permissions rp JOIN roles r ON r.id = rp.role_id WHERE r.name = 'moderator' ORDER BY 1")).rows.map((r) => r.permission);
  assert.deepEqual(perms, ["post.create", "report.resolve", "report.view"]);
  await assert.rejects(a.setRolePermissions(h.pool, as("mod"), "moderator", ["made.up"]), /Unknown permission/);
  const actions = (await h.pool.query("SELECT action FROM mod_log WHERE actor_id IS NOT NULL ORDER BY id")).rows.map((r) => r.action);
  assert.deepEqual(actions, ["role.assign", "role.remove", "role.permissions"]);
});

test("subcategory of a subcategory refused", async (t) => {
  const { h, as } = await setup(t);
  const general = await categoryId(h.pool, "general");
  const sub = await a.saveCategory(h.pool, as("owner"), { slug: "configs", name: "Configs", description: "Shared configs", position: 1, parentId: general, postPermission: "post.create" });
  await assert.rejects(a.saveCategory(h.pool, as("owner"), { slug: "deeper", name: "Deeper", parentId: sub.id }), /one level/);
  await assert.rejects(a.saveCategory(h.pool, as("owner"), { slug: "Bad Slug", name: "Bad" }), /slug/);
  await assert.rejects(a.saveCategory(h.pool, as("owner"), { id: general, slug: "general", name: "General", parentId: sub.id }), /one level/);
  await a.saveCategory(h.pool, as("owner"), { id: sub.id, slug: "configs", name: "Config files", parentId: general });
  assert.equal((await h.pool.query("SELECT name FROM categories WHERE id = $1", [sub.id])).rows[0].name, "Config files");
  await assert.rejects(a.deleteCategory(h.pool, as("owner"), general), /not empty/);
  await a.deleteCategory(h.pool, as("owner"), sub.id);
  await assert.rejects(a.saveCategory(h.pool, as("gracz"), { slug: "x-y", name: "X" }), /permission/);
  const tag = await a.saveTag(h.pool, as("owner"), { slug: "crash", name: "Crash" });
  await a.saveTag(h.pool, as("owner"), { id: tag.id, slug: "crash", name: "Crashes" });
  await a.deleteTag(h.pool, as("owner"), tag.id);
  const { client, csrf } = await h.signedIn("owner");
  const page = await client.get("/forum/admin");
  assert.equal(page.status, 200);
  assert.match(page.text, /Categories/);
  const res = await client.post("/forum/admin/categories", { _csrf: csrf, slug: "guides", name: "Guides", description: "", position: "6", parent: "", post_permission: "post.create" });
  assert.equal(res.status, 303);
  assert.equal((await h.anon().get("/forum/c/guides")).status, 200);
  assert.equal((await (await h.signedIn("gracz")).client.get("/forum/admin")).status, 403);
});
