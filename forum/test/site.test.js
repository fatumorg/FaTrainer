const { test } = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const { startForum } = require("./server.js");
const { categoryId } = require("./helpers.js");
const { createThread } = require("../src/compose.js");

const site = path.resolve(__dirname, "..", "..", "site");
const read = (f) => fs.readFileSync(path.join(site, f), "utf8");

test("forum.html points to /forum", () => {
  const html = read("forum.html");
  assert.match(html, /<meta http-equiv="refresh" content="0; url=\/forum">/);
  assert.match(html, /<a href="\/forum">/);
  for (const f of ["index.html", "dying-light/index.html"]) assert.match(read(f), /href="\/forum"/);
});

test("legal pages mention cookies only for forum sign-in", () => {
  for (const f of ["legal.html", "prawne.html"]) {
    const html = read(f);
    assert.match(html, /__Host-ft_sid/);
    assert.equal(html.includes("GitHub Discussions"), false);
  }
  assert.match(read("legal.html"), /no cookies or other data on your device until you sign in/);
  assert.match(read("prawne.html"), /dopóki nie zalogujesz się na Forum/);
});

test("style has a daylight theme and robots lists the sitemap", () => {
  assert.match(read("assets/style.css"), /prefers-color-scheme: light/);
  assert.match(read("robots.txt"), /Sitemap: https:\/\/fatrainer\.fatum\.cc\/forum\/sitemap\.xml/);
});

test("guidelines link to report and appeal", async (t) => {
  const h = await startForum(t);
  const res = await h.anon().get("/forum/guidelines");
  assert.equal(res.status, 200);
  assert.match(res.text, /href="\/forum\/report"/);
  assert.match(res.text, /appeal/i);
  assert.match(res.text, /href="\/prawne\.html#forum"/);
});

test("sitemap lists categories and public threads only", async (t) => {
  const h = await startForum(t);
  const general = await categoryId(h.pool, "general");
  const as = { id: h.users.gracz, username: "gracz" };
  const pub = await createThread(h.pool, as, { categoryId: general, title: "Public thread", body: "one" });
  const gone = await createThread(h.pool, as, { categoryId: general, title: "Removed thread", body: "two" });
  await h.pool.query("UPDATE threads SET deleted_at = now() WHERE id = $1", [gone.id]);
  const res = await h.anon().get("/forum/sitemap.xml");
  assert.equal(res.status, 200);
  assert.match(res.headers.get("content-type"), /xml/);
  assert.match(res.text, /\/forum\/c\/general</);
  assert.match(res.text, new RegExp(`/forum/t/${pub.id}/public-thread<`));
  assert.equal(res.text.includes(`/forum/t/${gone.id}/`), false);
  assert.equal(res.text.includes("/forum/u/"), false);
});
