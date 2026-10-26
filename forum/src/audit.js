async function logAction(db, { actorId, action, targetType, targetId = null, reason = null, meta = {} }) {
  await db.query("INSERT INTO mod_log (actor_id, action, target_type, target_id, reason, meta) VALUES ($1, $2, $3, $4, $5, $6)", [actorId, action, targetType, targetId, reason, meta]);
}

module.exports = { logAction };
