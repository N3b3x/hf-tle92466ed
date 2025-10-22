---
layout: default
title: Doxygen Extensions
nav_order: 90
---

# Doxygen Extensions

The TLE92466ED driver documentation uses
[Doxygen Awesome CSS](https://github.com/jothepro/doxygen-awesome-css) for modern,
responsive API documentation.

## Features

### Modern Design
- Clean, minimalist interface
- Professional typography
- Consistent spacing and layout
- Mobile-responsive design

### Dark Mode
- Toggle between light and dark themes
- Automatic theme detection
- Persistent theme selection

### Interactive Elements
- **Copy Buttons**: One-click code snippet copying
- **Paragraph Links**: Direct linking to sections
- **Interactive TOC**: Collapsible table of contents
- **Tabbed Content**: Organized information display

### Developer Experience
- Fast page loading
- Smooth animations
- Keyboard navigation
- Search integration

## Configuration

The Doxygen configuration (`Doxyfile`) includes:

```doxyfile
HTML_EXTRA_STYLESHEET  = _config/doxygen-extensions/doxygen-awesome-css/doxygen-awesome.css \
                         _config/doxygen-extensions/doxygen-awesome-css/doxygen-awesome-sidebar-only.css \
                         _config/doxygen-extensions/doxygen-awesome-css/doxygen-awesome-sidebar-only-darkmode-toggle.css

HTML_EXTRA_FILES       = _config/doxygen-extensions/doxygen-awesome-css/doxygen-awesome-darkmode-toggle.js \
                         _config/doxygen-extensions/doxygen-awesome-css/doxygen-awesome-fragment-copy-button.js \
                         _config/doxygen-extensions/doxygen-awesome-css/doxygen-awesome-paragraph-link.js \
                         _config/doxygen-extensions/doxygen-awesome-css/doxygen-awesome-interactive-toc.js \
                         _config/doxygen-extensions/doxygen-awesome-css/doxygen-awesome-tabs.js
```text

## Resources

- [Doxygen Awesome CSS Documentation](https://jothepro.github.io/doxygen-awesome-css/)
- [GitHub Repository](https://github.com/jothepro/doxygen-awesome-css)
- [Customization Guide](https://jothepro.github.io/doxygen-awesome-css/md_docs_2customization.html)
