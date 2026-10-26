const PERMISSIONS = [
  "thread.create", "post.create", "post.edit.own", "post.delete.own", "post.edit.any", "post.delete.any",
  "thread.pin", "thread.lock", "thread.move", "thread.delete.any", "report.view", "report.resolve",
  "user.suspend", "user.ban", "role.assign", "category.manage", "tag.manage", "audit.view", "upload.create", "announcement.create",
];

async function isSuspended(db, userId) {
  const { rowCount } = await db.query("SELECT 1 FROM suspensions WHERE user_id = $1 AND lifted_at IS NULL AND (until IS NULL OR until > now()) LIMIT 1", [userId]);
  return rowCount > 0;
}

async function permissionsFor(db, userId, categoryId = null) {
  if (!userId || (await isSuspended(db, userId))) return new Set();
  const { rows } = await db.query(
    `SELECT DISTINCT rp.permission
     FROM role_permissions rp
     JOIN roles r ON r.id = rp.role_id
     WHERE r.name = 'member'
        OR rp.role_id IN (SELECT role_id FROM user_roles WHERE user_id = $1 AND (category_id IS NULL OR category_id = $2))`,
    [userId, categoryId],
  );
  return new Set(rows.map((r) => r.permission));
}

const can = (perms, permission) => perms.has(permission);

async function promoteOwner(db, sub) {
  if (!/^\d{1,10}$/.test(String(sub ?? ""))) return false;
  const userId = Number(sub);
  const { rowCount } = await db.query(
    `INSERT INTO user_roles (user_id, role_id, category_id)
     SELECT p.id, r.id, NULL FROM aura.public_profiles p, roles r WHERE p.id = $1 AND r.name = 'administrator'
     ON CONFLICT DO NOTHING`,
    [userId],
  );
  if (!rowCount) return false;
  await db.query("INSERT INTO mod_log (actor_id, action, target_type, target_id, reason) VALUES (NULL, 'role.assign', 'user', $1, 'owner from FORUM_OWNER_SUB')", [userId]);
  return true;
}

module.exports = { permissionsFor, can, promoteOwner, isSuspended, PERMISSIONS };
