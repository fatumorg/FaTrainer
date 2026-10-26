const crypto = require("node:crypto");
const { permissionsFor } = require("./permissions.js");

const NEW_DAYS = 7;
const NEW_MIN_POSTS = 3;
const NEW_THREADS_PER_DAY = 3;
const NEW_POSTS_PER_DAY = 20;
const POSTS_PER_HOUR = 60;
const HOLD_LINKS = 3;

const refuse = (reason, message) => ({ ok: false, reason, message });
const countLinks = (body) => (String(body).match(/https?:\/\/[^\s)<>]+/gi) || []).length;

async function checkPostAllowed(db, user, { body, threadId = null }) {
  const perms = await permissionsFor(db, user.id);
  if (!perms.has("post.create")) return refuse("suspended", "Your account cannot post right now. You can still read.");
  const trusted = perms.has("report.view");
  const { rows } = await db.query(
    `SELECT
       COALESCE((SELECT joined_at FROM forum_prefs WHERE user_id = $1), now()) > now() - make_interval(days => ${NEW_DAYS}) AS recent,
       (SELECT count(*)::int FROM posts WHERE author_id = $1 AND NOT held AND deleted_at IS NULL) AS visible,
       (SELECT count(*)::int FROM threads WHERE author_id = $1 AND created_at > now() - interval '1 day') AS threads_day,
       (SELECT count(*)::int FROM posts WHERE author_id = $1 AND created_at > now() - interval '1 day') AS posts_day,
       (SELECT count(*)::int FROM posts WHERE author_id = $1 AND created_at > now() - interval '1 hour') AS posts_hour,
       EXISTS (SELECT 1 FROM posts WHERE author_id = $1 AND created_at > now() - interval '10 minutes' AND btrim(body_md) = btrim($2)) AS duplicate`,
    [user.id, String(body)],
  );
  const s = rows[0];
  if (s.duplicate) return refuse("duplicate", "You already posted this in the last 10 minutes.");
  if (!trusted && s.posts_hour >= POSTS_PER_HOUR) return refuse("rate_limited", "You are posting very fast. Wait a while and try again.");
  const isNew = !trusted && (s.recent || s.visible < NEW_MIN_POSTS);
  if (isNew && !threadId && s.threads_day >= NEW_THREADS_PER_DAY) return refuse("rate_limited", `New members can start ${NEW_THREADS_PER_DAY} threads a day. Try again tomorrow.`);
  if (isNew && s.posts_day >= NEW_POSTS_PER_DAY) return refuse("rate_limited", `New members can post ${NEW_POSTS_PER_DAY} times a day. Try again tomorrow.`);
  return { ok: true, hold: isNew && countLinks(body) >= HOLD_LINKS };
}

const sign = (secret, value) => crypto.createHmac("sha256", secret).update(`compose:${value}`).digest("base64url").slice(0, 22);

function composeStamp(secret, now = Date.now()) {
  const t = now.toString(36);
  return `${t}.${sign(secret, t)}`;
}

function stampAge(secret, stamp, now = Date.now()) {
  const m = /^([0-9a-z]{1,12})\.([A-Za-z0-9_-]{22})$/.exec(String(stamp || ""));
  if (!m) return -1;
  const expected = Buffer.from(sign(secret, m[1]));
  if (!crypto.timingSafeEqual(expected, Buffer.from(m[2]))) return -1;
  return now - parseInt(m[1], 36);
}

module.exports = { checkPostAllowed, composeStamp, stampAge, countLinks };
