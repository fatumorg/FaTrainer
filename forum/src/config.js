const REQUIRED = ["PORT", "SECRET", "DB_USER", "DB_HOST", "DB_NAME", "DB_PASSWORD", "DB_PORT", "REDIS_URL", "AURA_CLIENT_SECRET", "UPLOAD_DIR"];

function loadConfig(env = process.env) {
  const missing = REQUIRED.filter((k) => !env[k]);
  if (missing.length) throw new Error("MISSING_ENV: " + missing.join(", "));
  if (env.SECRET.length < 32) throw new Error("SECRET_TOO_SHORT");
  return {
    port: Number(env.PORT),
    appUrl: env.APP_URL || "https://fatrainer.fatum.cc",
    issuer: env.AURA_ISSUER || "https://fatum.cc",
    secret: env.SECRET,
    clientSecret: env.AURA_CLIENT_SECRET,
    redisUrl: env.REDIS_URL,
    ownerSub: env.FORUM_OWNER_SUB || "",
    uploadDir: env.UPLOAD_DIR,
    db: { user: env.DB_USER, host: env.DB_HOST, database: env.DB_NAME, password: env.DB_PASSWORD, port: Number(env.DB_PORT) },
  };
}

module.exports = { loadConfig };
