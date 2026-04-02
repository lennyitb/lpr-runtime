#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"

using namespace lpr;

static Context make_ctx() { return Context(nullptr); }

// Helper: check that stack top is a Symbol matching expected repr
static void check_symbol(Context& ctx, const std::string& expected) {
    REQUIRE(ctx.depth() >= 1);
    auto actual = ctx.repr_at(1);
    INFO("Expected: " << expected << ", Got: " << actual);
    REQUIRE(actual == expected);
}

// ============================================================
// 2.4: String ↔ SymEngine round-trip conversion tests
// ============================================================

TEST_CASE("DIFF round-trip preserves polynomial", "[cas][conversion]") {
    auto ctx = make_ctx();
    // Differentiating and integrating should give us something sensible
    REQUIRE(ctx.exec("'X^2+1' 'X' DIFF"));
    REQUIRE(ctx.depth() == 1);
    // 2*X
    auto r = ctx.repr_at(1);
    REQUIRE(r == "'2*X'");
}

TEST_CASE("EXPAND round-trip preserves expression", "[cas][conversion]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'3*X' EXPAND"));
    check_symbol(ctx, "'3*X'");
}

// ============================================================
// 3.7: Bridge operation unit tests
// ============================================================

TEST_CASE("DIFF: polynomial differentiation", "[cas][diff]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X^2+3*X+1' 'X' DIFF"));
    check_symbol(ctx, "'3 + 2*X'");
}

TEST_CASE("DIFF: trigonometric differentiation", "[cas][diff]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'SIN(X)' 'X' DIFF"));
    check_symbol(ctx, "'COS(X)'");
}

TEST_CASE("DIFF: chain rule", "[cas][diff]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'SIN(X^2)' 'X' DIFF"));
    auto r = ctx.repr_at(1);
    // Should contain 2*X*COS(X^2) in some form
    REQUIRE(r.find("COS") != std::string::npos);
    REQUIRE(r.find("X") != std::string::npos);
}

TEST_CASE("DIFF: constant expression", "[cas][diff]") {
    auto ctx = make_ctx();
    // '5+0' ensures the parser creates a Symbol (has an operator)
    REQUIRE(ctx.exec("'5+0' 'X' DIFF"));
    check_symbol(ctx, "'0'");
}

TEST_CASE("INTEGRATE: polynomial", "[cas][integrate]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'2*X+3' 'X' INTEGRATE"));
    auto r = ctx.repr_at(1);
    // Should be X^2 + 3*X (or equivalent)
    REQUIRE(r.find("X^2") != std::string::npos);
    REQUIRE(r.find("3*X") != std::string::npos);
}

TEST_CASE("INTEGRATE: cos(x) → sin(x)", "[cas][integrate]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'COS(X)' 'X' INTEGRATE"));
    check_symbol(ctx, "'SIN(X)'");
}

TEST_CASE("EXPAND: binomial", "[cas][expand]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'(X+1)^2' EXPAND"));
    auto r = ctx.repr_at(1);
    REQUIRE(r.find("X^2") != std::string::npos);
    REQUIRE(r.find("2*X") != std::string::npos);
}

TEST_CASE("EXPAND: distribution", "[cas][expand]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'(X+1)*(X-1)' EXPAND"));
    auto r = ctx.repr_at(1);
    // X^2 - 1
    REQUIRE(r.find("X^2") != std::string::npos);
}

TEST_CASE("EXPAND: cubic", "[cas][expand]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'(X+1)^3' EXPAND"));
    auto r = ctx.repr_at(1);
    REQUIRE(r.find("X^3") != std::string::npos);
    REQUIRE(r.find("3*X^2") != std::string::npos);
    REQUIRE(r.find("3*X") != std::string::npos);
}

TEST_CASE("SOLVE: quadratic with integer solutions", "[cas][solve]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X^2-4' 'X' SOLVE"));
    REQUIRE(ctx.depth() == 1);
    auto r = ctx.repr_at(1);
    // Should be a list containing -2 and 2
    REQUIRE(r.find("2") != std::string::npos);
    REQUIRE(r.find("-2") != std::string::npos);
}

TEST_CASE("SOLVE: linear equation", "[cas][solve]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'2*X-6' 'X' SOLVE"));
    auto r = ctx.repr_at(1);
    REQUIRE(r.find("3") != std::string::npos);
}

TEST_CASE("SOLVE: rational solution", "[cas][solve]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'3*X-1' 'X' SOLVE"));
    auto r = ctx.repr_at(1);
    REQUIRE(r.find("1/3") != std::string::npos);
}

TEST_CASE("SIMPLIFY: combine like terms", "[cas][simplify]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X+X' SIMPLIFY"));
    check_symbol(ctx, "'2*X'");
}

TEST_CASE("SIMPLIFY: cancel common factors", "[cas][simplify]") {
    auto ctx = make_ctx();
    // X^2/X should simplify to X
    REQUIRE(ctx.exec("'X^2/X' SIMPLIFY"));
    check_symbol(ctx, "'X'");
}

TEST_CASE("FACTOR: difference of squares", "[cas][factor]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X^2-4' FACTOR"));
    auto r = ctx.repr_at(1);
    // Should contain (X-2) and (X+2) in some form
    REQUIRE(r.find("X") != std::string::npos);
    REQUIRE(r.find("2") != std::string::npos);
    // Verify by expanding
    REQUIRE(ctx.exec("EXPAND"));
    auto expanded = ctx.repr_at(1);
    REQUIRE(expanded.find("X^2") != std::string::npos);
}

TEST_CASE("FACTOR: perfect square", "[cas][factor]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X^2+2*X+1' FACTOR"));
    auto r = ctx.repr_at(1);
    // Should be (X+1)^2 or equivalent
    REQUIRE(r.find("X") != std::string::npos);
    REQUIRE(r.find("^2") != std::string::npos);
}

TEST_CASE("FACTOR: irreducible over integers", "[cas][factor]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X^2+1' FACTOR"));
    auto r = ctx.repr_at(1);
    // Should remain X^2+1 (not factored)
    REQUIRE(r.find("X^2") != std::string::npos);
}

TEST_CASE("FACTOR: common factor extraction", "[cas][factor]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'2*X^2+4*X' FACTOR"));
    auto r = ctx.repr_at(1);
    // Should contain 2*X*(X+2) or similar
    REQUIRE(r.find("2") != std::string::npos);
    REQUIRE(r.find("X") != std::string::npos);
}

// ============================================================
// 3.8: Error cases
// ============================================================

TEST_CASE("DIFF: wrong type at level 2", "[cas][error]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("42 'X' DIFF"));
    // Error should be on stack
    REQUIRE(ctx.depth() >= 1);
    auto r = ctx.repr_at(1);
    REQUIRE(r.find("Error") != std::string::npos);
}

TEST_CASE("SIMPLIFY: wrong type", "[cas][error]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("\"hello\" SIMPLIFY"));
    REQUIRE(ctx.depth() >= 1);
    auto r = ctx.repr_at(1);
    REQUIRE(r.find("Error") != std::string::npos);
}

// ============================================================
// 5.7: End-to-end via lpr_exec
// ============================================================

TEST_CASE("DIFF end-to-end: X^2+3*X → 2*X+3", "[cas][e2e]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X^2+3*X' 'X' DIFF"));
    REQUIRE(ctx.depth() == 1);
    check_symbol(ctx, "'3 + 2*X'");
}

TEST_CASE("EXPAND end-to-end: (X+1)^2 → X^2+2*X+1", "[cas][e2e]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'(X+1)^2' EXPAND"));
    REQUIRE(ctx.depth() == 1);
    auto r = ctx.repr_at(1);
    REQUIRE(r.find("X^2") != std::string::npos);
}

TEST_CASE("SOLVE end-to-end: X^2-9 → {-3, 3}", "[cas][e2e]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X^2-9' 'X' SOLVE"));
    REQUIRE(ctx.depth() == 1);
    auto r = ctx.repr_at(1);
    REQUIRE(r.find("3") != std::string::npos);
    REQUIRE(r.find("-3") != std::string::npos);
}

// ============================================================
// 5.8: Type error cases
// ============================================================

TEST_CASE("DIFF on Integer fails", "[cas][error]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("42 'X' DIFF"));
}

TEST_CASE("SIMPLIFY on String fails", "[cas][error]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("\"hello\" SIMPLIFY"));
}

TEST_CASE("SOLVE with non-Name variable fails", "[cas][error]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("'X^2' 42 SOLVE"));
}

// ============================================================
// 5.9: Stack underflow cases
// ============================================================

TEST_CASE("DIFF underflow", "[cas][error]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("'X^2' DIFF"));
}

TEST_CASE("INTEGRATE underflow", "[cas][error]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("INTEGRATE"));
}

TEST_CASE("SOLVE underflow", "[cas][error]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("SOLVE"));
}

TEST_CASE("SIMPLIFY underflow", "[cas][error]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("SIMPLIFY"));
}

TEST_CASE("EXPAND underflow", "[cas][error]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("EXPAND"));
}

TEST_CASE("FACTOR underflow", "[cas][error]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("FACTOR"));
}

// ============================================================
// 6.1–6.6: Integration tests
// ============================================================

TEST_CASE("Multi-step CAS: EXPAND then DIFF", "[cas][integration]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'(X+1)^3' EXPAND 'X' DIFF"));
    REQUIRE(ctx.depth() == 1);
    auto r = ctx.repr_at(1);
    // Derivative of X^3+3*X^2+3*X+1 = 3*X^2+6*X+3
    REQUIRE(r.find("X^2") != std::string::npos);
}

TEST_CASE("FACTOR then EXPAND round-trip", "[cas][integration]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X^2-4' FACTOR EXPAND"));
    REQUIRE(ctx.depth() == 1);
    auto r = ctx.repr_at(1);
    // Should come back as X^2-4 (or equivalent -4 + X^2)
    REQUIRE(r.find("X^2") != std::string::npos);
    REQUIRE(r.find("4") != std::string::npos);
}

TEST_CASE("SOLVE returns list usable with SIZE", "[cas][integration]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X^2-9' 'X' SOLVE SIZE"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "2");
}

TEST_CASE("Undo restores pre-DIFF state", "[cas][integration]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X^2+1'"));
    auto before = ctx.repr_at(1);
    REQUIRE(ctx.exec("'X' DIFF"));
    REQUIRE(ctx.repr_at(1) != before);
    REQUIRE(ctx.undo());
    // After undo, the DIFF arguments should be on the stack
    // (undo goes to the state before the DIFF exec)
    // The pre-state of the DIFF exec had 'X^2+1' and 'X' on stack
    // Actually undo goes back to before the second exec call
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == before);
}
