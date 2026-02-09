#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"

using namespace lpr;

static Context make_ctx() { return Context(nullptr); }

TEST_CASE("Integer addition", "[arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("3 4 +"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "7");
}

TEST_CASE("Integer subtraction", "[arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 3 -"));
    REQUIRE(ctx.repr_at(1) == "7");
}

TEST_CASE("Integer multiplication", "[arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("6 7 *"));
    REQUIRE(ctx.repr_at(1) == "42");
}

TEST_CASE("Integer division produces Rational", "[arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("355 113 /"));
    REQUIRE(ctx.repr_at(1) == "355/113");
}

TEST_CASE("Division by zero produces error", "[arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("5 0 /"));
    // Error should be on stack
    REQUIRE(ctx.depth() >= 1);
}

TEST_CASE("NEG command", "[arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("5 NEG"));
    REQUIRE(ctx.repr_at(1) == "-5");
}

TEST_CASE("INV command", "[arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("4 INV"));
    REQUIRE(ctx.repr_at(1) == "1/4");
}

TEST_CASE("ABS command", "[arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("-7 ABS"));
    REQUIRE(ctx.repr_at(1) == "7");
}

TEST_CASE("MOD command", "[arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 3 MOD"));
    REQUIRE(ctx.repr_at(1) == "1");
}

TEST_CASE("Trailing-dot literal parses as Real", "[arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("2."));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "2.");
}

TEST_CASE("Real arithmetic preserves Real type", "[arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("2. 2 +"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "4.");
}

TEST_CASE("Real with fractional part displays normally", "[arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("3.14"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "3.14");
}

TEST_CASE("Mixed-type addition promotes to Real", "[arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 2.5 +"));
    REQUIRE(ctx.depth() == 1);
    // Result should be Real(3.5)
    std::string r = ctx.repr_at(1);
    REQUIRE(r.find("3.5") != std::string::npos);
}

TEST_CASE("Too few arguments produces error", "[arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("+"));
    REQUIRE(ctx.depth() >= 1); // Error on stack
}

// ---- Symbolic arithmetic ----

TEST_CASE("Symbolic addition: two names", "[arithmetic][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'A' 'B' +"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "'A+B'");
}

TEST_CASE("Symbolic addition: name and number", "[arithmetic][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'A' 3 +"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "'A+3'");
}

TEST_CASE("Symbolic addition: number and name", "[arithmetic][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("3 'A' +"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "'3+A'");
}

TEST_CASE("Symbolic subtraction", "[arithmetic][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X' 'Y' -"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "'X-Y'");
}

TEST_CASE("Symbolic multiplication", "[arithmetic][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'A' 'B' *"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "'A*B'");
}

TEST_CASE("Symbolic division", "[arithmetic][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'A' 'B' /"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "'A/B'");
}

TEST_CASE("Symbolic NEG", "[arithmetic][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'A' NEG"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "'-(A)'");
}

TEST_CASE("Symbolic precedence: (A+B)*C", "[arithmetic][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'A' 'B' + 'C' *"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "'(A+B)*C'");
}

TEST_CASE("Symbolic precedence: A*B+C has no extra parens", "[arithmetic][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'A' 'B' * 'C' +"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "'A*B+C'");
}

TEST_CASE("Symbolic chained: A+B+C", "[arithmetic][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'A' 'B' + 'C' +"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "'A+B+C'");
}
