const { test } = require("node:test");
const assert = require("node:assert/strict");
const { startForum } = require("./server.js");
const { categoryId, grantRole } = require("./helpers.js");
const { createThread, createPost } = require("../src/compose.js");
const m = require("../src/moderation.js");

async function setup(t) {
  const h = await startForum(t);
  await grantRole(h.pool, h.users.mod, "moderator");
  const as = (name) => ({ id: h.users[name], username: name });
  const th = await createThread(h.pool, as("gracz"), { categoryId: await categoryId(h.pool, "general"), title: "Reported thread", body: "fine opening" });
  const bad = await createPost(h.pool, as("gracz"), th.id, "rude words here");
  const report = await m.fileReport(h.pool, as("inny"), { targetType: "post", targetId: bad.id, reason: "harassment", details: "insults" });
  return { h, as, th, bad, report };
}

const decision = { action: "remove", rule: "harassment", reason: "Insults other players by name." };

test("two moderators deciding the same report: one ok, one already_decided", async (t) => {
  const { h, as, report } = await setup(t);
  const results = await Promise.all([m.decide(h.pool, as("mod"), report.id, decision), m.decide(h.pool, as("owner"), report.id, decision)]);
  assert.deepEqual(results.sort(), ["already_decided", "ok"]);
  await assert.rejects(m.decide(h.pool, as("inny"), report.id, decision), /permission/);
  const second = await m.fileReport(h.pool, as("mod"), { targetType: "thread", targetId: (await h.pool.query("SELECT id FROM threads")).rows[0].id, reason: "spam" });
  await assert.rejects(m.decide(h.pool, as("mod"), second.id, { ...decision, reason: "short" }), /10 to 1000/);
});

test("author receives a statement of reasons notification with the rule and appeal link", async (t) => {
  const { h, as, bad, report } = await setup(t);
  assert.equal(await m.decide(h.pool, as("mod"), report.id, decision), "ok");
  assert.ok((await h.pool.query("SELECT deleted_at FROM posts WHERE id = $1", [bad.id])).rows[0].deleted_at);
  const { rows } = await h.pool.query("SELECT id, kind, report_id FROM notifications WHERE user_id = $1 AND kind = 'moderation'", [h.users.gracz]);
  assert.equal(rows.length, 1);
  assert.equal(rows[0].report_id, report.id);
  const outcome = await h.pool.query("SELECT kind FROM notifications WHERE user_id = $1 AND kind = 'report_outcome'", [h.users.inny]);
  assert.equal(outcome.rowCount, 1);
  const { client } = await h.signedIn("gracz");
  const open = await client.get(`/forum/notifications/${rows[0].id}`);
  assert.equal(open.headers.get("location"), `/forum/appeal/${report.id}`);
  const page = await client.get(`/forum/appeal/${report.id}`);
  assert.equal(page.status, 200);
  assert.match(page.text, /No hate, threats or harassment/);
  assert.match(page.text, /Insults other players by name\./);
  assert.match(page.text, new RegExp(`action="/forum/appeal/${report.id}"`));
  const stranger = await h.signedIn("owner");
  assert.equal((await stranger.client.get(`/forum/appeal/${report.id}`)).status, 404);
});

test("appeal after 6 months refused", async (t) => {
  const { h, as, report } = await setup(t);
  await m.decide(h.pool, as("mod"), report.id, decision);
  await h.pool.query("UPDATE reports SET decided_at = now() - interval '7 months' WHERE id = $1", [report.id]);
  assert.deepEqual(await m.fileAppeal(h.pool, as("gracz"), report.id, "Please look again"), { error: "window_closed" });
  await h.pool.query("UPDATE reports SET decided_at = now() - interval '5 months' WHERE id = $1", [report.id]);
  assert.ok((await m.fileAppeal(h.pool, as("gracz"), report.id, "Please look again")).id);
  assert.deepEqual(await m.fileAppeal(h.pool, as("gracz"), report.id, "Again"), { error: "already_open" });
  assert.deepEqual(await m.fileAppeal(h.pool, as("inny"), report.id, "Not mine"), { error: "not_found" });
});

test("appeal reviewed by a different moderator when available", async (t) => {
  const { h, as, bad, report } = await setup(t);
  await m.decide(h.pool, as("mod"), report.id, decision);
  const { client, csrf } = await h.signedIn("gracz");
  const res = await client.post(`/forum/appeal/${report.id}`, { _csrf: csrf, text: "It was a joke between friends." });
  assert.equal(res.status, 303);
  const appeal = (await h.pool.query("SELECT id FROM appeals WHERE report_id = $1", [report.id])).rows[0];
  await assert.rejects(m.decideAppeal(h.pool, as("mod"), appeal.id, "reversed"), /different moderator/);
  assert.equal(await m.decideAppeal(h.pool, as("owner"), appeal.id, "reversed"), "ok");
  assert.equal((await h.pool.query("SELECT deleted_at FROM posts WHERE id = $1", [bad.id])).rows[0].deleted_at, null);
  assert.equal((await h.pool.query("SELECT count(*)::int AS n FROM notifications WHERE user_id = $1 AND kind = 'appeal_outcome'", [h.users.gracz])).rows[0].n, 1);
  assert.equal(await m.decideAppeal(h.pool, as("owner"), appeal.id, "upheld"), "already_decided");
  const lone = await m.fileReport(h.pool, as("inny"), { targetType: "post", targetId: bad.id, reason: "spam" });
  await h.pool.query("DELETE FROM user_roles WHERE user_id = $1", [h.users.mod]);
  await m.decide(h.pool, as("owner"), lone.id, { ...decision, rule: "spam" });
  const a2 = await m.fileAppeal(h.pool, as("gracz"), lone.id, "Not spam");
  assert.equal(await m.decideAppeal(h.pool, as("owner"), a2.id, "upheld"), "ok");
});

test("report confirmation page shown", async (t) => {
  const { h, bad } = await setup(t);
  const visitor = await h.anon().get(`/forum/report?post=${bad.id}`);
  assert.equal(visitor.status, 200);
  assert.match(visitor.text, /contact@fatum\.cc/);
  const { client, csrf } = await h.signedIn("mod");
  const form = await client.get(`/forum/report?post=${bad.id}`);
  assert.match(form.text, /name="reason"/);
  const res = await client.post("/forum/report", { _csrf: csrf, target_type: "post", target_id: String(bad.id), reason: "personal_data", details: "phone number" });
  assert.equal(res.status, 303);
  assert.equal(res.headers.get("location"), "/forum/report/sent");
  assert.match((await client.get("/forum/report/sent")).text, /Thanks, your report reached the moderators/);
  const bad2 = await client.post("/forum/report", { _csrf: csrf, target_type: "post", target_id: String(bad.id), reason: "nonsense" });
  assert.equal(bad2.status, 422);
  const queue = await client.get("/forum/mod");
  assert.equal(queue.status, 200);
  assert.match(queue.text, /rude words here/);
  assert.match(queue.text, /Personal data/);
  assert.equal((await (await h.signedIn("inny")).client.get("/forum/mod")).status, 403);
});

test("mod_log entries for pin, lock, move, remove, suspend", async (t) => {
  const { h, th } = await setup(t);
  const { client, csrf } = await h.signedIn("mod");
  const ideas = await categoryId(h.pool, "ideas");
  for (const [action, extra] of [["pin", {}], ["lock", {}], ["move", { category: "ideas" }], ["delete", {}]]) {
    const res = await client.post(`/forum/mod/t/${th.id}/${action}`, { _csrf: csrf, reason: "Moderation test reason", rule: "topic", ...extra });
    assert.equal(res.status, 303, action);
  }
  const t1 = (await h.pool.query("SELECT pinned, locked, category_id, deleted_at FROM threads WHERE id = $1", [th.id])).rows[0];
  assert.deepEqual([t1.pinned, t1.locked, t1.category_id, Boolean(t1.deleted_at)], [true, true, ideas, true]);
  const sus = await client.post(`/forum/mod/users/${h.users.gracz}/suspend`, { _csrf: csrf, until: "2099-01-01", reason: "Repeated insults after warnings", rule: "harassment" });
  assert.equal(sus.status, 303);
  const ban = await client.post(`/forum/mod/users/${h.users.gracz}/suspend`, { _csrf: csrf, until: "", reason: "Repeated insults after warnings", rule: "harassment" });
  assert.equal(ban.status, 403);
  const { rows } = await h.pool.query("SELECT action FROM mod_log WHERE actor_id = $1 ORDER BY id", [h.users.mod]);
  assert.deepEqual(rows.map((r) => r.action), ["thread.pin", "thread.lock", "thread.move", "report.remove", "user.suspend"]);
  const log = await (await h.signedIn("owner")).client.get("/forum/mod/log");
  assert.equal(log.status, 200);
  assert.match(log.text, /thread\.move/);
  assert.equal((await (await h.signedIn("inny")).client.get("/forum/mod/log")).status, 403);
});
