#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"

using namespace lpr;

static Context make_ctx() { return Context(nullptr); }

// --- SIZE ---

TEST_CASE("SIZE of string", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"hello\" SIZE"));
    REQUIRE(ctx.repr_at(1) == "5");
}

TEST_CASE("SIZE of empty string", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"\" SIZE"));
    REQUIRE(ctx.repr_at(1) == "0");
}

// --- HEAD / TAIL ---

TEST_CASE("HEAD", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"hello\" HEAD"));
    REQUIRE(ctx.repr_at(1) == "\"h\"");
}

TEST_CASE("TAIL", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"hello\" TAIL"));
    REQUIRE(ctx.repr_at(1) == "\"ello\"");
}

TEST_CASE("TAIL of single char", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"x\" TAIL"));
    REQUIRE(ctx.repr_at(1) == "\"\"");
}

TEST_CASE("HEAD of empty string fails", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("\"\" HEAD"));
}

TEST_CASE("TAIL of empty string fails", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("\"\" TAIL"));
}

// --- SUB ---

TEST_CASE("SUB basic", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"hello\" 2 4 SUB"));
    REQUIRE(ctx.repr_at(1) == "\"ell\"");
}

TEST_CASE("SUB full string", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"abc\" 1 3 SUB"));
    REQUIRE(ctx.repr_at(1) == "\"abc\"");
}

TEST_CASE("SUB out of bounds clamps", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"abc\" 1 10 SUB"));
    REQUIRE(ctx.repr_at(1) == "\"abc\"");
}

TEST_CASE("SUB start > end returns empty", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"abc\" 3 1 SUB"));
    REQUIRE(ctx.repr_at(1) == "\"\"");
}

// --- POS ---

TEST_CASE("POS found", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"hello world\" \"world\" POS"));
    REQUIRE(ctx.repr_at(1) == "7");
}

TEST_CASE("POS not found", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"hello\" \"xyz\" POS"));
    REQUIRE(ctx.repr_at(1) == "0");
}

TEST_CASE("POS at start", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"hello\" \"he\" POS"));
    REQUIRE(ctx.repr_at(1) == "1");
}

// --- REPL ---

TEST_CASE("REPL basic", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"hello world\" \"world\" \"there\" REPL"));
    REQUIRE(ctx.repr_at(1) == "\"hello there\"");
}

TEST_CASE("REPL not found", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"hello\" \"xyz\" \"abc\" REPL"));
    REQUIRE(ctx.repr_at(1) == "\"hello\"");
}

TEST_CASE("REPL replaces only first occurrence", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"aaa\" \"a\" \"b\" REPL"));
    REQUIRE(ctx.repr_at(1) == "\"baa\"");
}

// --- NUM / CHR ---

TEST_CASE("NUM of A = 65", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"A\" NUM"));
    REQUIRE(ctx.repr_at(1) == "65");
}

TEST_CASE("CHR 65 = A", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("65 CHR"));
    REQUIRE(ctx.repr_at(1) == "\"A\"");
}

TEST_CASE("NUM of empty string fails", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("\"\" NUM"));
}

// --- String concatenation via + ---

TEST_CASE("String + String", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"hello\" \" world\" +"));
    REQUIRE(ctx.repr_at(1) == "\"hello world\"");
}

TEST_CASE("String + number fails", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("\"hello\" 5 +"));
}

TEST_CASE("Empty string concatenation", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"\" \"\" +"));
    REQUIRE(ctx.repr_at(1) == "\"\"");
}

// --- SIZE with non-string fails ---

TEST_CASE("SIZE requires string", "[strings]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("42 SIZE"));
}
