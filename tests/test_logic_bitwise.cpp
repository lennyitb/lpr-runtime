#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"

using namespace lpr;

static Context make_ctx() { return Context(nullptr); }

// --- Boolean Logic ---

TEST_CASE("AND - true && true", "[logic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 1 AND"));
    REQUIRE(ctx.repr_at(1) == "1");
}

TEST_CASE("AND - true && false", "[logic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 0 AND"));
    REQUIRE(ctx.repr_at(1) == "0");
}

TEST_CASE("OR - false || true", "[logic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 1 OR"));
    REQUIRE(ctx.repr_at(1) == "1");
}

TEST_CASE("OR - false || false", "[logic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 0 OR"));
    REQUIRE(ctx.repr_at(1) == "0");
}

TEST_CASE("NOT - true", "[logic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 NOT"));
    REQUIRE(ctx.repr_at(1) == "0");
}

TEST_CASE("NOT - false", "[logic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 NOT"));
    REQUIRE(ctx.repr_at(1) == "1");
}

TEST_CASE("XOR - true ^ false", "[logic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 0 XOR"));
    REQUIRE(ctx.repr_at(1) == "1");
}

TEST_CASE("XOR - true ^ true", "[logic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 1 XOR"));
    REQUIRE(ctx.repr_at(1) == "0");
}

TEST_CASE("AND treats nonzero as true", "[logic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("5 3 AND"));
    REQUIRE(ctx.repr_at(1) == "1");
}

// --- Bitwise Operations ---

TEST_CASE("BAND", "[bitwise]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("12 10 BAND"));
    REQUIRE(ctx.repr_at(1) == "8"); // 1100 & 1010 = 1000
}

TEST_CASE("BOR", "[bitwise]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("12 10 BOR"));
    REQUIRE(ctx.repr_at(1) == "14"); // 1100 | 1010 = 1110
}

TEST_CASE("BXOR", "[bitwise]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("12 10 BXOR"));
    REQUIRE(ctx.repr_at(1) == "6"); // 1100 ^ 1010 = 0110
}

TEST_CASE("BNOT", "[bitwise]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 BNOT"));
    REQUIRE(ctx.repr_at(1) == "-1");
}

TEST_CASE("SL - shift left", "[bitwise]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 4 SL"));
    REQUIRE(ctx.repr_at(1) == "16");
}

TEST_CASE("SR - shift right", "[bitwise]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("16 2 SR"));
    REQUIRE(ctx.repr_at(1) == "4");
}

TEST_CASE("ASR - arithmetic shift right", "[bitwise]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("-16 2 ASR"));
    REQUIRE(ctx.repr_at(1) == "-4");
}

// --- SAME ---

TEST_CASE("SAME - identical integers", "[logic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("42 42 SAME"));
    REQUIRE(ctx.repr_at(1) == "1");
}

TEST_CASE("SAME - different types same value", "[logic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 1.0 SAME"));
    REQUIRE(ctx.repr_at(1) == "0");
}

TEST_CASE("SAME - strings", "[logic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"hello\" \"hello\" SAME"));
    REQUIRE(ctx.repr_at(1) == "1");
}

TEST_CASE("SAME - different strings", "[logic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"hello\" \"world\" SAME"));
    REQUIRE(ctx.repr_at(1) == "0");
}

TEST_CASE("SAME - integer vs rational", "[logic]") {
    auto ctx = make_ctx();
    // Integer 1 vs Rational would be different types
    REQUIRE(ctx.exec("1 1 1 / SAME"));
    // 1 / 1 = Rational 1/1, but != Integer 1 by SAME
    REQUIRE(ctx.repr_at(1) == "0");
}

// --- Type errors ---

TEST_CASE("AND requires integers", "[logic]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1.0 1 AND"));
}

TEST_CASE("BAND requires integers", "[bitwise]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1.0 1 BAND"));
}
