## ADDED Requirements

### Requirement: CMake Build Configuration
The build system SHALL use CMake 3.20+ targeting C++17 and SHALL use `FetchContent` to acquire all third-party dependencies without manual submodule management.

#### Scenario: Project configures and builds from clean checkout
- **WHEN** a developer runs `cmake -B build && cmake --build build`
- **THEN** all dependencies are fetched automatically and the project builds without errors

#### Scenario: Required dependencies are fetched
- **WHEN** the project is configured
- **THEN** Boost headers (1.84+), SQLite3 amalgamation (3.44+), and Catch2 (v3.5+) are available

### Requirement: Build Targets
The build system SHALL produce three targets: a static library `liblpr` from `src/`, a CLI executable `lpr-cli` from `cli/`, and a test executable `lpr-tests` from `tests/`.

#### Scenario: Static library is produced
- **WHEN** the project is built
- **THEN** a static library `liblpr` is produced containing all core runtime code

#### Scenario: CLI executable is produced
- **WHEN** the project is built
- **THEN** an executable `lpr-cli` is produced linking against `liblpr`

#### Scenario: Test executable is produced
- **WHEN** the project is built
- **THEN** an executable `lpr-tests` is produced linking against `liblpr` and Catch2

### Requirement: Install Target
The build system SHALL define an install target that exports `include/lpr/lpr.h` and the `liblpr` static library.

#### Scenario: Headers and library are installed
- **WHEN** `cmake --install build` is run
- **THEN** `include/lpr/lpr.h` and `liblpr.a` are installed to the target prefix
