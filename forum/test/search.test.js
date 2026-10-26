const { test } = require("node:test");
const assert = require("node:assert/strict");
const { startForum } = require("./server.js");
const { categoryId } = require("./helpers.js");
const { createThread, createPost } = require("../src/compose.js");
const { searchForum } = require("../src/search.js");

async function setup(t) {
  const h = await startForum(t);
  const general = await categoryId(h.pool, "general");
  const qa = await categoryId(h.pool, "qa");
  const as = (name) => ({ id: h.users[name], username: name });
  return { h, general, qa, as };
}

test("unaccent_search", async (t) => {
  const { h, general, as } = await setup(t);
  await createThread(h.pool, as("gracz"), { categoryId: general, title: "Problem po patchu", body: "Żółw nie działa po aktualizacji" });
  assert.equal((await searchForum(h.pool, { q: "zolw dziala", page: 1 })).items.length, 1);
  assert.equal((await searchForum(h.pool, { q: "ŻÓŁW", page: 1 })).items.length, 1);
  assert.equal((await searchForum(h.pool, { q: "patchu", page: 1 })).items.length, 1);
  assert.equal((await searchForum(h.pool, { q: "kot", page: 1 })).items.length, 0);
});

test("held and deleted posts are not searchable", async (t) => {
  const { h, general, as } = await setup(t);
  const th = await createThread(h.pool, as("gracz"), { categoryId: general, title: "Visible thread", body: "opening words" });
  const held = await createPost(h.pool, as("inny"), th.id, "launcher heldword");
  const gone = await createPost(h.pool, as("inny"), th.id, "launcher goneword");
  await h.pool.query("UPDATE posts SET held = true WHERE id = $1", [held.id]);
  await h.pool.query("UPDATE posts SET deleted_at = now(), deleted_by = author_id WHERE id = $1", [gone.id]);
  assert.equal((await searchForum(h.pool, { q: "heldword", page: 1 })).items.length, 0);
  assert.equal((await searchForum(h.pool, { q: "goneword", page: 1 })).items.length, 0);
  const other = await createThread(h.pool, as("gracz"), { categoryId: general, title: "Removed thread", body: "threadgone" });
  await h.pool.query("UPDATE threads SET deleted_at = now() WHERE id = $1", [other.id]);
  assert.equal((await searchForum(h.pool, { q: "threadgone", page: 1 })).items.length, 0);
});

test("snippet highlights are escaped", async (t) => {
  const { h, general, as } = await setup(t);
  const th = await createThread(h.pool, as("gracz"), { categoryId: general, title: "Odd markup", body: '<script>alert(1)</script> crashword <b onclick="x">bold</b> R&D "quoted"' });
  const { items } = await searchForum(h.pool, { q: "crashword", page: 1 });
  assert.equal(items.length, 1);
  assert.equal(items[0].threadId, th.id);
  assert.equal(items[0].postId, th.postId);
  assert.match(items[0].snippetHtml, /<b>crashword<\/b>/);
  assert.equal(items[0].snippetHtml.includes("<script"), false);
  assert.equal(items[0].snippetHtml.includes("<b onclick"), false);
  assert.match(items[0].snippetHtml, /R&amp;D/);
  assert.match(items[0].snippetHtml, /&quot;quoted/);
  const page = await h.anon().get("/forum/search?q=crashword");
  assert.equal(page.status, 200);
  assert.match(page.text, /<b>crashword<\/b>/);
  assert.match(page.text, new RegExp(`href="/forum/t/${th.id}/odd-markup#p${th.postId}"`));
  assert.match(page.text, /name="robots" content="noindex"/);
});

test("filters narrow results and bad input is refused", async (t) => {
  const { h, general, qa, as } = await setup(t);
  await createThread(h.pool, as("gracz"), { categoryId: general, title: "General keyword", body: "sharedword one" });
  await createThread(h.pool, as("inny"), { categoryId: qa, title: "Question keyword", body: "sharedword two" });
  assert.equal((await searchForum(h.pool, { q: "sharedword", page: 1 })).items.length, 2);
  assert.equal((await searchForum(h.pool, { q: "sharedword", categoryId: qa, page: 1 })).items.length, 1);
  assert.equal((await searchForum(h.pool, { q: "sharedword", author: "gracz", page: 1 })).items.length, 1);
  assert.equal((await searchForum(h.pool, { q: "sharedword", to: "2000-01-01", page: 1 })).items.length, 0);
  await assert.rejects(searchForum(h.pool, { q: "a", page: 1 }), /2 to 200/);
  const page = await h.anon().get("/forum/search?q=a");
  assert.equal(page.status, 422);
  assert.match(page.text, /Search needs 2 to 200 characters\./);
  assert.equal((await h.anon().get("/forum/search")).status, 200);
});
