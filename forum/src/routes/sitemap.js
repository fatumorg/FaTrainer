const express = require("express");

const esc = (s) => String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");

function sitemapRoutes({ pool, config }) {
  const r = express.Router();
  r.get("/sitemap.xml", async (req, res) => {
    const base = config.appUrl;
    const cats = await pool.query("SELECT slug FROM categories ORDER BY position, id");
    const threads = await pool.query(
      `SELECT t.id, t.slug, t.last_post_at FROM threads t
       WHERE t.deleted_at IS NULL AND EXISTS (SELECT 1 FROM posts p WHERE p.thread_id = t.id AND NOT p.held AND p.deleted_at IS NULL)
       ORDER BY t.id DESC LIMIT 20000`,
    );
    const urls = [
      `<url><loc>${esc(base)}/forum</loc></url>`,
      ...cats.rows.map((c) => `<url><loc>${esc(base)}/forum/c/${esc(c.slug)}</loc></url>`),
      ...threads.rows.map((t) => `<url><loc>${esc(base)}/forum/t/${t.id}/${esc(t.slug)}</loc><lastmod>${t.last_post_at.toISOString()}</lastmod></url>`),
    ];
    res.type("application/xml").send(`<?xml version="1.0" encoding="UTF-8"?>\n<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n${urls.join("\n")}\n</urlset>\n`);
  });
  return r;
}

module.exports = { sitemapRoutes };
