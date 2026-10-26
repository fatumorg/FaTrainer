const { test } = require("node:test");
const assert = require("node:assert/strict");
const { startForum } = require("./server.js");
const { categoryId } = require("./helpers.js");
const { createThread, createPost } = require("../src/compose.js");
const { acceptAnswer } = require("../src/engagement.js");
const { runMailBatch, unsubscribeToken } = require("../src/notifications.js");

async function setup(t) {
  const h = await startForum(t);
  const as = (name) => ({ id: h.users[name], username: name });
  const mailer = { sent: [], send: async (m) => mailer.sent.push(m) };
  const batch = () => runMailBatch(h.pool, mailer, { appUrl: h.appUrl, secret: h.config.secret });
  return { h, as, mailer, batch, general: await categoryId(h.pool, "general"), qa: await categoryId(h.pool, "qa") };
}

const kinds = async (h, name) => (await h.pool.query("SELECT kind FROM notifications WHERE user_id = $1 ORDER BY id", [h.users[name]])).rows.map((r) => r.kind);

test("author not notified of own reply", async (t) => {
  const { h, as, general, qa } = await setup(t);
  const th = await createThread(h.pool, as("gracz"), { categoryId: general, title: "Notify thread", body: "start" });
  await createPost(h.pool, as("gracz"), th.id, "my own follow-up");
  assert.deepEqual(await kinds(h, "gracz"), []);
  await createPost(h.pool, as("inny"), th.id, "hello @mod and @gracz");
  assert.deepEqual(await kinds(h, "gracz"), ["reply"]);
  assert.deepEqual(await kinds(h, "mod"), ["mention"]);
  assert.deepEqual(await kinds(h, "inny"), []);
  const q = await createThread(h.pool, as("gracz"), { categoryId: qa, title: "Question here", body: "q" });
  const answer = await createPost(h.pool, as("inny"), q.id, "answer");
  await acceptAnswer(h.pool, as("gracz"), q.id, answer.id);
  assert.deepEqual(await kinds(h, "inny"), ["accepted"]);
  const { client } = await h.signedIn("gracz");
  const page = await client.get("/forum/notifications");
  assert.match(page.text, /inny replied in/);
  assert.match(page.text, /Notifications <span class="count">2<\/span>/);
  const { rows } = await h.pool.query("SELECT id FROM notifications WHERE user_id = $1 ORDER BY id LIMIT 1", [h.users.gracz]);
  const open = await client.get(`/forum/notifications/${rows[0].id}`);
  assert.equal(open.status, 303);
  assert.match(open.headers.get("location"), new RegExp(`^/forum/t/${th.id}/notify-thread`));
  assert.match((await client.get("/forum")).text, /Notifications <span class="count">1<\/span>/);
});

test("ten replies in 10 minutes give one mail", async (t) => {
  const { h, as, general, batch, mailer } = await setup(t);
  const th = await createThread(h.pool, as("gracz"), { categoryId: general, title: "Busy thread", body: "start" });
  for (let i = 0; i < 10; i++) await createPost(h.pool, as("inny"), th.id, `reply ${i}`);
  assert.equal(await batch(), 1);
  assert.equal(mailer.sent.length, 1);
  const mail = mailer.sent[0];
  assert.equal(mail.to, "gracz@x.pl");
  assert.match(mail.subject, /10 new/);
  assert.match(mail.text, /Busy thread/);
  assert.match(mail.headers["List-Unsubscribe"], new RegExp(`^<${h.appUrl}/forum/unsubscribe/[A-Za-z0-9_.-]+>$`));
  assert.equal(mail.headers["List-Unsubscribe-Post"], "List-Unsubscribe=One-Click");
  assert.equal(await batch(), 0);
  await createPost(h.pool, as("inny"), th.id, "one more");
  await h.pool.query("UPDATE forum_prefs SET email_replies = false WHERE user_id = $1", [h.users.gracz]);
  assert.equal(await batch(), 0);
});

test("one-click unsubscribe works without session and is idempotent", async (t) => {
  const { h } = await setup(t);
  const token = unsubscribeToken(h.config.secret, h.users.gracz, "replies");
  const page = await h.anon().get(`/forum/unsubscribe/${token}`);
  assert.equal(page.status, 200);
  assert.match(page.text, /method="post"/);
  for (let i = 0; i < 2; i++) {
    const res = await h.anon().post(`/forum/unsubscribe/${token}`, { "List-Unsubscribe": "One-Click" });
    assert.equal(res.status, 200);
    assert.match(res.text, /no longer get e-mails about replies/);
  }
  const { rows } = await h.pool.query("SELECT email_replies, email_mentions FROM forum_prefs WHERE user_id = $1", [h.users.gracz]);
  assert.deepEqual(rows, [{ email_replies: false, email_mentions: true }]);
  assert.equal((await h.anon().post(`/forum/unsubscribe/${token.slice(0, -2)}xx`, {})).status, 404);
  assert.equal((await h.anon().get("/forum/unsubscribe/nonsense")).status, 404);
});

test("email address read at send time, never stored by the forum", async (t) => {
  const { h, as, general, batch, mailer } = await setup(t);
  const th = await createThread(h.pool, as("gracz"), { categoryId: general, title: "Privacy thread", body: "start" });
  await createPost(h.pool, as("inny"), th.id, "reply @gracz");
  assert.equal(await batch(), 1);
  assert.equal(mailer.sent[0].to, "gracz@x.pl");
  const tables = (await h.pool.query("SELECT tablename FROM pg_tables WHERE schemaname = 'fatrainer'")).rows.map((r) => r.tablename);
  for (const table of tables) {
    const { rows } = await h.pool.query(`SELECT count(*)::int AS n FROM fatrainer.${table} x WHERE x::text LIKE '%@x.pl%'`);
    assert.equal(rows[0].n, 0, table);
  }
  await h.pool.query("UPDATE aura.users SET status = 'deactivated' WHERE id = $1", [h.users.gracz]);
  await createPost(h.pool, as("inny"), th.id, "another");
  assert.equal(await batch(), 0);
});

test("forum settings save e-mail and watch preferences", async (t) => {
  const { h } = await setup(t);
  const { client, csrf } = await h.signedIn("gracz");
  const page = await client.get("/forum/settings");
  assert.equal(page.status, 200);
  assert.match(page.text, /name="email_replies"[^>]*checked/);
  const res = await client.post("/forum/settings", { _csrf: csrf, email_mentions: "on" });
  assert.equal(res.status, 303);
  const { rows } = await h.pool.query("SELECT email_replies, email_mentions, auto_watch FROM forum_prefs WHERE user_id = $1", [h.users.gracz]);
  assert.deepEqual(rows, [{ email_replies: false, email_mentions: true, auto_watch: false }]);
});
