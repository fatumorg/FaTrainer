const path = require("node:path");
const pg = require("pg");
const { createTestDatabase } = require("fatum-kit/testing");

const AURA_MIGRATIONS = process.env.AURA_MIGRATIONS || "/data/Coding/Web/Fatum Aura/migrations";

async function testDb(t, names = ["owner", "gracz", "mod", "inny"]) {
  const db = await createTestDatabase({ adminUrl: process.env.TEST_PG_ADMIN_URL, migrationsDir: AURA_MIGRATIONS });
  const pool = new pg.Pool({ connectionString: db.url, options: "-c search_path=fatrainer,public", max: 5 });
  t.after(async () => {
    await pool.end();
    await db.drop();
  });
  const users = {};
  for (const name of names) {
    const { rows } = await pool.query("INSERT INTO aura.users (username, password_hash, email, profile_visibility, discoverable) VALUES ($1, 'x', $2, 'everyone', true) RETURNING id", [name, `${name}@x.pl`]);
    users[name] = rows[0].id;
  }
  return { pool, users, url: db.url };
}

const categoryId = async (pool, slug) => (await pool.query("SELECT id FROM categories WHERE slug = $1", [slug])).rows[0].id;

async function grantRole(pool, userId, role, category = null) {
  await pool.query("INSERT INTO user_roles (user_id, role_id, category_id) SELECT $1, id, $3 FROM roles WHERE name = $2", [userId, role, category]);
}

module.exports = { testDb, categoryId, grantRole, ROOT: path.resolve(__dirname, "..") };
