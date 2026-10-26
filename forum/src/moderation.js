const { permissionsFor } = require("./permissions.js");
const { ForumError, tx, renderPost } = require("./compose.js");
const { RENDERER_VERSION } = require("./markdown.js");
const { logAction } = require("./audit.js");
const { notify, notifyOnPost } = require("./notifications.js");

const RULES = {
  topic: "Stay on topic: FaTrainer, in English or Polish",
  illegal: "Nothing illegal",
  harassment: "No hate, threats or harassment, in the forum or in the game",
  personal_data: "No other people's personal data",
  files: "No game files, cracks, pirated copies, malware, or other people's mods and code without permission",
  spam: "No spam or advertising",
};
const REASONS = { illegal: "Illegal content", harassment: "Harassment", spam: "Spam", personal_data: "Personal data", off_topic: "Off-topic", other: "Other" };
const ACTIONS = ["dismiss", "remove", "edit", "lock", "suspend", "ban"];
const APPEAL_MONTHS = 6;

async function need(db, user, permission, categoryId = null) {
  const perms = await permissionsFor(db, user.id, categoryId);
  if (!perms.has(permission)) throw new ForumError(403, "You do not have permission to do that.");
  return perms;
}

function checkReason(reason) {
  const r = String(reason ?? "").trim();
  if (r.length < 10 || r.length > 1000) throw new ForumError(422, "Write a statement of reasons of 10 to 1000 characters.", "reason");
  return r;
}

function checkRule(rule) {
  if (!RULES[rule]) throw new ForumError(422, "Choose the rule that was broken.", "rule");
  return rule;
}

async function targetOf(db, type, id) {
  const sql = type === "post"
    ? "SELECT p.id, p.author_id, p.thread_id, t.category_id, p.body_md AS excerpt, t.title FROM posts p JOIN threads t ON t.id = p.thread_id WHERE p.id = $1"
    : "SELECT t.id, t.author_id, t.id AS thread_id, t.category_id, t.title AS excerpt, t.title FROM threads t WHERE t.id = $1";
  return (await db.query(sql, [Number(id) || 0])).rows[0] || null;
}

async function fileReport(db, reporter, { targetType, targetId, reason, details = "" }) {
  if (!["post", "thread"].includes(targetType)) throw new ForumError(422, "Choose what to report.", "target");
  if (!REASONS[reason]) throw new ForumError(422, "Choose a reason.", "reason");
  const text = String(details ?? "").trim();
  if (text.length > 2000) throw new ForumError(422, `Details are too long (${text.length}/2000 characters).`, "details");
  if (!(await targetOf(db, targetType, targetId))) throw new ForumError(404, "This content does not exist.");
  const open = await db.query("SELECT id FROM reports WHERE target_type = $1 AND target_id = $2 AND reporter_id = $3 AND status = 'open'", [targetType, Number(targetId), reporter.id]);
  if (open.rows[0]) return { id: open.rows[0].id };
  const { rows } = await db.query("INSERT INTO reports (target_type, target_id, reporter_id, reason, details) VALUES ($1, $2, $3, $4, $5) RETURNING id", [targetType, Number(targetId), reporter.id, reason, text]);
  return { id: rows[0].id };
}

async function apply(db, mod, report, target, { action, rule, reason, until, body }) {
  const isThread = report.target_type === "thread";
  if (action === "remove") {
    await db.query(`UPDATE ${isThread ? "threads" : "posts"} SET deleted_at = now(), deleted_by = $2 WHERE id = $1 AND deleted_at IS NULL`, [target.id, mod.id]);
  } else if (action === "lock") {
    await db.query("UPDATE threads SET locked = true WHERE id = $1", [target.thread_id]);
  } else if (action === "edit") {
    if (isThread) throw new ForumError(422, "Edit a thread's first post instead.");
    const md = String(body ?? "");
    if (!md.trim()) throw new ForumError(422, "Give the edited text.", "body");
    const rendered = await renderPost(db, md);
    await db.query("INSERT INTO post_revisions (post_id, body_md, edited_by) SELECT id, body_md, $2 FROM posts WHERE id = $1", [target.id, mod.id]);
    await db.query("UPDATE posts SET body_md = $2, body_html = $3, renderer_version = $4, edited_at = now() WHERE id = $1", [target.id, md, rendered.html, RENDERER_VERSION]);
  } else if (action === "suspend" || action === "ban") {
    if (!target.author_id) throw new ForumError(422, "The author's account no longer exists.");
    await db.query("INSERT INTO suspensions (user_id, until, reason, by_id) VALUES ($1, $2, $3, $4)", [target.author_id, action === "ban" ? null : until, `${RULES[rule]}: ${reason}`, mod.id]);
  }
}

async function decide(pool, mod, reportId, input) {
  const action = ACTIONS.includes(input.action) ? input.action : null;
  if (!action) throw new ForumError(422, "Choose an action.", "action");
  const reason = checkReason(input.reason);
  const rule = action === "dismiss" ? null : checkRule(input.rule);
  const report = (await pool.query("SELECT * FROM reports WHERE id = $1", [Number(reportId) || 0])).rows[0];
  if (!report) throw new ForumError(404, "This report does not exist.");
  const target = await targetOf(pool, report.target_type, report.target_id);
  await need(pool, mod, "report.resolve", target?.category_id);
  if (action === "suspend") await need(pool, mod, "user.suspend");
  if (action === "ban") await need(pool, mod, "user.ban");
  let until = null;
  if (action === "suspend") {
    until = new Date(String(input.until || ""));
    if (Number.isNaN(until.getTime()) || until <= new Date()) throw new ForumError(422, "Choose a future end date for the suspension.", "until");
  }
  return tx(pool, async (db) => {
    const claimed = await db.query(
      "UPDATE reports SET status = 'decided', decided_by = $2, decision = $3, statement = $4, decided_at = now() WHERE id = $1 AND status = 'open' RETURNING id",
      [report.id, mod.id, rule ? `${action}:${rule}` : action, reason],
    );
    if (!claimed.rowCount) return "already_decided";
    if (target && action !== "dismiss") await apply(db, mod, report, target, { action, rule, reason, until, body: input.body });
    await logAction(db, { actorId: mod.id, action: `report.${action}`, targetType: report.target_type, targetId: report.target_id, reason, meta: { report: report.id, rule, before: target?.excerpt?.slice(0, 200) ?? null } });
    if (target && action !== "dismiss") await notify(db, { userId: target.author_id, kind: "moderation", threadId: target.thread_id, postId: report.target_type === "post" ? target.id : null, actorId: mod.id, reportId: report.id });
    if (report.reporter_id) await notify(db, { userId: report.reporter_id, kind: "report_outcome", threadId: target?.thread_id ?? null, actorId: mod.id, reportId: report.id });
    return "ok";
  });
}

async function moderate(pool, mod, { targetType, targetId, action, rule, reason, until, body }) {
  const { id } = await fileReport(pool, mod, { targetType, targetId, reason: "other", details: "Moderator action" });
  const result = await decide(pool, mod, id, { action, rule, reason, until, body });
  return { result, reportId: id };
}

async function reportView(db, user, reportId) {
  const report = (await db.query("SELECT * FROM reports WHERE id = $1 AND status = 'decided'", [Number(reportId) || 0])).rows[0];
  if (!report) return null;
  const target = await targetOf(db, report.target_type, report.target_id);
  const isAuthor = Boolean(target && target.author_id === user.id);
  const isReporter = report.reporter_id === user.id && !isAuthor;
  if (!isAuthor && !isReporter) return null;
  const [action, rule] = String(report.decision).split(":");
  const appeal = (await db.query("SELECT * FROM appeals WHERE report_id = $1 ORDER BY id DESC LIMIT 1", [report.id])).rows[0] || null;
  const closes = new Date(report.decided_at);
  closes.setMonth(closes.getMonth() + APPEAL_MONTHS);
  return { report, target, action, rule: RULES[rule] || null, isAuthor, appeal, canAppeal: isAuthor && action !== "dismiss" && !(appeal && appeal.status === "open") && closes > new Date(), closes };
}

async function fileAppeal(db, user, reportId, text) {
  const view = await reportView(db, user, reportId);
  if (!view || !view.isAuthor || view.action === "dismiss") return { error: "not_found" };
  if (view.appeal && view.appeal.status === "open") return { error: "already_open" };
  if (view.closes <= new Date()) return { error: "window_closed" };
  const t = String(text ?? "").trim();
  if (t.length < 1 || t.length > 2000) throw new ForumError(422, "Write your appeal in 1 to 2000 characters.", "text");
  const { rows } = await db.query("INSERT INTO appeals (report_id, user_id, text) VALUES ($1, $2, $3) RETURNING id", [view.report.id, user.id, t]);
  return { id: rows[0].id };
}

async function undo(db, report, target) {
  const [action] = String(report.decision).split(":");
  if (!target) return;
  if (action === "remove") await db.query(`UPDATE ${report.target_type === "thread" ? "threads" : "posts"} SET deleted_at = NULL, deleted_by = NULL WHERE id = $1`, [target.id]);
  if (action === "lock") await db.query("UPDATE threads SET locked = false WHERE id = $1", [target.thread_id]);
  if (action === "suspend" || action === "ban") await db.query("UPDATE suspensions SET lifted_at = now() WHERE user_id = $1 AND lifted_at IS NULL AND by_id = $2", [target.author_id, report.decided_by]);
  if (action === "edit") {
    const rev = (await db.query("SELECT body_md FROM post_revisions WHERE post_id = $1 AND edited_by = $2 ORDER BY id DESC LIMIT 1", [target.id, report.decided_by])).rows[0];
    if (rev) {
      const rendered = await renderPost(db, rev.body_md);
      await db.query("UPDATE posts SET body_md = $2, body_html = $3, edited_at = now() WHERE id = $1", [target.id, rev.body_md, rendered.html]);
    }
  }
}

async function otherModeratorExists(db, deciderId) {
  const { rows } = await db.query(
    `SELECT DISTINCT ur.user_id FROM user_roles ur JOIN role_permissions rp ON rp.role_id = ur.role_id
     JOIN aura.public_profiles p ON p.id = ur.user_id
     WHERE rp.permission = 'report.resolve' AND ur.user_id <> $1
       AND NOT EXISTS (SELECT 1 FROM suspensions s WHERE s.user_id = ur.user_id AND s.lifted_at IS NULL AND (s.until IS NULL OR s.until > now()))`,
    [deciderId],
  );
  return rows.length > 0;
}

async function decideAppeal(pool, mod, appealId, outcome) {
  if (!["upheld", "reversed"].includes(outcome)) throw new ForumError(422, "Choose upheld or reversed.", "outcome");
  const appeal = (await pool.query("SELECT a.*, r.decided_by, r.target_type, r.target_id, r.decision FROM appeals a JOIN reports r ON r.id = a.report_id WHERE a.id = $1", [Number(appealId) || 0])).rows[0];
  if (!appeal) throw new ForumError(404, "This appeal does not exist.");
  const target = await targetOf(pool, appeal.target_type, appeal.target_id);
  await need(pool, mod, "report.resolve", target?.category_id);
  if (appeal.decided_by === mod.id && (await otherModeratorExists(pool, mod.id))) throw new ForumError(403, "A different moderator has to review this appeal.");
  return tx(pool, async (db) => {
    const claimed = await db.query("UPDATE appeals SET status = 'decided', decided_by = $2, decided_at = now(), outcome = $3 WHERE id = $1 AND status = 'open' RETURNING id", [appeal.id, mod.id, outcome]);
    if (!claimed.rowCount) return "already_decided";
    if (outcome === "reversed") await undo(db, { target_type: appeal.target_type, decision: appeal.decision, decided_by: appeal.decided_by }, target);
    await logAction(db, { actorId: mod.id, action: `appeal.${outcome}`, targetType: appeal.target_type, targetId: appeal.target_id, meta: { appeal: appeal.id, report: appeal.report_id } });
    await notify(db, { userId: appeal.user_id, kind: "appeal_outcome", threadId: target?.thread_id ?? null, actorId: mod.id, reportId: appeal.report_id });
    return "ok";
  });
}

async function threadAction(pool, mod, threadId, action, { category, reason, rule } = {}) {
  const t = (await pool.query("SELECT id, category_id, deleted_at FROM threads WHERE id = $1", [Number(threadId) || 0])).rows[0];
  if (!t || t.deleted_at) throw new ForumError(404, "This thread does not exist.");
  const perm = { pin: "thread.pin", unpin: "thread.pin", lock: "thread.lock", unlock: "thread.lock", move: "thread.move", delete: "thread.delete.any" }[action];
  if (!perm) throw new ForumError(404, "Unknown action.");
  await need(pool, mod, perm, t.category_id);
  if (action === "delete") {
    const { result } = await moderate(pool, mod, { targetType: "thread", targetId: t.id, action: "remove", rule, reason });
    if (result !== "ok") throw new ForumError(409, "Someone else already decided this.");
    return t;
  }
  let meta = {};
  if (action === "move") {
    const to = (await pool.query("SELECT id FROM categories WHERE slug = $1", [String(category || "")])).rows[0];
    if (!to) throw new ForumError(422, "Choose a category.", "category");
    await pool.query("UPDATE threads SET category_id = $2 WHERE id = $1", [t.id, to.id]);
    meta = { from: t.category_id, to: to.id };
  } else {
    const col = action.endsWith("pin") ? "pinned" : "locked";
    await pool.query(`UPDATE threads SET ${col} = $2 WHERE id = $1`, [t.id, !action.startsWith("un")]);
  }
  await logAction(pool, { actorId: mod.id, action: `thread.${action}`, targetType: "thread", targetId: t.id, reason: String(reason ?? "").trim() || null, meta });
  return t;
}

async function suspendUser(pool, mod, userId, { until, reason, rule }) {
  const ban = !until;
  await need(pool, mod, ban ? "user.ban" : "user.suspend");
  const why = checkReason(reason);
  checkRule(rule);
  let end = null;
  if (!ban) {
    end = new Date(String(until));
    if (Number.isNaN(end.getTime()) || end <= new Date()) throw new ForumError(422, "Choose a future end date for the suspension.", "until");
  }
  const target = (await pool.query("SELECT id FROM aura.public_profiles WHERE id = $1", [Number(userId) || 0])).rows[0];
  if (!target) throw new ForumError(404, "This account does not exist.");
  if (target.id === mod.id) throw new ForumError(422, "You cannot suspend yourself.");
  await tx(pool, async (db) => {
    await db.query("INSERT INTO suspensions (user_id, until, reason, by_id) VALUES ($1, $2, $3, $4)", [target.id, end, `${RULES[rule]}: ${why}`, mod.id]);
    await logAction(db, { actorId: mod.id, action: ban ? "user.ban" : "user.suspend", targetType: "user", targetId: target.id, reason: why, meta: { rule, until: end } });
    await notify(db, { userId: target.id, kind: "moderation", actorId: mod.id });
  });
}

async function approvePost(pool, mod, postId) {
  const post = (await pool.query(
    `SELECT p.id, p.thread_id, p.author_id, p.body_md, t.category_id, p.id = (SELECT min(id) FROM posts WHERE thread_id = p.thread_id) AS first
     FROM posts p JOIN threads t ON t.id = p.thread_id WHERE p.id = $1 AND p.held AND p.deleted_at IS NULL`,
    [Number(postId) || 0],
  )).rows[0];
  if (!post) throw new ForumError(404, "This held post does not exist.");
  await need(pool, mod, "report.resolve", post.category_id);
  const rendered = await renderPost(pool, post.body_md);
  await tx(pool, async (db) => {
    await db.query("UPDATE posts SET held = false WHERE id = $1", [post.id]);
    if (!post.first) await db.query("UPDATE threads SET last_post_at = now(), reply_count = reply_count + 1 WHERE id = $1", [post.thread_id]);
    else await db.query("UPDATE threads SET last_post_at = now() WHERE id = $1", [post.thread_id]);
    await notifyOnPost(db, { id: post.id, threadId: post.thread_id, authorId: post.author_id, mentions: rendered.mentions, isReply: !post.first });
    await logAction(db, { actorId: mod.id, action: "post.approve", targetType: "post", targetId: post.id });
  });
  return post;
}

async function queue(db, mod) {
  await need(db, mod, "report.view");
  const reports = await db.query(
    `SELECT r.*, rp.username AS reporter,
            CASE r.target_type WHEN 'post' THEN (SELECT body_md FROM posts WHERE id = r.target_id) ELSE (SELECT title FROM threads WHERE id = r.target_id) END AS content,
            CASE r.target_type WHEN 'post' THEN (SELECT thread_id FROM posts WHERE id = r.target_id) ELSE r.target_id END AS thread_id
     FROM reports r LEFT JOIN aura.public_profiles rp ON rp.id = r.reporter_id
     WHERE r.status = 'open' ORDER BY r.created_at, r.id LIMIT 100`,
  );
  const held = await db.query(
    `SELECT p.id, p.body_md, p.created_at, p.thread_id, t.title, a.username AS author
     FROM posts p JOIN threads t ON t.id = p.thread_id LEFT JOIN aura.public_profiles a ON a.id = p.author_id
     WHERE p.held AND p.deleted_at IS NULL ORDER BY p.created_at, p.id LIMIT 100`,
  );
  const appeals = await db.query(
    `SELECT a.*, r.statement, r.decision, r.decided_by, r.target_type, r.target_id, u.username AS appellant
     FROM appeals a JOIN reports r ON r.id = a.report_id LEFT JOIN aura.public_profiles u ON u.id = a.user_id
     WHERE a.status = 'open' ORDER BY a.created_at, a.id LIMIT 100`,
  );
  return { reports: reports.rows, held: held.rows, appeals: appeals.rows };
}

async function modLog(db, mod, page = 1) {
  await need(db, mod, "audit.view");
  const per = 50;
  const total = (await db.query("SELECT count(*)::int AS n FROM mod_log")).rows[0].n;
  const pages = Math.max(1, Math.ceil(total / per));
  const current = Math.min(Math.max(1, Number(page) || 1), pages);
  const { rows } = await db.query(
    `SELECT l.*, a.username AS actor FROM mod_log l LEFT JOIN aura.public_profiles a ON a.id = l.actor_id ORDER BY l.id DESC LIMIT ${per} OFFSET ${(current - 1) * per}`,
  );
  return { items: rows, page: current, pages };
}

module.exports = { RULES, REASONS, ACTIONS, approvePost, fileReport, decide, moderate, reportView, fileAppeal, decideAppeal, threadAction, suspendUser, queue, modLog };
