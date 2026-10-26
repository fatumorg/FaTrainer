const { test } = require("node:test");
const assert = require("node:assert/strict");
const { startForum } = require("./server.js");
const { categoryId, grantRole } = require("./helpers.js");
const { createThread, createPost } = require("../src/compose.js");
const e = require("../src/engagement.js");

async function setup(t) {
  const h = await startForum(t);
  const as = (name) => ({ id: h.users[name], username: name });
  return { h, as, qa: await categoryId(h.pool, "qa"), general: await categoryId(h.pool, "general") };
}

test("one helpful per user, not on own post", async (t) => {
  const { h, as, general } = await setup(t);
  const th = await createThread(h.pool, as("gracz"), { categoryId: general, title: "Helpful thread", body: "q" });
  await assert.rejects(e.toggleHelpful(h.pool, h.users.gracz, th.postId), /your own post/);
  assert.deepEqual(await e.toggleHelpful(h.pool, h.users.inny, th.postId), { helpful: true, count: 1 });
  assert.deepEqual(await e.toggleHelpful(h.pool, h.users.mod, th.postId), { helpful: true, count: 2 });
  assert.deepEqual(await e.toggleHelpful(h.pool, h.users.inny, th.postId), { helpful: false, count: 1 });
  const { client, csrf } = await h.signedIn("inny");
  const res = await client.post(`/forum/p/${th.postId}/helpful`, { _csrf: csrf });
  assert.equal(res.status, 303);
  assert.equal(res.headers.get("location"), `/forum/t/${th.id}/helpful-thread#p${th.postId}`);
  const page = await client.get(`/forum/t/${th.id}/helpful-thread`);
  assert.match(page.text, /aria-pressed="true"[^>]*>Helpful/);
  const json = await client.post(`/forum/p/${th.postId}/helpful`, { _csrf: csrf }, { accept: "application/json" });
  assert.deepEqual(json.json, { helpful: false, count: 1 });
});

test("accept only in Q&A by author or moderator", async (t) => {
  const { h, as, qa, general } = await setup(t);
  const q = await createThread(h.pool, as("gracz"), { categoryId: qa, title: "How to bind keys", body: "question" });
  const answer = await createPost(h.pool, as("inny"), q.id, "use the keys tab");
  await assert.rejects(e.acceptAnswer(h.pool, as("inny"), q.id, answer.id), /thread author or a moderator/);
  await assert.rejects(e.acceptAnswer(h.pool, as("gracz"), q.id, q.postId), /reply/);
  assert.deepEqual(await e.acceptAnswer(h.pool, as("gracz"), q.id, answer.id), { solvedPostId: answer.id, authorId: h.users.inny });
  assert.deepEqual(await e.acceptAnswer(h.pool, as("gracz"), q.id, answer.id), { solvedPostId: null, authorId: h.users.inny });
  await grantRole(h.pool, h.users.mod, "moderator");
  assert.equal((await e.acceptAnswer(h.pool, as("mod"), q.id, answer.id)).solvedPostId, answer.id);
  const g = await createThread(h.pool, as("gracz"), { categoryId: general, title: "Not a question", body: "chat" });
  const reply = await createPost(h.pool, as("inny"), g.id, "reply");
  await assert.rejects(e.acceptAnswer(h.pool, as("gracz"), g.id, reply.id), /Q&A/);
  const page = await h.anon().get(`/forum/t/${q.id}/how-to-bind-keys`);
  assert.match(page.text, /Accepted answer/);
  assert.match((await h.anon().get("/forum/u/inny")).text, /<span class="num">1<\/span> solved answer/);
});

test("bookmark list paginated", async (t) => {
  const { h, as, general } = await setup(t);
  const ids = [];
  for (let i = 0; i < 27; i++) ids.push((await createThread(h.pool, as("gracz"), { categoryId: general, title: `Saved thread ${i}`, body: "x" })).id);
  for (const id of ids) assert.deepEqual(await e.toggleBookmark(h.pool, h.users.inny, id), { saved: true });
  assert.deepEqual(await e.toggleBookmark(h.pool, h.users.inny, ids[0]), { saved: false });
  const first = await e.listBookmarks(h.pool, h.users.inny, 1);
  assert.equal(first.items.length, 25);
  assert.equal(first.pages, 2);
  assert.equal((await e.listBookmarks(h.pool, h.users.inny, 2)).items.length, 1);
  const { client, csrf } = await h.signedIn("inny");
  const saved = await client.get("/forum/saved?page=2");
  assert.equal(saved.status, 200);
  assert.match(saved.text, /Saved thread/);
  assert.equal((await h.anon().get("/forum/saved", { accept: "text/html" })).status, 302);
  const res = await client.post(`/forum/t/${ids[0]}/bookmark`, { _csrf: csrf });
  assert.equal(res.status, 303);
  assert.equal((await e.listBookmarks(h.pool, h.users.inny, 1)).total, 27);
});

test("unsubscribe stops notifications", async (t) => {
  const { h, as, general } = await setup(t);
  const th = await createThread(h.pool, as("gracz"), { categoryId: general, title: "Watched thread", body: "x" });
  await createPost(h.pool, as("inny"), th.id, "reply");
  assert.deepEqual((await e.watchersOf(h.pool, th.id)).sort(), [h.users.gracz, h.users.inny].sort());
  await e.setSubscription(h.pool, h.users.inny, { threadId: th.id, muted: true });
  assert.deepEqual(await e.watchersOf(h.pool, th.id), [h.users.gracz]);
  await createPost(h.pool, as("inny"), th.id, "again");
  assert.deepEqual(await e.watchersOf(h.pool, th.id), [h.users.gracz]);
  await e.setSubscription(h.pool, h.users.mod, { categoryId: general, muted: false });
  assert.deepEqual((await e.watchersOf(h.pool, th.id)).sort(), [h.users.gracz, h.users.mod].sort());
  await h.pool.query("UPDATE forum_prefs SET auto_watch = false WHERE user_id = $1", [h.users.gracz]);
  const second = await createThread(h.pool, as("gracz"), { categoryId: general, title: "No auto watch", body: "x" });
  assert.deepEqual(await e.watchersOf(h.pool, second.id), [h.users.mod]);
  const { client, csrf } = await h.signedIn("gracz");
  assert.equal((await client.post(`/forum/t/${th.id}/watch`, { _csrf: csrf, muted: "1" })).status, 303);
  assert.deepEqual(await e.watchersOf(h.pool, th.id), [h.users.mod]);
  const watching = await client.get("/forum/watching");
  assert.equal(watching.status, 200);
});
