const { test } = require("node:test");
const assert = require("node:assert/strict");
const { testDb, categoryId, grantRole } = require("./helpers.js");
const { permissionsFor, can, promoteOwner } = require("../src/permissions.js");

test("member can create threads but not pin", async (t) => {
  const { pool, users } = await testDb(t);
  const perms = await permissionsFor(pool, users.gracz, await categoryId(pool, "general"));
  assert.equal(can(perms, "thread.create"), true);
  assert.equal(can(perms, "post.create"), true);
  assert.equal(can(perms, "thread.pin"), false);
  assert.equal(can(perms, "announcement.create"), false);
  assert.equal((await permissionsFor(pool, null)).size, 0);
});

test("category moderator can lock only in their category", async (t) => {
  const { pool, users } = await testDb(t);
  const qa = await categoryId(pool, "qa");
  const general = await categoryId(pool, "general");
  await grantRole(pool, users.mod, "moderator", qa);
  assert.equal(can(await permissionsFor(pool, users.mod, qa), "thread.lock"), true);
  assert.equal(can(await permissionsFor(pool, users.mod, general), "thread.lock"), false);
  assert.equal(can(await permissionsFor(pool, users.mod), "thread.lock"), false);
});

test("suspended user loses every write permission", async (t) => {
  const { pool, users } = await testDb(t);
  await grantRole(pool, users.mod, "moderator");
  await pool.query("INSERT INTO suspensions (user_id, until, reason, by_id) VALUES ($1, now() + interval '1 day', 'spam', $2)", [users.mod, users.owner]);
  assert.equal((await permissionsFor(pool, users.mod)).size, 0);
  await pool.query("INSERT INTO suspensions (user_id, until, reason, by_id) VALUES ($1, now() - interval '1 day', 'old', $2)", [users.gracz, users.owner]);
  assert.equal(can(await permissionsFor(pool, users.gracz), "post.create"), true);
  await pool.query("INSERT INTO suspensions (user_id, until, reason, by_id) VALUES ($1, NULL, 'ban', $2)", [users.inny, users.owner]);
  assert.equal((await permissionsFor(pool, users.inny)).size, 0);
});

test("mod_log rejects update and delete for fatrainer_app", async (t) => {
  const { pool, users } = await testDb(t);
  const client = await pool.connect();
  try {
    await client.query("SET ROLE fatrainer_app");
    await client.query("INSERT INTO fatrainer.mod_log (actor_id, action, target_type) VALUES ($1, 'pin', 'thread')", [users.owner]);
    await assert.rejects(client.query("UPDATE fatrainer.mod_log SET action = 'x'"), /permission denied/);
    await assert.rejects(client.query("DELETE FROM fatrainer.mod_log"), /permission denied/);
  } finally {
    await client.query("RESET ROLE");
    client.release();
  }
});

test("owner promoted once from env", async (t) => {
  const { pool, users } = await testDb(t);
  assert.equal(await promoteOwner(pool, String(users.owner)), true);
  assert.equal(await promoteOwner(pool, String(users.owner)), false);
  assert.equal(await promoteOwner(pool, "999999"), false);
  assert.equal(await promoteOwner(pool, ""), false);
  assert.equal(await promoteOwner(pool, "1 OR 1=1"), false);
  assert.equal((await pool.query("SELECT count(*)::int n FROM user_roles")).rows[0].n, 1);
  assert.equal((await pool.query("SELECT count(*)::int n FROM mod_log WHERE action = 'role.assign'")).rows[0].n, 1);
  assert.equal(can(await permissionsFor(pool, users.owner), "announcement.create"), true);
});

test("seed contains no threads or posts", async (t) => {
  const { pool } = await testDb(t);
  for (const table of ["threads", "posts", "user_roles", "tags", "reports"]) {
    assert.equal((await pool.query(`SELECT count(*)::int n FROM ${table}`)).rows[0].n, 0, table);
  }
  assert.equal((await pool.query("SELECT count(*)::int n FROM categories")).rows[0].n, 5);
});
