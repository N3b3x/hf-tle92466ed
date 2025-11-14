# Contributing to HardFOC TLE92466ED Driver

Thank you for your interest in contributing to the HardFOC TLE92466ED Driver project!

## Development Workflow

1. **Fork** the repository
2. **Create** a feature branch (`feature/new-feature`)
3. **Implement** following coding standards
4. **Test** with existing applications
5. **Document** your changes
6. **Submit** a pull request

## Coding Standards

- **Functions**: PascalCase (`SetChannelCurrent`, `ReadDiagnostics`)
- **Types**: snake_case with `*t` suffix (`hal_error_t`)
- **Error Handling**: Use `std::expected` for type safety
- **Code Formatting**: Use `clang-format`

## Testing

Please ensure your changes are tested with the existing test applications in `examples/esp32/`.

## Documentation

- Update relevant documentation files in the `docs/` directory
- Add code comments for new functions and classes
- Update examples if API changes are made

## Questions?

- Open an issue on [GitHub Issues](https://github.com/n3b3x/hf-tle92466ed-driver/issues)
- Check the [README.md](README.md) for more information
