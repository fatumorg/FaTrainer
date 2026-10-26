(() => {
  const PENDING = "ft_draft_pending";
  const key = "ft_draft:" + location.pathname;
  const params = new URLSearchParams(location.search);

  if (params.has("posted")) {
    const pending = sessionStorage.getItem(PENDING);
    if (pending) localStorage.removeItem(pending);
    localStorage.removeItem(key);
    sessionStorage.removeItem(PENDING);
    params.delete("posted");
    const q = params.toString();
    history.replaceState(null, "", location.pathname + (q ? "?" + q : "") + location.hash);
  }

  const wrap = {
    bold: (s) => ["**", s || "bold", "**"],
    italic: (s) => ["*", s || "italic", "*"],
    code: (s) => ["`", s || "code", "`"],
    link: (s) => ["[", s || "text", "](https://)"],
    quote: (s) => ["", (s || "quote").split("\n").map((l) => "> " + l).join("\n"), ""],
    list: (s) => ["", (s || "item").split("\n").map((l) => "- " + l).join("\n"), ""],
  };

  for (const form of document.querySelectorAll("form[data-draft]")) {
    const area = form.querySelector("textarea[name=body]");
    const title = form.querySelector("input[name=title]");
    const editor = form.querySelector("[data-editor]");
    if (!area) continue;

    try {
      const saved = JSON.parse(localStorage.getItem(key) || "null");
      if (saved && !area.value) {
        area.value = saved.body || "";
        if (title && !title.value && saved.title) title.value = saved.title;
      }
    } catch {
      localStorage.removeItem(key);
    }

    const save = () => {
      if (!area.value && !(title && title.value)) return localStorage.removeItem(key);
      localStorage.setItem(key, JSON.stringify({ body: area.value, title: title ? title.value : "", at: Date.now() }));
    };
    area.addEventListener("input", save);
    if (title) title.addEventListener("input", save);
    form.addEventListener("submit", () => sessionStorage.setItem(PENDING, key));

    const bar = editor && editor.querySelector(".editor-bar");
    if (!bar) continue;
    bar.hidden = false;
    for (const b of form.querySelectorAll("[data-nojs-preview]")) b.hidden = true;

    const insert = ([before, middle, after]) => {
      const { selectionStart: a, selectionEnd: z, value } = area;
      area.value = value.slice(0, a) + before + middle + after + value.slice(z);
      area.focus();
      area.setSelectionRange(a + before.length, a + before.length + middle.length);
      save();
    };

    const picker = bar.querySelector("[data-upload]");
    const status = editor.querySelector("[data-upload-status]");
    if (picker) picker.addEventListener("change", async () => {
      const file = picker.files[0];
      picker.value = "";
      if (!file) return;
      status.textContent = "Uploading " + file.name + "…";
      try {
        const res = await fetch("/forum/api/uploads", {
          method: "POST",
          headers: { "content-type": file.type, "x-csrf-token": form.querySelector("input[name=_csrf]").value },
          body: file,
        });
        const data = await res.json().catch(() => ({}));
        if (!res.ok) throw new Error(data.message || "Upload failed.");
        insert(["\n", data.markdown, "\n"]);
        status.textContent = "Image added.";
      } catch (err) {
        status.textContent = err.message === "Failed to fetch" ? "Upload failed. Check your connection." : err.message;
      }
    });

    bar.addEventListener("click", async (e) => {
      const btn = e.target.closest("button");
      if (!btn) return;
      const selected = area.value.slice(area.selectionStart, area.selectionEnd);
      if (btn.dataset.md === "block") {
        const lang = bar.querySelector("[data-lang]").value;
        return insert(["\n```" + lang + "\n", selected || "code", "\n```\n"]);
      }
      if (btn.dataset.md) return insert(wrap[btn.dataset.md](selected));
      if (!btn.dataset.tab) return;
      const preview = editor.querySelector("[data-preview]");
      const showPreview = btn.dataset.tab === "preview";
      for (const t of bar.querySelectorAll("[data-tab]")) t.setAttribute("aria-pressed", String(t === btn));
      area.hidden = showPreview;
      preview.hidden = !showPreview;
      if (!showPreview) return area.focus();
      preview.textContent = "Loading preview…";
      try {
        const res = await fetch("/forum/preview", {
          method: "POST",
          headers: { "content-type": "application/json", "x-csrf-token": form.querySelector("input[name=_csrf]").value },
          body: JSON.stringify({ body: area.value }),
        });
        if (!res.ok) throw new Error(String(res.status));
        preview.innerHTML = (await res.json()).html || "<p>Nothing to preview.</p>";
      } catch {
        preview.textContent = "Preview failed. Your text is kept; try again.";
      }
    });
  }
})();
