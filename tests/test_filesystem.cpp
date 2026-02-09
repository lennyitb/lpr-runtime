#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"

using namespace lpr;

TEST_CASE("STO and RCL", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("42 'x' STO"));
    REQUIRE(ctx.depth() == 0);
    REQUIRE(ctx.exec("'x' RCL"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "42");
}

TEST_CASE("RCL nonexistent variable produces error", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE_FALSE(ctx.exec("'unknown' RCL"));
}

TEST_CASE("PURGE removes variable", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("42 'x' STO"));
    REQUIRE(ctx.exec("'x' PURGE"));
    REQUIRE_FALSE(ctx.exec("'x' RCL"));
}

TEST_CASE("HOME command", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("HOME"));
    // Should not error
    REQUIRE(ctx.depth() == 0);
}

TEST_CASE("PATH command", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("PATH"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "\"HOME\"");
}

TEST_CASE("CRDIR creates subdirectory", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("'MYDIR' CRDIR"));
    REQUIRE(ctx.depth() == 0);
}

TEST_CASE("VARS lists variables", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("10 'a' STO"));
    REQUIRE(ctx.exec("20 'b' STO"));
    REQUIRE(ctx.exec("VARS"));
    REQUIRE(ctx.depth() == 1);
    std::string vars = ctx.repr_at(1);
    REQUIRE(vars.find("a") != std::string::npos);
    REQUIRE(vars.find("b") != std::string::npos);
}

// ---- Implicit variable recall (name fallthrough) ----

TEST_CASE("Bare name recalls stored variable", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("42 'X' STO"));
    REQUIRE(ctx.exec("X"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "42");
}

TEST_CASE("Bare name executes stored program", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("<< 1 2 + >> 'ADD3' STO"));
    REQUIRE(ctx.exec("ADD3"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "3");
}

TEST_CASE("Bare unknown name still errors", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE_FALSE(ctx.exec("NOSUCHVAR"));
}
