const { test } = require("node:test");
const assert = require("node:assert/strict");
const { startForum } = require("./server.js");
const { categoryId, grantRole } = require("./helpers.js");
const { createThread, slugify } = require("../src/compose.js");

const user = (h, name) => ({ id: h.users[name], username: name });

test("slugify keeps ascii words", () => {
  assert.equal(slugify("Crash on start: Zażółć gęślą jaźń!"), "crash-on-start-zazolc-gesla-jazn");
  assert.equal(slugify("!!!"), "thread");
});

test("new thread form needs sign-in and creates a thread with tags", async (t) => {
  const h = await startForum(t);
  const visitor = await h.anon().get("/forum/new?category=general", { accept: "text/html" });
  assert.equal(visitor.status, 302);
  assert.match(visitor.headers.get("location"), /\/forum\/auth\/login\?return_to=%2Fforum%2Fnew%3Fcategory%3Dgeneral/);
  const expired = await h.anon().post("/forum/new", { category: "general", title: "Crash on start", body: "text" });
  assert.equal(expired.status, 303);
  assert.match(expired.headers.get("location"), /return_to=%2Fforum%2Fnew%3Fcategory%3Dgeneral/);
  await h.pool.query("INSERT INTO tags (slug, name) VALUES ('crash', 'Crash'), ('launcher', 'Launcher')");
  const { client, csrf } = await h.signedIn("gracz");
  const form = await client.get("/forum/new?category=general");
  assert.equal(form.status, 200);
  assert.match(form.text, /<label for="title">Title<\/label>/);
  assert.match(form.text, /<label for="body">Post<\/label>/);
  assert.match(form.text, /<option value="general" selected>/);
  assert.equal(form.text.includes('value="announcements"'), false);
  const res = await client.post("/forum/new", [["_csrf", csrf], ["category", "general"], ["title", "Crash on start"], ["body", "It **crashes** @mod"], ["tags", "crash"], ["tags", "launcher"]]);
  assert.equal(res.status, 303);
  const m = res.headers.get("location").match(/^\/forum\/t\/(\d+)\/crash-on-start\?posted=1#p\d+$/);
  assert.ok(m, res.headers.get("location"));
  const page = await client.get(`/forum/t/${m[1]}/crash-on-start`);
  assert.match(page.text, /<strong>crashes<\/strong>/);
  assert.match(page.text, /href="\/forum\/u\/mod" class="mention"/);
  assert.match(page.text, /href="\/forum\/tags\/crash"/);
  const watched = await h.pool.query("SELECT 1 FROM subscriptions WHERE user_id = $1 AND thread_id = $2", [h.users.gracz, m[1]]);
  assert.equal(watched.rowCount, 1);
  const bad = await client.post("/forum/new", { _csrf: csrf, category: "general", title: "abc", body: "x" });
  assert.equal(bad.status, 422);
  assert.match(bad.text, /Title needs at least 5 characters\./);
  assert.match(bad.text, /value="abc"/);
  const noCsrf = await client.post("/forum/new", { category: "general", title: "Crash on start", body: "x" });
  assert.equal(noCsrf.status, 403);
});

test("locked thread refuses replies with a clear message", async (t) => {
  const h = await startForum(t);
  const th = await createThread(h.pool, user(h, "gracz"), { categoryId: await categoryId(h.pool, "general"), title: "Locked one here", body: "first" });
  await h.pool.query("UPDATE threads SET locked = true WHERE id = $1", [th.id]);
  const { client, csrf } = await h.signedIn("inny");
  const res = await client.post(`/forum/t/${th.id}/reply`, { _csrf: csrf, body: "let me in" });
  assert.equal(res.status, 403);
  assert.match(res.text, /This thread is locked\. New replies are closed\./);
  const page = await client.get(`/forum/t/${th.id}/${th.slug}`);
  assert.match(page.text, /This thread is locked\. New replies are closed\./);
  assert.equal(page.text.includes(`action="/forum/t/${th.id}/reply"`), false);
  const { rows } = await h.pool.query("SELECT count(*)::int AS n FROM posts WHERE thread_id = $1", [th.id]);
  assert.equal(rows[0].n, 1);
});

test("reply lands on the right page and bumps the thread", async (t) => {
  const h = await startForum(t);
  const th = await createThread(h.pool, user(h, "gracz"), { categoryId: await categoryId(h.pool, "general"), title: "Open thread", body: "first" });
  const { client, csrf } = await h.signedIn("inny");
  const page = await client.get(`/forum/t/${th.id}/${th.slug}`);
  assert.match(page.text, new RegExp(`action="/forum/t/${th.id}/reply"`));
  const res = await client.post(`/forum/t/${th.id}/reply`, { _csrf: csrf, body: "a reply" });
  assert.equal(res.status, 303);
  assert.match(res.headers.get("location"), new RegExp(`^/forum/t/${th.id}/open-thread\\?posted=1#p\\d+$`));
  const { rows } = await h.pool.query("SELECT reply_count FROM threads WHERE id = $1", [th.id]);
  assert.equal(rows[0].reply_count, 1);
});

test("edit keeps revision and marks edited", async (t) => {
  const h = await startForum(t);
  const th = await createThread(h.pool, user(h, "gracz"), { categoryId: await categoryId(h.pool, "general"), title: "Edit me please", body: "original words" });
  const { client, csrf } = await h.signedIn("gracz");
  const form = await client.get(`/forum/p/${th.postId}/edit`);
  assert.equal(form.status, 200);
  assert.match(form.text, /original words/);
  assert.match(form.text, /name="title"/);
  const res = await client.post(`/forum/p/${th.postId}/edit`, { _csrf: csrf, title: "Edited title now", body: "changed words" });
  assert.equal(res.status, 303);
  const rev = await h.pool.query("SELECT body_md, edited_by FROM post_revisions WHERE post_id = $1", [th.postId]);
  assert.deepEqual(rev.rows, [{ body_md: "original words", edited_by: h.users.gracz }]);
  const page = await client.get(`/forum/t/${th.id}/edited-title-now`);
  assert.equal(page.status, 200);
  assert.match(page.text, /changed words/);
  assert.match(page.text, /edited/);
  const del = await client.post(`/forum/p/${th.postId}/delete`, { _csrf: csrf });
  assert.equal(del.status, 303);
  assert.match((await client.get(`/forum/t/${th.id}/edited-title-now`)).text, /Deleted by the author\./);
});

test("only author or post.edit.any can edit", async (t) => {
  const h = await startForum(t);
  const th = await createThread(h.pool, user(h, "gracz"), { categoryId: await categoryId(h.pool, "general"), title: "Not yours to edit", body: "mine" });
  const other = await h.signedIn("inny");
  assert.equal((await other.client.get(`/forum/p/${th.postId}/edit`)).status, 403);
  assert.equal((await other.client.post(`/forum/p/${th.postId}/edit`, { _csrf: other.csrf, body: "hacked" })).status, 403);
  assert.equal((await other.client.post(`/forum/p/${th.postId}/delete`, { _csrf: other.csrf })).status, 403);
  await grantRole(h.pool, h.users.mod, "moderator");
  const mod = await h.signedIn("mod");
  const noReason = await mod.client.post(`/forum/p/${th.postId}/edit`, { _csrf: mod.csrf, title: "Not yours to edit", body: "moderated" });
  assert.equal(noReason.status, 422);
  assert.match(noReason.text, /Give a reason/);
  const ok = await mod.client.post(`/forum/p/${th.postId}/edit`, { _csrf: mod.csrf, title: "Not yours to edit", body: "moderated", reason: "personal data" });
  assert.equal(ok.status, 303);
  const log = await h.pool.query("SELECT action, reason FROM mod_log WHERE actor_id = $1", [h.users.mod]);
  assert.deepEqual(log.rows, [{ action: "post.edit", reason: "personal data" }]);
  const body = (await h.pool.query("SELECT body_md FROM posts WHERE id = $1", [th.postId])).rows[0].body_md;
  assert.equal(body, "moderated");
});

test("body over 20000 chars refused with field error", async (t) => {
  const h = await startForum(t);
  const th = await createThread(h.pool, user(h, "gracz"), { categoryId: await categoryId(h.pool, "general"), title: "Long body test", body: "first" });
  const { client, csrf } = await h.signedIn("inny");
  const res = await client.post(`/forum/t/${th.id}/reply`, { _csrf: csrf, body: "x".repeat(20001) });
  assert.equal(res.status, 422);
  assert.match(res.text, /Post is too long \(20001\/20000 characters\)\./);
  assert.match(res.text, /aria-invalid="true"/);
  assert.match(res.text, /id="body-error"/);
});

test("preview renders markdown for signed-in users only", async (t) => {
  const h = await startForum(t);
  assert.equal((await h.anon().post("/forum/preview", { body: "**x**" })).status, 401);
  const { client, csrf } = await h.signedIn("gracz");
  const res = await client.post("/forum/preview", { _csrf: csrf, body: "**x** <b>" });
  assert.equal(res.status, 200);
  assert.equal(res.json.html, "<p><strong>x</strong> &lt;b&gt;</p>");
  const nojs = await client.post("/forum/new", { _csrf: csrf, category: "general", title: "Preview only", body: "**y**", preview: "1" });
  assert.equal(nojs.status, 200);
  assert.match(nojs.text, /<strong>y<\/strong>/);
  assert.equal((await h.pool.query("SELECT count(*)::int AS n FROM threads")).rows[0].n, 0);
});
