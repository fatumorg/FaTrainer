const { test } = require("node:test");
const assert = require("node:assert/strict");
const { renderMarkdown, RENDERER_VERSION } = require("../src/markdown.js");

const known = new Set(["gracz"]);
const r = { mentionResolver: (name) => (known.has(name.toLowerCase()) ? { username: name } : null) };
const html = (md) => renderMarkdown(md, r).html;

test("renderer has a version", () => assert.equal(RENDERER_VERSION, 1));

test("raw html is escaped", () => {
  const out = html('<img src=x onerror=alert(1)> <script>alert(1)</script>');
  assert.equal(out.includes("<img"), false);
  assert.equal(out.includes("<script"), false);
  assert.match(out, /&lt;img/);
});

test("javascript links dropped", () => {
  assert.equal(/href="javascript:/i.test(html("[a](javascript:alert(1))")), false);
  assert.equal(/href="\s*java\s*script:/i.test(html("[a](JaVa\tScRiPt:alert(1))")), false);
  assert.equal(/href="data:/i.test(html("[a](data:text/html;base64,AAAA)")), false);
  assert.match(html("[a](javascript:alert(1))"), /a/);
});

test("external links get nofollow ugc noopener noreferrer", () => {
  assert.match(html("https://example.com"), /rel="nofollow ugc noopener noreferrer"/);
  assert.match(html("[x](https://example.com/a?b=1&c=2)"), /href="https:\/\/example\.com\/a\?b=1&amp;c=2"/);
  assert.match(html('[x](https://e.com/"onmouseover="x)'), /href="https:\/\/e\.com\/%22onmouseover=%22x"/);
});

test("external images are not embedded", () => {
  assert.equal(html("![a](https://x.y/a.png)").includes("<img"), false);
  assert.equal(html("![a](//x.y/a.png)").includes("<img"), false);
  assert.equal(html("![a](/forum/media/../../etc/passwd)").includes("<img"), false);
});

test("own upload images allowed", () => {
  assert.match(html("![shot](/forum/media/0123456789abcdef0123456789abcdef.webp)"), /<img src="\/forum\/media\/[a-f0-9]{32}\.webp" alt="shot"/);
  assert.match(html('![a"b](/forum/media/0123456789abcdef0123456789abcdef.webp)'), /alt="a&quot;b"/);
});

test("cpp code highlighted on the server", () => {
  assert.match(html("```cpp\nint a;\n```"), /class="hljs-(type|keyword)"/);
  const unknown = html("```brainfuck\n<b>\n```");
  assert.match(unknown, /&lt;b&gt;/);
  assert.equal(unknown.includes("<b>"), false);
});

test("mentions resolved only for discoverable users", () => {
  const out = renderMarkdown("hi @gracz and @ghost, mail a@gracz.pl", r);
  assert.deepEqual(out.mentions, ["gracz"]);
  assert.match(out.html, /<a href="\/forum\/u\/gracz" class="mention">@gracz<\/a>/);
  assert.match(out.html, /@ghost/);
  assert.equal(out.html.includes('/forum/u/ghost'), false);
});

test("basic formatting", () => {
  const out = html("# Title\n\n**bold** and *italic* and `code <b>`\n\n- one\n- two\n\n1. a\n2. b\n\n> quote\n\nline1\nline2");
  for (const re of [/<h3>Title<\/h3>/, /<strong>bold<\/strong>/, /<em>italic<\/em>/, /<code>code &lt;b&gt;<\/code>/, /<ul>\s*<li>one<\/li>\s*<li>two<\/li>\s*<\/ul>/, /<ol>\s*<li>a<\/li>\s*<li>b<\/li>\s*<\/ol>/, /<blockquote>[\s\S]*quote[\s\S]*<\/blockquote>/, /line1<br>\s*line2/]) assert.match(out, re);
});

test("code inside emphasis markers stays literal", () => {
  assert.match(html("`*not italic*`"), /<code>\*not italic\*<\/code>/);
  assert.match(html("```\n**x**\n```"), /\*\*x\*\*/);
});
