const { test } = require("node:test");
const assert = require("node:assert/strict");
const { startForum } = require("./server.js");

test("forum index lists categories, sets no cookie for visitors and sends security headers", async (t) => {
  const h = await startForum(t);
  const res = await h.anon().get("/forum");
  assert.equal(res.status, 200);
  for (const slug of ["qa", "ideas", "general", "show-and-tell", "announcements"]) assert.match(res.text, new RegExp(`href="/forum/c/${slug}"`));
  assert.match(res.text, /<title>[^<]*FaTrainer Forum<\/title>/);
  assert.match(res.text, /<html lang="en"/);
  assert.equal(res.headers.getSetCookie().length, 0);
  const csp = res.headers.get("content-security-policy");
  assert.match(csp, /default-src 'self'/);
  assert.match(csp, /frame-ancestors 'none'/);
  assert.match(res.text, /href="\/forum\/auth\/login\?return_to=%2Fforum"/);
});

test("signed-in member sees their nickname and a sign out form", async (t) => {
  const h = await startForum(t);
  const { client, csrf } = await h.signedIn("gracz");
  const page = await client.get("/forum");
  assert.match(page.text, /gracz/);
  assert.match(page.text, /action="\/forum\/auth\/logout"/);
  assert.ok(csrf);
});

test("unknown pages render the 404 page in the forum layout", async (t) => {
  const h = await startForum(t);
  const res = await h.anon().get("/forum/nope");
  assert.equal(res.status, 404);
  assert.match(res.text, /<h1[^>]*>Page not found<\/h1>/);
  assert.match(res.text, /<footer/);
  const api = await h.anon().get("/forum/api/nope");
  assert.equal(api.status, 404);
  assert.match(api.headers.get("content-type"), /json/);
});

test("boot promotes the owner from env only once", async (t) => {
  const h = await startForum(t);
  const { boot } = require("../src/app.js");
  await boot({ config: h.config, pool: h.pool });
  const { rows } = await h.pool.query("SELECT r.name FROM user_roles ur JOIN roles r ON r.id = ur.role_id WHERE ur.user_id = $1", [h.users.owner]);
  assert.deepEqual(rows, [{ name: "administrator" }]);
});
