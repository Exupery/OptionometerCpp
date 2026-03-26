# CLAUDE.md

## Testing

When making any code changes, always create new tests or update existing tests as appropriate:

- New functionality must have corresponding unit tests.
- Changes to existing logic must update any affected tests to match the new behavior.
- Tests are in `tests/` using Google Test. Follow the existing patterns (fixtures, `EXPECT_NEAR` for floats, helper functions).
- Run tests after changes to confirm they pass: `./out/build/x64-debug/tests/OptionometerTests.exe`
- **Do not attempt to build or run tests from WSL.** This project uses MSVC/VS2022 and must be built manually in Visual Studio. The test binary only works after a VS2022 build.
