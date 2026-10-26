const { test } = require("node:test");
const assert = require("node:assert/strict");
const { startForum } = require("./server.js");
const { categoryId } = require("./helpers.js");
const { listThreads } = require("../src/threads.js");

async function seedThread(pool, { category = "general", author, title, slug, pinned = false, created = "2026-10-01", activity = created, posts = ["First post body"] }) {
  const cat = await categoryId(pool, category);
  const { rows } = await pool.query(
    "INSERT INTO threads (category_id, author_id, title, slug, pinned, created_at, last_post_at, reply_count) VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING id",
    [cat, author, title, slug, pinned, created, activity, posts.length - 1],
  );
  const ids = [];
  for (const body of posts) {
    const p = await pool.query("INSERT INTO posts (thread_id, author_id, body_md, body_html, renderer_version) VALUES ($1, $2, $3, $4, 1) RETURNING id", [rows[0].id, author, body, `<p>${body}</p>`]);
    ids.push(p.rows[0].id);
  }
  return { id: rows[0].id, posts: ids };
}

test("empty category shows the empty state with the ask button", async (t) => {
  const h = await startForum(t);
  const res = await h.anon().get("/forum/c/ideas");
  assert.equal(res.status, 200);
  assert.match(res.text, /<h1[^>]*>Ideas<\/h1>/);
  assert.match(res.text, /No threads here yet\. Start the first one\./);
  assert.match(res.text, /href="\/forum\/new\?category=ideas"/);
  assert.equal((await h.anon().get("/forum/c/nope")).status, 404);
});

test("pinned first then by sort", async (t) => {
  const h = await startForum(t);
  const cat = await categoryId(h.pool, "general");
  const a = await seedThread(h.pool, { author: h.users.gracz, title: "Pinned rules thread", slug: "pinned", pinned: true, created: "2026-09-01", activity: "2026-09-02" });
  const b = await seedThread(h.pool, { author: h.users.gracz, title: "Busy older thread", slug: "busy", created: "2026-09-10", activity: "2026-10-20", posts: ["q", "reply"] });
  const c = await seedThread(h.pool, { author: h.users.inny, title: "Newest quiet thread", slug: "quiet", created: "2026-10-05", activity: "2026-10-05" });
  const ids = async (opts) => (await listThreads(h.pool, { categoryId: cat, page: 1, ...opts })).items.map((x) => x.id);
  assert.deepEqual(await ids({ sort: "activity" }), [a.id, b.id, c.id]);
  assert.deepEqual(await ids({ sort: "new" }), [a.id, c.id, b.id]);
  assert.deepEqual(await ids({ sort: "unanswered" }), [a.id, c.id]);
  await h.pool.query("INSERT INTO reactions (post_id, user_id) VALUES ($1, $2), ($1, $3)", [c.posts[0], h.users.gracz, h.users.mod]);
  assert.deepEqual(await ids({ sort: "helpful" }), [a.id, c.id, b.id]);
  assert.deepEqual(await ids({ sort: "activity", mine: h.users.inny }), [c.id]);
  const page = await h.anon().get("/forum/c/general?sort=new");
  assert.ok(page.text.indexOf("Pinned rules thread") < page.text.indexOf("Newest quiet thread"));
  assert.ok(page.text.indexOf("Newest quiet thread") < page.text.indexOf("Busy older thread"));
  assert.match(page.text, /Pinned/);
  const far = await h.anon().get("/forum/c/general?page=9");
  assert.equal(far.status, 301);
  assert.equal(far.headers.get("location"), "/forum/c/general");
});

test("canonical url with slug, wrong slug redirects", async (t) => {
  const h = await startForum(t);
  const th = await seedThread(h.pool, { author: h.users.gracz, title: "How do I bind keys", slug: "how-do-i-bind-keys" });
  const wrong = await h.anon().get(`/forum/t/${th.id}/old-title`);
  assert.equal(wrong.status, 301);
  assert.equal(wrong.headers.get("location"), `/forum/t/${th.id}/how-do-i-bind-keys`);
  const bare = await h.anon().get(`/forum/t/${th.id}`);
  assert.equal(bare.status, 301);
  const ok = await h.anon().get(`/forum/t/${th.id}/how-do-i-bind-keys`);
  assert.equal(ok.status, 200);
  assert.match(ok.text, new RegExp(`<link rel="canonical" href="${h.appUrl}/forum/t/${th.id}/how-do-i-bind-keys">`));
  assert.match(ok.text, /<h1[^>]*>How do I bind keys<\/h1>/);
  assert.match(ok.text, /First post body/);
  assert.equal((await h.anon().get("/forum/t/999999/x")).status, 404);
  assert.equal((await h.anon().get(`/forum/t/${th.id}/how-do-i-bind-keys?page=5`)).headers.get("location"), `/forum/t/${th.id}/how-do-i-bind-keys`);
});

test("deleted posts keep a placeholder", async (t) => {
  const h = await startForum(t);
  const th = await seedThread(h.pool, { author: h.users.gracz, title: "Thread with removals", slug: "removals", posts: ["opening", "secret words", "removed by mod", "closing"] });
  await h.pool.query("UPDATE posts SET deleted_at = now(), deleted_by = author_id WHERE id = $1", [th.posts[1]]);
  await h.pool.query("UPDATE posts SET deleted_at = now(), deleted_by = $2 WHERE id = $1", [th.posts[2], h.users.mod]);
  const res = await h.anon().get(`/forum/t/${th.id}/removals`);
  assert.equal(res.text.includes("secret words"), false);
  assert.equal(res.text.includes("removed by mod"), false);
  const order = ["opening", "Deleted by the author", "Removed by a moderator", "closing"].map((s) => res.text.indexOf(s));
  assert.ok(order.every((i, n) => i > 0 && (n === 0 || i > order[n - 1])), String(order));
});

test("profile hides bio when visibility forbids", async (t) => {
  const h = await startForum(t);
  await h.pool.query("UPDATE aura.users SET bio = 'members only bio', profile_visibility = 'members' WHERE id = $1", [h.users.gracz]);
  await h.pool.query("UPDATE aura.users SET bio = 'hidden bio', profile_visibility = 'nobody' WHERE id = $1", [h.users.inny]);
  await h.pool.query("UPDATE aura.users SET bio = 'public bio' WHERE id = $1", [h.users.mod]);
  await seedThread(h.pool, { author: h.users.gracz, title: "A thread by gracz", slug: "by-gracz" });
  const anon = await h.anon().get("/forum/u/gracz");
  assert.equal(anon.status, 200);
  assert.equal(anon.text.includes("members only bio"), false);
  assert.match(anon.text, /A thread by gracz/);
  assert.match((await h.anon().get("/forum/u/mod")).text, /public bio/);
  const { client } = await h.signedIn("mod");
  assert.match((await client.get("/forum/u/gracz")).text, /members only bio/);
  assert.equal((await client.get("/forum/u/inny")).text.includes("hidden bio"), false);
  assert.match((await h.anon().get("/forum/u/owner")).text, /Administrator/);
});

test("deleted account shown as Deleted account", async (t) => {
  const h = await startForum(t);
  const th = await seedThread(h.pool, { author: h.users.inny, title: "Thread from a leaver", slug: "leaver" });
  await h.pool.query("DELETE FROM aura.users WHERE id = $1", [h.users.inny]);
  const res = await h.anon().get(`/forum/t/${th.id}/leaver`);
  assert.match(res.text, /Deleted account/);
  assert.match((await h.anon().get("/forum/c/general")).text, /Deleted account/);
  assert.equal((await h.anon().get("/forum/u/inny")).status, 404);
});
