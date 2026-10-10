document.addEventListener("DOMContentLoaded", () => {
  // 1. Mobile Drawer Toggle
  const mobileBtn = document.getElementById("mobileMenuBtn");
  const sidebar = document.querySelector(".sidebar");
  if (mobileBtn && sidebar) {
    mobileBtn.addEventListener("click", () => {
      sidebar.classList.toggle("open");
    });

    document.addEventListener("click", (e) => {
      if (!sidebar.contains(e.target) && !mobileBtn.contains(e.target) && sidebar.classList.contains("open")) {
        sidebar.classList.remove("open");
      }
    });
  }

  // 2. Theme Toggle with LocalStorage persistence
  const themeToggle = document.getElementById("themeToggle");
  const currentTheme = localStorage.getItem("pylua-theme") || "dark";
  document.documentElement.setAttribute("data-theme", currentTheme);
  updateThemeIcon(currentTheme);

  if (themeToggle) {
    themeToggle.addEventListener("click", () => {
      const activeTheme = document.documentElement.getAttribute("data-theme") === "light" ? "dark" : "light";
      document.documentElement.setAttribute("data-theme", activeTheme);
      localStorage.setItem("pylua-theme", activeTheme);
      updateThemeIcon(activeTheme);
    });
  }

  function updateThemeIcon(theme) {
    if (!themeToggle) return;
    themeToggle.innerHTML = theme === "light" 
      ? `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"></path></svg>`
      : `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="5"></circle><line x1="12" y1="1" x2="12" y2="3"></line><line x1="12" y1="21" x2="12" y2="23"></line><line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line><line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line><line x1="1" y1="12" x2="3" y2="12"></line><line x1="21" y1="12" x2="23" y2="12"></line><line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line><line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line></svg>`;
  }

  // 3. Copy Code Buttons
  document.querySelectorAll(".copy-btn").forEach((btn) => {
    btn.addEventListener("click", () => {
      const codeBlock = btn.closest(".code-block-wrapper")?.querySelector("pre code");
      if (!codeBlock) return;
      navigator.clipboard.writeText(codeBlock.innerText).then(() => {
        const originalText = btn.innerText;
        btn.innerText = "Copied!";
        btn.style.color = "var(--success)";
        setTimeout(() => {
          btn.innerText = originalText;
          btn.style.color = "";
        }, 1800);
      });
    });
  });

  // 4. Multi-Page Global Search Index
  const searchIndex = [
    // Syntax
    { title: "Syntax Overview & Lexical Structure", url: "syntax/index.html", section: "Syntax", keywords: "tokens lexical comments keywords identifiers" },
    { title: "Type System & Tagged Values", url: "syntax/types.html", section: "Syntax", keywords: "types tvalue tagged values nil boolean number integer float string table function userdata thread vector" },
    { title: "Variables & Auto-Local Scoping", url: "syntax/variables.html", section: "Syntax", keywords: "variables local auto-local scope upvalues env global nactvar vardesc" },
    { title: "Control Flow: if, while, for, loops", url: "syntax/control-flow.html", section: "Syntax", keywords: "if else elseif braces curly brackets while repeat until for numeric generic break goto" },
    { title: "Operators & Expression Evaluation", url: "syntax/operators.html", section: "Syntax", keywords: "operators arithmetic != ~= inequality bitwise logical precedence metamethods" },
    { title: "Functions, Closures & Methods", url: "syntax/functions.html", section: "Syntax", keywords: "functions lambda closures varargs multiple returns method syntax colon tailcall" },
    { title: "Module System & Python-style Import", url: "syntax/modules.html", section: "Syntax", keywords: "import modules require package path searcher loaders" },

    // Runtime & Backend
    { title: "Compiler Pipeline: Lexer, Parser & CodeGen", url: "runtime/index.html", section: "Runtime", keywords: "compiler pipeline llex lparser lcode bytecode ast lexer parser" },
    { title: "Register-Based Virtual Machine", url: "runtime/bytecode-vm.html", section: "Runtime", keywords: "vm virtual machine registers lvm callframe execution stack dispatch loop" },
    { title: "Bytecode Opcode Reference", url: "runtime/opcodes.html", section: "Runtime", keywords: "opcodes op_move op_loadk op_call op_jmp op_gettabup op_settabup op_forloop instructions" },
    { title: "Memory Management & Garbage Collector", url: "runtime/gc-memory.html", section: "Runtime", keywords: "gc garbage collector tricolor generational memory allocator tvalue" },

    // Standard Libraries
    { title: "Standard Library Overview", url: "standard-library/index.html", section: "StdLib", keywords: "standard library stdlib globals preloaded" },
    { title: "Base Library & Core Built-ins", url: "standard-library/base.html", section: "StdLib", keywords: "base print assert pcall xpcall error type tostring tonumber select" },
    { title: "Vector Math Library (2D/3D)", url: "standard-library/vector.html", section: "StdLib", keywords: "vector vector2 vector3 math length dot cross normalize" },
    { title: "String & UTF-8 Library", url: "standard-library/string-utf8.html", section: "StdLib", keywords: "string utf8 format match gsub gmatch byte sub len unicode" },
    { title: "Table Manipulation Library", url: "standard-library/table.html", section: "StdLib", keywords: "table insert remove sort concat pack unpack arrays dictionaries" },
    { title: "Math & Bitwise Library", url: "standard-library/math.html", section: "StdLib", keywords: "math sin cos floor ceil sqrt random deg rad pi max min" },
    { title: "I/O & OS Subsystems", url: "standard-library/io-os.html", section: "StdLib", keywords: "io os open read write execute clock time date getenv remove rename" },
    { title: "HTTP Requests & Networking", url: "standard-library/request.html", section: "StdLib", keywords: "request http get post json api headers timeout rest" },
    { title: "Native GUI Subsystem (Windows)", url: "standard-library/gui.html", section: "StdLib", keywords: "gui window button label input messagebox native ui" },
    { title: "Cryptography & Hashing Library", url: "standard-library/hash.html", section: "StdLib", keywords: "hash md5 sha256 checksum cryptography digest" },

    // Game Engine
    { title: "2D Game Engine Architecture", url: "engine/index.html", section: "Engine", keywords: "engine architecture loop run lengine lifecycle init update draw" },
    { title: "Window & Display Subsystem", url: "engine/window.html", section: "Engine", keywords: "window glfw monitors fullscreen resolution vsync resize" },
    { title: "2D Graphics & Immediate Rendering", url: "engine/graphics.html", section: "Engine", keywords: "graphics opengl drawrect drawcircle drawline drawsprite drawtext clear textures" },
    { title: "2D Rigid-Body Physics Engine", url: "engine/physics.html", section: "Engine", keywords: "physics world body collider gravity velocity aabb circle impulse step" },
    { title: "Audio & Spatial Sound Subsystem", url: "engine/audio.html", section: "Engine", keywords: "audio sound miniaudio spatial 3d listener volume looping" },
    { title: "Input Handling & Events", url: "engine/input.html", section: "Engine", keywords: "input keyboard mouse keys clicks position scroll polling" },
    { title: "Immediate-Mode In-Game UI", url: "engine/ui.html", section: "Engine", keywords: "ui immediate button label slider panel hud" },
    { title: "Sprite Sheet & Animation Engine", url: "engine/animation.html", section: "Engine", keywords: "animation animator panim sprite sheet keyframes timeline editor" },
    { title: "Entity-Script Component Model", url: "engine/entity.html", section: "Engine", keywords: "entity scene script lifecycle init update draw components" },
    { title: "Multithreaded Workers & Concurrency", url: "engine/workers.html", section: "Engine", keywords: "workers multithreading threads concurrency parallel background" }
  ];

  // 5. Search Modal / Dropdown
  const searchInput = document.getElementById("docSearch");
  if (searchInput) {
    let dropdown = document.getElementById("searchDropdown");
    if (!dropdown) {
      dropdown = document.createElement("div");
      dropdown.id = "searchDropdown";
      dropdown.style.cssText = `
        position: absolute;
        top: calc(100% + 8px);
        left: 0;
        right: 0;
        background: var(--bg-secondary);
        border: 1px solid var(--bg-card-border);
        border-radius: 10px;
        box-shadow: 0 16px 36px rgba(0,0,0,0.4);
        max-height: 380px;
        overflow-y: auto;
        display: none;
        z-index: 200;
        padding: 8px;
      `;
      searchInput.parentElement.appendChild(dropdown);
    }

    // Determine path prefix (relative to root docs/)
    const isRoot = window.location.pathname.endsWith("index.html") && !window.location.pathname.includes("/syntax/") && !window.location.pathname.includes("/runtime/") && !window.location.pathname.includes("/standard-library/") && !window.location.pathname.includes("/engine/");
    const pathPrefix = isRoot ? "" : "../";

    searchInput.addEventListener("input", (e) => {
      const q = e.target.value.toLowerCase().trim();
      if (!q) {
        dropdown.style.display = "none";
        dropdown.innerHTML = "";
        return;
      }

      const results = searchIndex.filter(item => 
        item.title.toLowerCase().includes(q) ||
        item.keywords.toLowerCase().includes(q) ||
        item.section.toLowerCase().includes(q)
      );

      if (results.length === 0) {
        dropdown.innerHTML = `<div style="padding: 12px; color: var(--text-muted); font-size: 0.85rem; text-align: center;">No documentation matches for "${q}"</div>`;
        dropdown.style.display = "block";
        return;
      }

      dropdown.innerHTML = results.slice(0, 8).map(item => `
        <a href="${pathPrefix}${item.url}" style="
          display: flex;
          align-items: center;
          justify-content: space-between;
          padding: 10px 14px;
          color: var(--text-primary);
          text-decoration: none;
          border-radius: 6px;
          margin-bottom: 4px;
          transition: background 0.15s;
          font-size: 0.88rem;
        " onmouseover="this.style.background='var(--bg-tertiary)'" onmouseout="this.style.background='transparent'">
          <div>
            <div style="font-weight: 600; color: var(--text-primary);">${item.title}</div>
            <div style="font-size: 0.75rem; color: var(--text-muted); margin-top: 2px;">${item.section}</div>
          </div>
          <span style="font-size: 0.75rem; background: rgba(59, 130, 246, 0.15); color: var(--accent-secondary); padding: 2px 6px; border-radius: 4px;">Jump &rarr;</span>
        </a>
      `).join("");
      dropdown.style.display = "block";
    });

    document.addEventListener("click", (e) => {
      if (!searchInput.contains(e.target) && !dropdown.contains(e.target)) {
        dropdown.style.display = "none";
      }
    });

    // Keyboard shortcut '/'
    window.addEventListener("keydown", (e) => {
      if (e.key === "/" && document.activeElement !== searchInput) {
        e.preventDefault();
        searchInput.focus();
      }
      if (e.key === "Escape") {
        searchInput.blur();
        dropdown.style.display = "none";
        if (sidebar) sidebar.classList.remove("open");
      }
    });
  }

  // 6. Active Sidebar Link based on Current Page
  const currentPath = window.location.pathname;
  document.querySelectorAll(".sidebar-link").forEach(link => {
    const href = link.getAttribute("href");
    if (!href) return;
    if (currentPath.endsWith(href) || (href.endsWith("index.html") && currentPath.endsWith("/"))) {
      link.classList.add("active");
    }
  });
});

