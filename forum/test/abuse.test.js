const { test } = require("node:test");
const assert = require("node:assert/strict");
const { startForum } = require("./server.js");
const { categoryId, grantRole } = require("./helpers.js");
const { createThread, createPost } = require("../src/compose.js");
const { checkPostAllowed } = require("../src/abuse.js");

async function setup(t) {
  const h = await startForum(t);
  await grantRole(h.pool, h.users.mod, "moderator");
  const as = (name) => ({ id: h.users[name], username: name });
  return { h, as, general: await categoryId(h.pool, "general") };
}

test("new member link-heavy post held", async (t) => {
  const { h, as, general } = await setup(t);
  const th = await createThread(h.pool, as("gracz"), { categoryId: general, title: "Links thread", body: "start" });
  const links = "see https://a.example https://b.example and [c](https://c.example)";
  assert.deepEqual(await checkPostAllowed(h.pool, as("inny"), { body: links, threadId: th.id }), { ok: true, hold: true });
  const p = await createPost(h.pool, as("inny"), th.id, links);
  assert.equal(p.held, true);
  assert.equal((await h.pool.query("SELECT held FROM posts WHERE id = $1", [p.id])).rows[0].held, true);
  assert.equal((await h.pool.query("SELECT reply_count FROM threads WHERE id = $1", [th.id])).rows[0].reply_count, 0);
  assert.equal((await h.pool.query("SELECT count(*)::int AS n FROM notifications WHERE user_id = $1", [h.users.gracz])).rows[0].n, 0);
  assert.equal((await h.anon().get(`/forum/t/${th.id}/links-thread`)).text.includes("b.example"), false);
  const author = await h.signedIn("inny");
  assert.match((await author.client.get(`/forum/t/${th.id}/links-thread`)).text, /Waiting for a moderator/);
  const mod = await h.signedIn("mod");
  assert.match((await mod.client.get("/forum/mod")).text, /b\.example/);
  const ok = await mod.client.post(`/forum/mod/posts/${p.id}/approve`, { _csrf: mod.csrf });
  assert.equal(ok.status, 303);
  assert.match((await h.anon().get(`/forum/t/${th.id}/links-thread`)).text, /b\.example/);
  assert.equal((await h.pool.query("SELECT count(*)::int AS n FROM notifications WHERE user_id = $1", [h.users.gracz])).rows[0].n, 1);
  assert.deepEqual((await h.pool.query("SELECT action FROM mod_log WHERE actor_id = $1", [h.users.mod])).rows, [{ action: "post.approve" }]);
  await h.pool.query("UPDATE forum_prefs SET joined_at = now() - interval '30 days' WHERE user_id = $1", [h.users.inny]);
  for (let i = 0; i < 3; i++) await createPost(h.pool, as("inny"), th.id, `normal reply ${i}`);
  assert.deepEqual(await checkPostAllowed(h.pool, as("inny"), { body: links + " again", threadId: th.id }), { ok: true, hold: false });
});

test("duplicate within 10 minutes refused", async (t) => {
  const { h, as, general } = await setup(t);
  const th = await createThread(h.pool, as("gracz"), { categoryId: general, title: "Dup thread", body: "start" });
  await createPost(h.pool, as("inny"), th.id, "same words");
  await assert.rejects(createPost(h.pool, as("inny"), th.id, "  same words "), /already posted this/);
  await h.pool.query("UPDATE posts SET created_at = now() - interval '11 minutes' WHERE body_md = 'same words'");
  await createPost(h.pool, as("inny"), th.id, "same words");
});

test("fourth thread on day one refused with a clear message", async (t) => {
  const { h, as, general } = await setup(t);
  for (let i = 0; i < 3; i++) await createThread(h.pool, as("inny"), { categoryId: general, title: `New member thread ${i}`, body: `body ${i}` });
  await assert.rejects(createThread(h.pool, as("inny"), { categoryId: general, title: "Fourth thread", body: "body 4" }), /New members can start 3 threads a day/);
  const { client, csrf } = await h.signedIn("inny");
  const res = await client.post("/forum/new", { _csrf: csrf, category: "general", title: "Fourth thread", body: "body 4" });
  assert.equal(res.status, 429);
  assert.match(res.text, /New members can start 3 threads a day/);
  await createThread(h.pool, as("owner"), { categoryId: general, title: "Owner thread", body: "fine" });
  await h.pool.query("INSERT INTO suspensions (user_id, until, reason) VALUES ($1, NULL, 'ban')", [h.users.gracz]);
  assert.deepEqual(await checkPostAllowed(h.pool, as("gracz"), { body: "x" }), { ok: false, reason: "suspended", message: "Your account cannot post right now. You can still read." });
});

test("honeypot and minimum compose time", async (t) => {
  const { h } = await setup(t);
  const { client, csrf } = await h.signedIn("gracz");
  const form = await client.get("/forum/new?category=general");
  const started = form.text.match(/name="started" value="([^"]+)"/)[1];
  assert.match(form.text, /name="website"/);
  h.config.minComposeMs = 3000;
  const fast = await client.post("/forum/new", { _csrf: csrf, started, category: "general", title: "Too fast here", body: "x" });
  assert.equal(fast.status, 422);
  assert.match(fast.text, /Take a moment/);
  h.config.minComposeMs = 0;
  const bot = await client.post("/forum/new", { _csrf: csrf, started, website: "http://spam", category: "general", title: "Bot thread", body: "x" });
  assert.equal(bot.status, 422);
  const ok = await client.post("/forum/new", { _csrf: csrf, started, category: "general", title: "Human thread", body: "x" });
  assert.equal(ok.status, 303);
});
