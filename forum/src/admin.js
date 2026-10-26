const { permissionsFor, PERMISSIONS } = require("./permissions.js");
const { ForumError, tx } = require("./compose.js");
const { logAction } = require("./audit.js");

const SLUG = /^[a-z0-9]+(-[a-z0-9]+)*$/;

async function need(db, user, permission) {
  if (!(await permissionsFor(db, user.id)).has(permission)) throw new ForumError(403, "You do not have permission to do that.");
}

function checkSlug(slug, max = 60) {
  const s = String(slug ?? "").trim();
  if (!SLUG.test(s) || s.length > max) throw new ForumError(422, "Use a slug of lowercase letters, digits and single dashes.", "slug");
  return s;
}

function checkName(name, max) {
  const n = String(name ?? "").trim();
  if (!n || n.length > max) throw new ForumError(422, `Name needs 1 to ${max} characters.`, "name");
  return n;
}

async function saveCategory(pool, admin, { id = null, slug, name, description = "", position = 0, parentId = null, postPermission = "post.create" }) {
  await need(pool, admin, "category.manage");
  const fields = { slug: checkSlug(slug), name: checkName(name, 60), description: String(description ?? "").trim() };
  if (fields.description.length > 300) throw new ForumError(422, "Description is too long (300 characters at most).", "description");
  if (!PERMISSIONS.includes(postPermission)) throw new ForumError(422, "Choose a posting permission from the list.", "post_permission");
  const parent = parentId ? Number(parentId) : null;
  if (parent) {
    const p = (await pool.query("SELECT parent_id FROM categories WHERE id = $1", [parent])).rows[0];
    const hasChildren = id ? (await pool.query("SELECT 1 FROM categories WHERE parent_id = $1", [id])).rowCount > 0 : false;
    if (!p || p.parent_id || parent === Number(id) || hasChildren) throw new ForumError(422, "Subcategories go one level deep.", "parent");
  }
  const args = [fields.slug, fields.name, fields.description, Number(position) || 0, parent, postPermission];
  try {
    return await tx(pool, async (db) => {
      const { rows } = id
        ? await db.query("UPDATE categories SET slug = $1, name = $2, description = $3, position = $4, parent_id = $5, post_permission = $6 WHERE id = $7 RETURNING id", [...args, Number(id)])
        : await db.query("INSERT INTO categories (slug, name, description, position, parent_id, post_permission) VALUES ($1, $2, $3, $4, $5, $6) RETURNING id", args);
      if (!rows[0]) throw new ForumError(404, "This category does not exist.");
      await logAction(db, { actorId: admin.id, action: id ? "category.update" : "category.create", targetType: "category", targetId: rows[0].id, meta: { slug: fields.slug, name: fields.name, parent } });
      return { id: rows[0].id };
    });
  } catch (err) {
    if (err.code === "23505") throw new ForumError(422, "That slug is already used.", "slug");
    throw err;
  }
}

async function deleteCategory(pool, admin, id) {
  await need(pool, admin, "category.manage");
  const { rows } = await pool.query("SELECT (SELECT count(*) FROM threads WHERE category_id = $1)::int + (SELECT count(*) FROM categories WHERE parent_id = $1)::int AS n", [Number(id) || 0]);
  if (rows[0].n > 0) throw new ForumError(422, "This category is not empty. Move its threads and subcategories first.");
  await tx(pool, async (db) => {
    const del = await db.query("DELETE FROM categories WHERE id = $1 RETURNING slug", [Number(id) || 0]);
    if (!del.rowCount) throw new ForumError(404, "This category does not exist.");
    await logAction(db, { actorId: admin.id, action: "category.delete", targetType: "category", targetId: Number(id), meta: { slug: del.rows[0].slug } });
  });
}

async function saveTag(pool, admin, { id = null, slug, name }) {
  await need(pool, admin, "tag.manage");
  const args = [checkSlug(slug, 40), checkName(name, 40)];
  try {
    return await tx(pool, async (db) => {
      const { rows } = id
        ? await db.query("UPDATE tags SET slug = $1, name = $2 WHERE id = $3 RETURNING id", [...args, Number(id)])
        : await db.query("INSERT INTO tags (slug, name) VALUES ($1, $2) RETURNING id", args);
      if (!rows[0]) throw new ForumError(404, "This tag does not exist.");
      await logAction(db, { actorId: admin.id, action: id ? "tag.update" : "tag.create", targetType: "tag", targetId: rows[0].id, meta: { slug: args[0], name: args[1] } });
      return { id: rows[0].id };
    });
  } catch (err) {
    if (err.code === "23505") throw new ForumError(422, "That slug is already used.", "slug");
    throw err;
  }
}

async function deleteTag(pool, admin, id) {
  await need(pool, admin, "tag.manage");
  await tx(pool, async (db) => {
    const del = await db.query("DELETE FROM tags WHERE id = $1 RETURNING slug", [Number(id) || 0]);
    if (!del.rowCount) throw new ForumError(404, "This tag does not exist.");
    await logAction(db, { actorId: admin.id, action: "tag.delete", targetType: "tag", targetId: Number(id), meta: { slug: del.rows[0].slug } });
  });
}

async function setRolePermissions(pool, admin, roleName, perms) {
  await need(pool, admin, "role.assign");
  if (roleName === "administrator") throw new ForumError(422, "The administrator role keeps every permission, so role.assign can never be lost.");
  const list = [...new Set(Array.isArray(perms) ? perms : perms ? [perms] : [])];
  const unknown = list.find((p) => !PERMISSIONS.includes(p));
  if (unknown) throw new ForumError(422, `Unknown permission: ${unknown}`);
  const role = (await pool.query("SELECT id FROM roles WHERE name = $1", [roleName])).rows[0];
  if (!role) throw new ForumError(404, "This role does not exist.");
  await tx(pool, async (db) => {
    const before = (await db.query("SELECT permission FROM role_permissions WHERE role_id = $1 ORDER BY 1", [role.id])).rows.map((r) => r.permission);
    await db.query("DELETE FROM role_permissions WHERE role_id = $1", [role.id]);
    await db.query("INSERT INTO role_permissions (role_id, permission) SELECT $1, unnest($2::text[])", [role.id, list]);
    await logAction(db, { actorId: admin.id, action: "role.permissions", targetType: "role", targetId: role.id, meta: { role: roleName, before, after: list.sort() } });
  });
}

async function assignRole(pool, admin, { nickname, role, categoryId = null }) {
  await need(pool, admin, "role.assign");
  const user = (await pool.query("SELECT id FROM aura.public_profiles WHERE lower(username) = lower($1)", [String(nickname ?? "").trim()])).rows[0];
  if (!user) throw new ForumError(422, "No active account with that nickname.", "nickname");
  const r = (await pool.query("SELECT id FROM roles WHERE name = $1", [role])).rows[0];
  if (!r || role === "member") throw new ForumError(422, "Choose moderator or administrator.", "role");
  await tx(pool, async (db) => {
    await db.query("INSERT INTO user_roles (user_id, role_id, category_id) VALUES ($1, $2, $3) ON CONFLICT DO NOTHING", [user.id, r.id, categoryId ? Number(categoryId) : null]);
    await logAction(db, { actorId: admin.id, action: "role.assign", targetType: "user", targetId: user.id, meta: { role, category: categoryId ? Number(categoryId) : null } });
  });
}

async function removeRole(pool, admin, { userId, role, categoryId = null }) {
  await need(pool, admin, "role.assign");
  const cat = categoryId ? Number(categoryId) : null;
  await tx(pool, async (db) => {
    if (role === "administrator" && !cat) {
      const { rows } = await db.query("SELECT ur.user_id FROM user_roles ur JOIN roles r ON r.id = ur.role_id WHERE r.name = 'administrator' AND ur.category_id IS NULL FOR UPDATE OF ur");
      if (rows.length <= 1 && rows.some((x) => x.user_id === Number(userId))) throw new ForumError(422, "You cannot remove the last administrator.");
    }
    const del = await db.query("DELETE FROM user_roles ur USING roles r WHERE r.id = ur.role_id AND ur.user_id = $1 AND r.name = $2 AND ur.category_id IS NOT DISTINCT FROM $3", [Number(userId) || 0, role, cat]);
    if (!del.rowCount) throw new ForumError(404, "That person does not have this role.");
    await logAction(db, { actorId: admin.id, action: "role.remove", targetType: "user", targetId: Number(userId), meta: { role, category: cat } });
  });
}

async function overview(db) {
  const categories = (await db.query("SELECT c.*, (SELECT count(*)::int FROM threads WHERE category_id = c.id) AS threads FROM categories c ORDER BY COALESCE(c.parent_id, c.id), c.parent_id NULLS FIRST, c.position, c.id")).rows;
  const tags = (await db.query("SELECT t.*, (SELECT count(*)::int FROM thread_tags WHERE tag_id = t.id) AS threads FROM tags t ORDER BY t.name")).rows;
  const roles = (await db.query("SELECT r.id, r.name, COALESCE(array_agg(rp.permission ORDER BY rp.permission) FILTER (WHERE rp.permission IS NOT NULL), '{}') AS permissions FROM roles r LEFT JOIN role_permissions rp ON rp.role_id = r.id GROUP BY r.id ORDER BY r.id")).rows;
  const holders = (await db.query(
    `SELECT ur.user_id, ur.category_id, r.name AS role, p.username, c.name AS category
     FROM user_roles ur JOIN roles r ON r.id = ur.role_id LEFT JOIN aura.public_profiles p ON p.id = ur.user_id LEFT JOIN categories c ON c.id = ur.category_id
     ORDER BY r.id DESC, p.username`,
  )).rows;
  return { categories, tags, roles, holders, permissions: PERMISSIONS };
}

module.exports = { saveCategory, deleteCategory, saveTag, deleteTag, setRolePermissions, assignRole, removeRole, overview };
