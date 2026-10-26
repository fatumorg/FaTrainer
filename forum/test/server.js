const { createServer } = require("node:http");
const os = require("node:os");
const fs = require("node:fs");
const path = require("node:path");
const Redis = require("ioredis");
const { startTestIssuer, httpClient } = require("fatum-kit/testing");
const { testDb } = require("./helpers.js");
const { createForumAuth } = require("../src/auth.js");
const { createApp, boot } = require("../src/app.js");

const SECRET = "forum-test-secret-forum-test-secret-forum";

async function startForum(t, { claims = {}, ownerSub } = {}) {
  const { pool, users } = await testDb(t);
  const server = createServer();
  await new Promise((r) => server.listen(0, "127.0.0.1", r));
  const appUrl = `http://127.0.0.1:${server.address().port}`;
  const accounts = {};
  for (const [name, id] of Object.entries(users)) accounts[id] = { preferred_username: name, email: `${name}@x.pl`, email_verified: true, ...(claims[name] || {}) };
  const issuer = await startTestIssuer({
    clients: [{ client_id: "fatrainer-forum", client_secret: SECRET, redirect_uris: [`${appUrl}/forum/auth/callback`], post_logout_redirect_uris: [`${appUrl}/`], scope: "openid profile email preferences" }],
    accounts,
  });
  const redis = new Redis(process.env.TEST_REDIS_URL);
  await redis.flushdb();
  const uploadDir = fs.mkdtempSync(path.join(os.tmpdir(), "forum-uploads-"));
  const config = { appUrl, issuer: issuer.issuer, ownerSub: ownerSub === undefined ? String(users.owner) : ownerSub, uploadDir, secret: SECRET };
  const sent = [];
  const auth = await createForumAuth({ config, redis, clientSecret: SECRET });
  const app = createApp({ config, pool, auth, redis, mailer: { send: async (m) => sent.push(m) } });
  await boot({ config, pool });
  server.on("request", app);
  t.after(async () => {
    server.closeAllConnections();
    await new Promise((r) => server.close(r));
    await issuer.close();
    await redis.quit();
  });
  const signedIn = async (name) => {
    const client = httpClient(appUrl);
    issuer.signIn(String(users[name]));
    await client.get("/forum/auth/login?return_to=%2Fforum").followRedirects();
    const page = await client.get("/forum");
    const csrf = (page.text.match(/name="_csrf" value="([^"]+)"/) || [])[1];
    return { client, csrf };
  };
  return { appUrl, pool, users, issuer, config, sent, uploadDir, signedIn, anon: () => httpClient(appUrl) };
}

module.exports = { startForum };
