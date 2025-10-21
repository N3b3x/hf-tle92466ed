# GitHub Actions Workflows

This directory contains the CI/CD workflows for the TLE92466ED driver project.

## Workflows

### 📚 Publish Documentation (`publish-docs.yml`)
Builds and publishes versioned documentation to GitHub Pages using:
- Doxygen for API documentation
- Jekyll for site generation
- Doxygen Awesome CSS for modern styling
- Markdown and link quality checks

**Triggers:**
- Push to `main` or `release/*` branches
- Version tags (`v*`)
- Pull requests to `main`
- Manual dispatch

### 📚 YAML Lint (`yamllint.yml`)
Validates YAML syntax across all workflow files and configurations.

**Triggers:**
- Push to `main` or `develop`
- Pull requests to `main` or `develop`
- Manual dispatch

## Configuration

Workflows use reusable workflows from:
- `N3b3x/hf-general-ci-tools` for documentation publishing
- `N3b3x/hf-general-ci-tools` for YAML linting

## Dependabot

The `dependabot.yml` file configures automated dependency updates for:
- Python dependencies
- GitHub Actions

## Development

When adding new workflows:
1. Use meaningful names with emoji prefixes
2. Add comprehensive comments
3. Configure appropriate triggers
4. Set proper permissions
5. Use reusable workflows where possible
6. Test with `act` locally if possible

