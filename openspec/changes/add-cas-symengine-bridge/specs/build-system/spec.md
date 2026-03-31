## MODIFIED Requirements

### Requirement: CMake Build Configuration
The build system SHALL use CMake 3.20+ targeting C++17 and SHALL use `FetchContent` to acquire all third-party dependencies without manual submodule management.

#### Scenario: Project configures and builds from clean checkout
- **WHEN** a developer runs `cmake -B build && cmake --build build`
- **THEN** all dependencies are fetched automatically and the project builds without errors

#### Scenario: Required dependencies are fetched
- **WHEN** the project is configured
- **THEN** Boost headers (1.84+), SQLite3 amalgamation (3.44+), Catch2 (v3.5+), and SymEngine (v0.14.0+ with `INTEGER_CLASS=boostmp`) are available

#### Scenario: SymEngine builds without GMP
- **WHEN** the project is configured on a system without GMP installed
- **THEN** SymEngine SHALL build successfully using Boost.Multiprecision as its integer backend
- **AND** no external GMP, FLINT, MPC, or MPFR libraries are required
