require("dotenv").config({ quiet: true });
const Redis = require("ioredis");
const { loadConfig } = require("./config.js");
const { createPool } = require("./db.js");
const { createForumAuth } = require("./auth.js");
const { createApp, boot } = require("./app.js");
const { cleanupUploads } = require("./uploads.js");

let config;
try {
  config = loadConfig();
} catch (e) {
  console.error(e.message);
  process.exit(1);
}

const pool = createPool(config.db);
const redis = new Redis(config.redisUrl, { maxRetriesPerRequest: 2 });

createForumAuth({ config, redis, clientSecret: config.clientSecret })
  .then(async (auth) => {
    await boot({ config, pool });
    createApp({ config, pool, auth, redis }).listen(config.port, "127.0.0.1", () => console.log("FaTrainer forum online on port " + config.port));
    setInterval(() => cleanupUploads(pool, config.uploadDir).catch((e) => console.error("upload cleanup failed:", e.message)), 3600_000).unref();
  })
  .catch((e) => {
    console.error("startup failed:", e.message);
    process.exit(1);
  });
