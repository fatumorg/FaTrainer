const crypto = require("node:crypto");
const path = require("node:path");
const { Eta } = require("eta");

const PER_PAGE = 30;
const KINDS = { replies: "email_replies", mentions: "email_mentions" };
const mailViews = new Eta({ views: path.join(__dirname, "..", "views", "mail"), cache: true, autoEscape: true });

async function watchers(db, threadId) {
  const { rows } = await db.query(
    `SELECT s.user_id FROM subscriptions s WHERE s.thread_id = $1 AND NOT s.muted
     UNION
     SELECT s.user_id FROM subscriptions s JOIN threads t ON t.category_id = s.category_id
     WHERE t.id = $1 AND NOT s.muted
       AND NOT EXISTS (SELECT 1 FROM subscriptions m WHERE m.user_id = s.user_id AND m.thread_id = $1 AND m.muted)`,
    [threadId],
  );
  return rows.map((r) => r.user_id);
}

async function notifyOnPost(db, { id, threadId, authorId, mentions = [], isReply = true }) {
  const replyTo = isReply ? (await watchers(db, threadId)).filter((u) => u !== authorId) : [];
  let mentioned = [];
  if (mentions.length) {
    const { rows } = await db.query("SELECT id FROM aura.public_profiles WHERE discoverable AND username = ANY($1)", [mentions]);
    mentioned = rows.map((r) => r.id).filter((u) => u !== authorId && !replyTo.includes(u));
  }
  const insert = (users, kind) =>
    users.length && db.query("INSERT INTO notifications (user_id, kind, thread_id, post_id, actor_id) SELECT u, $2, $3, $4, $5 FROM unnest($1::int[]) u", [users, kind, threadId, id, authorId]);
  await insert(replyTo, "reply");
  await insert(mentioned, "mention");
}

async function notify(db, { userId, kind, threadId = null, postId = null, actorId = null, reportId = null }) {
  if (!userId || userId === actorId) return;
  await db.query("INSERT INTO notifications (user_id, kind, thread_id, post_id, actor_id, report_id) VALUES ($1, $2, $3, $4, $5, $6)", [userId, kind, threadId, postId, actorId, reportId]);
}

const notifyModeration = (db, decision) => notify(db, { kind: "moderation", ...decision });

function describe(n) {
  const who = n.actor || "Someone";
  const where = n.title ? `"${n.title}"` : "a thread";
  return {
    reply: `${who} replied in ${where}`,
    mention: `${who} mentioned you in ${where}`,
    accepted: `Your answer in ${where} was accepted`,
    moderation: `A moderator acted on your content in ${where}. Read the statement of reasons.`,
    report_outcome: "Your report was decided",
    appeal_outcome: "Your appeal was decided",
  }[n.kind];
}

function target(n) {
  if (["moderation", "report_outcome", "appeal_outcome"].includes(n.kind) && n.report_id) return `/forum/appeal/${n.report_id}`;
  if (n.thread_id && n.slug) return `/forum/t/${n.thread_id}/${n.slug}${n.post_id ? `?post=${n.post_id}#p${n.post_id}` : ""}`;
  return "/forum";
}

const SELECT = `SELECT n.*, t.title, t.slug, a.username AS actor
  FROM notifications n LEFT JOIN threads t ON t.id = n.thread_id LEFT JOIN aura.public_profiles a ON a.id = n.actor_id`;

async function listNotifications(db, userId, page = 1) {
  const total = (await db.query("SELECT count(*)::int AS n FROM notifications WHERE user_id = $1", [userId])).rows[0].n;
  const pages = Math.max(1, Math.ceil(total / PER_PAGE));
  const current = Math.min(Math.max(1, Number(page) || 1), pages);
  const { rows } = await db.query(`${SELECT} WHERE n.user_id = $1 ORDER BY n.id DESC LIMIT ${PER_PAGE} OFFSET ${(current - 1) * PER_PAGE}`, [userId]);
  return { items: rows.map((n) => ({ ...n, text: describe(n) })), page: current, pages, total };
}

async function openNotification(db, userId, id) {
  const { rows } = await db.query(`${SELECT} WHERE n.id = $1 AND n.user_id = $2`, [Number(id) || 0, userId]);
  if (!rows[0]) return null;
  await db.query("UPDATE notifications SET read_at = now() WHERE id = $1 AND read_at IS NULL", [rows[0].id]);
  return target(rows[0]);
}

const unreadCount = async (db, userId) => (await db.query("SELECT count(*)::int AS n FROM notifications WHERE user_id = $1 AND read_at IS NULL", [userId])).rows[0].n;

const sign = (secret, payload) => crypto.createHmac("sha256", secret).update(`unsubscribe:${payload}`).digest("base64url");

function unsubscribeToken(secret, userId, kind) {
  const payload = `${userId}.${kind}`;
  return `${payload}.${sign(secret, payload)}`;
}

function readToken(secret, token) {
  const m = /^(\d{1,10})\.(replies|mentions|all)\.([A-Za-z0-9_-]{43})$/.exec(String(token));
  if (!m) return null;
  const expected = Buffer.from(sign(secret, `${m[1]}.${m[2]}`));
  const given = Buffer.from(m[3]);
  if (expected.length !== given.length || !crypto.timingSafeEqual(expected, given)) return null;
  return { userId: Number(m[1]), kind: m[2] };
}

async function unsubscribe(db, { userId, kind }) {
  const cols = kind === "all" ? Object.values(KINDS) : [KINDS[kind]];
  await db.query(
    `INSERT INTO forum_prefs (user_id, ${cols.join(", ")}) VALUES ($1, ${cols.map(() => "false").join(", ")})
     ON CONFLICT (user_id) DO UPDATE SET ${cols.map((c) => `${c} = false`).join(", ")}`,
    [userId],
  );
}

async function runMailBatch(db, mailer, { appUrl, secret }) {
  const { rows } = await db.query(
    `${SELECT.replace("SELECT n.*", "SELECT n.*, COALESCE(f.email_replies, true) AS email_replies, COALESCE(f.email_mentions, true) AS email_mentions")}
     LEFT JOIN forum_prefs f ON f.user_id = n.user_id
     WHERE n.emailed_at IS NULL ORDER BY n.user_id, n.id`,
  );
  const byUser = new Map();
  for (const n of rows) {
    const wanted = !n.read_at && (n.kind === "reply" || n.kind === "accepted" ? n.email_replies : n.kind === "mention" ? n.email_mentions : true);
    if (!byUser.has(n.user_id)) byUser.set(n.user_id, { send: [], skip: [] });
    byUser.get(n.user_id)[wanted ? "send" : "skip"].push(n);
  }
  let mails = 0;
  for (const [userId, { send, skip }] of byUser) {
    const done = skip.map((n) => n.id);
    const to = send.length ? (await db.query("SELECT fatrainer.recipient_email($1) AS e", [userId])).rows[0].e : null;
    if (to) {
      const items = send.map((n) => ({ text: describe(n), url: appUrl + target(n) }));
      const links = {
        all: `${appUrl}/forum/unsubscribe/${unsubscribeToken(secret, userId, "all")}`,
        replies: `${appUrl}/forum/unsubscribe/${unsubscribeToken(secret, userId, "replies")}`,
        mentions: `${appUrl}/forum/unsubscribe/${unsubscribeToken(secret, userId, "mentions")}`,
        settings: `${appUrl}/forum/settings`,
      };
      const subject = items.length === 1 ? `FaTrainer Forum: ${items[0].text}` : `FaTrainer Forum: ${items.length} new notifications`;
      const text = [
        `${items.length} new on the FaTrainer forum:`, "",
        ...items.map((i) => `- ${i.text}\n  ${i.url}`), "",
        `Change e-mail settings: ${links.settings}`,
        `Stop e-mails about replies: ${links.replies}`,
        `Stop e-mails about mentions: ${links.mentions}`,
      ].join("\n");
      try {
        await mailer.send({
          to,
          subject,
          text,
          html: mailViews.render("digest", { items, links }),
          headers: { "List-Unsubscribe": `<${links.all}>`, "List-Unsubscribe-Post": "List-Unsubscribe=One-Click", "Auto-Submitted": "auto-generated" },
        });
        mails++;
        done.push(...send.map((n) => n.id));
      } catch (err) {
        console.error("forum mail failed:", err?.code || "send_error");
      }
    } else {
      done.push(...send.map((n) => n.id));
    }
    if (done.length) await db.query("UPDATE notifications SET emailed_at = now() WHERE id = ANY($1)", [done]);
  }
  return mails;
}

module.exports = { notifyOnPost, notify, notifyModeration, listNotifications, openNotification, unreadCount, unsubscribeToken, readToken, unsubscribe, runMailBatch, watchers };
