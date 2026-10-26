const hljs = require("highlight.js");

const RENDERER_VERSION = 1;
const LANGUAGES = new Set(["cpp", "c", "ini", "json", "bash", "powershell", "python", "lua", "diff", "plaintext"]);
const MEDIA = /^\/forum\/media\/[a-f0-9]{32}\.webp$/;
const REL = 'rel="nofollow ugc noopener noreferrer"';
const INLINE = /(`+)([\s\S]*?[^`])\1(?!`)|!\[([^\]]*)\]\(([^)]*)\)|\[([^\]]*)\]\(([^)]*)\)|(https?:\/\/[^\s<>"]+)/g;

function escapeHtml(s) {
  return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;").replace(/'/g, "&#39;");
}

function safeUrl(raw) {
  const url = raw.trim();
  const probe = url.replace(/[\u0000- \u007f]/g, "").toLowerCase();
  const allowed = /^(https?:|mailto:)/.test(probe) || (probe.startsWith("/") && !probe.startsWith("//"));
  if (!allowed) return null;
  return url.replace(/[\s"'<>`]/g, (c) => encodeURIComponent(c)).replace(/&/g, "&amp;");
}

function link(text, raw) {
  const href = safeUrl(raw);
  const label = emphasis(escapeHtml(text));
  if (!href) return label;
  const external = /^(https?:|mailto:)/i.test(href);
  return `<a href="${href}"${external ? ` ${REL}` : ""}>${label}</a>`;
}

function emphasis(s) {
  return s.replace(/\*\*(?=\S)([\s\S]*?\S)\*\*/g, "<strong>$1</strong>").replace(/\*(?=\S)([\s\S]*?\S)\*/g, "<em>$1</em>");
}

function plain(text, ctx) {
  return emphasis(escapeHtml(text)).replace(/(^|[^A-Za-z0-9_.@&#;\/])@([A-Za-z0-9_]{3,32})(?![A-Za-z0-9_])/g, (m, pre, name) => {
    const user = ctx.mentionResolver ? ctx.mentionResolver(name) : null;
    if (!user) return m;
    if (!ctx.mentions.includes(user.username)) ctx.mentions.push(user.username);
    return `${pre}<a href="/forum/u/${encodeURIComponent(user.username)}" class="mention">@${escapeHtml(user.username)}</a>`;
  });
}

function inline(text, ctx) {
  let out = "";
  let last = 0;
  for (const m of text.matchAll(INLINE)) {
    out += plain(text.slice(last, m.index), ctx);
    last = m.index + m[0].length;
    if (m[1]) out += `<code>${escapeHtml(m[2].trim())}</code>`;
    else if (m[3] !== undefined) {
      const src = m[4].trim();
      out += MEDIA.test(src) ? `<img src="${src}" alt="${escapeHtml(m[3])}" loading="lazy">` : link(m[3] || src, m[4]);
    } else if (m[5] !== undefined) out += link(m[5], m[6]);
    else {
      const url = m[7].replace(/[.,;:!?)]+$/, "");
      out += link(url, url);
      last = m.index + url.length;
    }
  }
  return out + plain(text.slice(last), ctx);
}

function code(body, lang) {
  if (LANGUAGES.has(lang) && lang !== "plaintext") {
    const html = hljs.highlight(body, { language: lang, ignoreIllegals: true }).value;
    return `<pre><code class="hljs language-${lang}">${html}</code></pre>`;
  }
  return `<pre><code class="hljs">${escapeHtml(body)}</code></pre>`;
}

function blocks(lines, ctx) {
  const out = [];
  let i = 0;
  while (i < lines.length) {
    const line = lines[i];
    const fence = line.match(/^\s*(```+|~~~+)\s*([\w+-]*)\s*$/);
    if (fence) {
      const body = [];
      i++;
      while (i < lines.length && !lines[i].trim().startsWith(fence[1])) body.push(lines[i++]);
      i++;
      out.push(code(body.join("\n"), fence[2].toLowerCase()));
      continue;
    }
    if (!line.trim()) { i++; continue; }
    const heading = line.match(/^(#{1,6})\s+(.*?)\s*#*\s*$/);
    if (heading) {
      const tag = heading[1].length <= 2 ? "h3" : "h4";
      out.push(`<${tag}>${inline(heading[2], ctx)}</${tag}>`);
      i++;
      continue;
    }
    if (/^\s*>/.test(line)) {
      const inner = [];
      while (i < lines.length && /^\s*>/.test(lines[i])) inner.push(lines[i++].replace(/^\s*>\s?/, ""));
      out.push(`<blockquote>${blocks(inner, ctx)}</blockquote>`);
      continue;
    }
    const list = line.match(/^\s*([-*+]|\d+[.)])\s+/);
    if (list) {
      const ordered = /\d/.test(list[1]);
      const item = ordered ? /^\s*\d+[.)]\s+(.*)$/ : /^\s*[-*+]\s+(.*)$/;
      const items = [];
      while (i < lines.length && item.test(lines[i])) items.push(`<li>${inline(lines[i++].match(item)[1], ctx)}</li>`);
      const tag = ordered ? "ol" : "ul";
      out.push(`<${tag}>${items.join("")}</${tag}>`);
      continue;
    }
    const para = [];
    while (i < lines.length && lines[i].trim() && !/^\s*(```|~~~|#{1,6}\s|>|[-*+]\s|\d+[.)]\s)/.test(lines[i])) para.push(lines[i++]);
    if (!para.length) para.push(lines[i++]);
    out.push(`<p>${para.map((l) => inline(l, ctx)).join("<br>\n")}</p>`);
  }
  return out.join("\n");
}

function renderMarkdown(md, { mentionResolver } = {}) {
  const ctx = { mentionResolver, mentions: [] };
  const html = blocks(String(md || "").replace(/\r\n?/g, "\n").split("\n"), ctx);
  return { html, mentions: ctx.mentions };
}

module.exports = { renderMarkdown, RENDERER_VERSION, escapeHtml };
