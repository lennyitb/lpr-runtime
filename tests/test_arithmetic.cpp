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
