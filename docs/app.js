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

  // 2. Theme Toggle
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
      const code = btn.closest(".code-block-wrapper").querySelector("pre code").innerText;
      navigator.clipboard.writeText(code).then(() => {
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

  // 4. Live Search
  const searchInput = document.getElementById("docSearch");
  const sidebarLinks = document.querySelectorAll(".sidebar-link");
  const sections = document.querySelectorAll("section.doc-section");

  if (searchInput) {
    searchInput.addEventListener("input", (e) => {
      const query = e.target.value.toLowerCase().trim();

      sidebarLinks.forEach((link) => {
        const text = link.innerText.toLowerCase();
        link.style.display = text.includes(query) || query === "" ? "flex" : "none";
      });

      if (query.length > 1) {
        sections.forEach((sec) => {
          const content = sec.innerText.toLowerCase();
          sec.style.display = content.includes(query) ? "block" : "none";
        });
      } else {
        sections.forEach((sec) => (sec.style.display = "block"));
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
        if (sidebar) sidebar.classList.remove("open");
      }
    });
  }

  // 5. Active Link Spy via IntersectionObserver
  const observerOptions = {
    root: null,
    rootMargin: "-20% 0px -70% 0px",
    threshold: 0
  };

  const observer = new IntersectionObserver((entries) => {
    entries.forEach((entry) => {
      if (entry.isIntersecting) {
        const id = entry.target.getAttribute("id");
        sidebarLinks.forEach((link) => {
          if (link.getAttribute("href") === `#${id}`) {
            link.classList.add("active");
          } else {
            link.classList.remove("active");
          }
        });
      }
    });
  }, observerOptions);

  sections.forEach((section) => observer.observe(section));
});

