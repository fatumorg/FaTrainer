const { test } = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const sharp = require("sharp");
const { startForum } = require("./server.js");
const { categoryId } = require("./helpers.js");
const { cleanupUploads } = require("../src/uploads.js");
const { createThread } = require("../src/compose.js");

const png = (color = "#c33") => sharp({ create: { width: 12, height: 8, channels: 3, background: color } }).png().toBuffer();

async function upload(client, csrf, buffer, type = "image/png") {
  return client.raw("/forum/api/uploads", buffer, type, { "x-csrf-token": csrf });
}

test("upload limited to 4 per draft", async (t) => {
  const h = await startForum(t);
  const { client, csrf } = await h.signedIn("gracz");
  const ids = [];
  for (let i = 0; i < 4; i++) {
    const res = await upload(client, csrf, await png());
    assert.equal(res.status, 201, res.text);
    assert.match(res.json.id, /^[a-f0-9]{32}$/);
    assert.equal(res.json.url, `/forum/media/${res.json.id}.webp`);
    assert.equal(res.json.markdown, `![image](/forum/media/${res.json.id}.webp)`);
    assert.ok(fs.existsSync(path.join(h.uploadDir, `${res.json.id}.webp`)));
    ids.push(res.json.id);
  }
  const fifth = await upload(client, csrf, await png());
  assert.equal(fifth.status, 429);
  assert.equal(fifth.json.error, "UPLOAD_LIMIT");
  const body = ids.map((id) => `![a](/forum/media/${id}.webp)`).join("\n");
  const th = await createThread(h.pool, { id: h.users.gracz }, { categoryId: await categoryId(h.pool, "general"), title: "Four screenshots", body });
  const { rows } = await h.pool.query("SELECT count(*)::int AS n FROM uploads WHERE post_id = $1", [th.postId]);
  assert.equal(rows[0].n, 4);
  assert.equal((await upload(client, csrf, await png())).status, 201);
  const fake = await upload(client, csrf, Buffer.from("not an image at all"), "image/png");
  assert.equal(fake.status, 422);
  assert.equal(fake.json.error, "type_mismatch");
  await assert.rejects(
    createThread(h.pool, { id: h.users.gracz }, { categoryId: await categoryId(h.pool, "general"), title: "Too many images", body: body + "\n![b](/forum/media/0123456789abcdef0123456789abcdef.webp)" }),
    /Up to 4 images per post/,
  );
});

test("served with nosniff and csp none", async (t) => {
  const h = await startForum(t);
  const { client, csrf } = await h.signedIn("gracz");
  const { json } = await upload(client, csrf, await png());
  const res = await fetch(`${h.appUrl}/forum/media/${json.id}.webp`);
  assert.equal(res.status, 200);
  assert.equal(res.headers.get("content-type"), "image/webp");
  assert.equal(res.headers.get("x-content-type-options"), "nosniff");
  assert.equal(res.headers.get("content-security-policy"), "default-src 'none'");
  assert.equal(res.headers.get("content-disposition"), 'inline; filename="image.webp"');
  const bytes = Buffer.from(await res.arrayBuffer());
  assert.equal(bytes.toString("latin1", 8, 12), "WEBP");
  assert.equal((await fetch(`${h.appUrl}/forum/media/0123456789abcdef0123456789abcdef.webp`)).status, 404);
  assert.equal((await fetch(`${h.appUrl}/forum/media/..%2F..%2Fetc%2Fpasswd`)).status, 404);
});

test("orphan cleanup after 24 h", async (t) => {
  const h = await startForum(t);
  const { client, csrf } = await h.signedIn("gracz");
  const old = (await upload(client, csrf, await png())).json.id;
  const fresh = (await upload(client, csrf, await png())).json.id;
  const attached = (await upload(client, csrf, await png())).json.id;
  await h.pool.query("UPDATE uploads SET created_at = now() - interval '25 hours' WHERE id = $1", [old]);
  const th = await createThread(h.pool, { id: h.users.gracz }, { categoryId: await categoryId(h.pool, "general"), title: "Removed later", body: `![x](/forum/media/${attached}.webp)` });
  assert.equal(await cleanupUploads(h.pool, h.uploadDir), 1);
  await h.pool.query("UPDATE posts SET deleted_at = now(), deleted_by = author_id WHERE id = $1", [th.postId]);
  assert.equal(await cleanupUploads(h.pool, h.uploadDir), 1);
  const { rows } = await h.pool.query("SELECT id FROM uploads ORDER BY id");
  assert.deepEqual(rows.map((r) => r.id), [fresh]);
  assert.equal(fs.existsSync(path.join(h.uploadDir, `${old}.webp`)), false);
  assert.equal(fs.existsSync(path.join(h.uploadDir, `${attached}.webp`)), false);
  assert.equal(fs.existsSync(path.join(h.uploadDir, `${fresh}.webp`)), true);
});

test("suspended user cannot upload", async (t) => {
  const h = await startForum(t);
  assert.equal((await h.anon().raw("/forum/api/uploads", await png(), "image/png")).status, 401);
  const { client, csrf } = await h.signedIn("inny");
  await h.pool.query("INSERT INTO suspensions (user_id, until, reason) VALUES ($1, now() + interval '1 day', 'spam')", [h.users.inny]);
  const res = await upload(client, csrf, await png());
  assert.equal(res.status, 403);
  assert.equal((await h.pool.query("SELECT count(*)::int AS n FROM uploads")).rows[0].n, 0);
  assert.equal((await upload(client, "", await png())).status, 403);
});
