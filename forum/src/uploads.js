const fs = require("node:fs/promises");
const path = require("node:path");
const { processImage } = require("fatum-kit/images");
const { permissionsFor } = require("./permissions.js");

const MAX_PENDING = 4;
const MAX_PER_POST = 4;
const MEDIA_REF = /\/forum\/media\/([a-f0-9]{32})\.webp/g;

class UploadError extends Error {
  constructor(status, code, message) {
    super(message);
    this.status = status;
    this.code = code;
  }
}

const fileOf = (dir, id) => path.join(dir, `${id}.webp`);
const mediaIds = (md) => [...new Set([...String(md).matchAll(MEDIA_REF)].map((m) => m[1]))];

async function saveUpload(pool, uploadDir, user, buffer, type) {
  const perms = await permissionsFor(pool, user.id);
  if (!perms.has("upload.create")) throw new UploadError(403, "FORBIDDEN", "You cannot upload images right now.");
  const pending = (await pool.query("SELECT count(*)::int AS n FROM uploads WHERE user_id = $1 AND post_id IS NULL", [user.id])).rows[0].n;
  if (pending >= MAX_PENDING) throw new UploadError(429, "UPLOAD_LIMIT", `You can attach up to ${MAX_PER_POST} images to one post. Post it first, then add more.`);
  const { id, variants } = await processImage(buffer, type, { maxBytes: 5_000_000, sizes: [2560] });
  const out = variants[0].buffer;
  await fs.mkdir(uploadDir, { recursive: true, mode: 0o700 });
  await fs.writeFile(fileOf(uploadDir, id), out, { mode: 0o600, flag: "wx" });
  await pool.query("INSERT INTO uploads (id, user_id, bytes) VALUES ($1, $2, $3)", [id, user.id, out.length]);
  return { id, url: `/forum/media/${id}.webp`, markdown: `![image](/forum/media/${id}.webp)` };
}

function checkImageCount(md) {
  return mediaIds(md).length <= MAX_PER_POST;
}

async function attachUploads(db, userId, postId, md) {
  const ids = mediaIds(md);
  if (ids.length) await db.query("UPDATE uploads SET post_id = $1 WHERE user_id = $2 AND post_id IS NULL AND id = ANY($3)", [postId, userId, ids]);
}

async function cleanupUploads(pool, uploadDir) {
  const { rows } = await pool.query(
    `DELETE FROM uploads u
     WHERE (u.post_id IS NULL AND u.created_at < now() - interval '24 hours')
        OR EXISTS (SELECT 1 FROM posts p WHERE p.id = u.post_id AND p.deleted_at IS NOT NULL)
     RETURNING u.id`,
  );
  for (const { id } of rows) await fs.rm(fileOf(uploadDir, id), { force: true });
  return rows.length;
}

async function mediaFile(pool, uploadDir, name) {
  const m = /^([a-f0-9]{32})\.webp$/.exec(name);
  if (!m) return null;
  const { rowCount } = await pool.query("SELECT 1 FROM uploads WHERE id = $1", [m[1]]);
  return rowCount ? fileOf(uploadDir, m[1]) : null;
}

module.exports = { saveUpload, attachUploads, checkImageCount, cleanupUploads, mediaFile, mediaIds, UploadError, MAX_PER_POST };
