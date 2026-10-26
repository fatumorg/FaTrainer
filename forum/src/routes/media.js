const express = require("express");
const { ImageError } = require("fatum-kit/images");
const { saveUpload, mediaFile, UploadError } = require("../uploads.js");

const MESSAGES = {
  unsupported_type: "Use a PNG, JPEG or WebP image.",
  type_mismatch: "The file is not the image type it claims to be.",
  too_large: "The image is over 5 MB or too many pixels.",
  bad_image: "The image could not be read.",
};

function mediaRoutes({ pool, config, auth }) {
  const r = express.Router();
  const raw = express.raw({ type: () => true, limit: "5mb" });

  r.post("/api/uploads", (req, res, next) => (req.session?.user ? next() : res.status(401).json({ error: "AUTH_REQUIRED" })), auth.csrf.verify, raw, async (req, res) => {
    try {
      const out = await saveUpload(pool, config.uploadDir, req.session.user, Buffer.isBuffer(req.body) ? req.body : Buffer.alloc(0), String(req.get("content-type") || "").split(";")[0].trim());
      res.status(201).json(out);
    } catch (err) {
      if (err instanceof UploadError) return res.status(err.status).json({ error: err.code, message: err.message });
      if (err instanceof ImageError) return res.status(422).json({ error: err.code, message: MESSAGES[err.code] || "The image could not be used." });
      throw err;
    }
  });

  r.get("/media/:name", async (req, res, next) => {
    const file = await mediaFile(pool, config.uploadDir, req.params.name);
    if (!file) return next();
    res.set({
      "content-type": "image/webp",
      "x-content-type-options": "nosniff",
      "content-security-policy": "default-src 'none'",
      "content-disposition": 'inline; filename="image.webp"',
      "cache-control": "public, max-age=31536000, immutable",
    });
    res.sendFile(file, { headers: { "content-type": "image/webp" } }, (err) => {
      if (err && !res.headersSent) next();
    });
  });

  return r;
}

module.exports = { mediaRoutes };
