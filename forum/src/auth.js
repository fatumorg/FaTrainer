const { createAuraAuth } = require("fatum-kit/aura");

function createForumAuth({ config, redis, clientSecret }) {
  return createAuraAuth({
    issuer: config.issuer,
    clientId: "fatrainer-forum",
    clientSecret,
    appUrl: config.appUrl,
    scopes: ["openid", "profile", "email", "preferences"],
    redis,
    cookieName: "__Host-ft_sid",
    sessionSecret: config.secret,
    mountPath: "/forum/auth",
    sessionTtlSeconds: 14 * 24 * 3600,
  });
}

module.exports = { createForumAuth };
