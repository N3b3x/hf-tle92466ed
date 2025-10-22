# Doxygen Awesome CSS

This directory contains the doxygen-awesome-css submodule for modern, responsive Doxygen
documentation styling.

## Features

- Modern, clean design
- Dark mode support with toggle
- Responsive layout for mobile devices
- Interactive table of contents
- Fragment copy buttons
- Paragraph links
- Tabbed content support

## Integration

The Doxygen Awesome CSS is integrated into the TLE92466ED driver documentation through:

1. **Git Submodule**: Located at `_config/doxygen-extensions/doxygen-awesome-css/`
2. **Doxyfile Configuration**: Configured in the root `Doxyfile`
3. **CSS Files**: Applied via `HTML_EXTRA_STYLESHEET`
4. **JavaScript Enhancements**: Applied via `HTML_EXTRA_FILES`

## Usage

The CSS and JavaScript files are automatically included when running Doxygen. No manual
configuration needed.

## Updating

To update the doxygen-awesome-css to the latest version:

```bash
cd _config/doxygen-extensions/doxygen-awesome-css
git pull origin main
cd ../../../
git add _config/doxygen-extensions/doxygen-awesome-css
git commit -m "Update doxygen-awesome-css"
```text

## Resources

- [Doxygen Awesome CSS GitHub](https://github.com/jothepro/doxygen-awesome-css)
- [Documentation](https://jothepro.github.io/doxygen-awesome-css/)
- [Customization Guide](https://jothepro.github.io/doxygen-awesome-css/md_docs_2customization.html)
