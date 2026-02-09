#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"
#include "core/expression.hpp"

using namespace lpr;

// --- Basic arithmetic ---

TEST_CASE("Expression: simple addition", "[expression]") {
    Context ctx(nullptr);
    Object r = eval_expression("2+3", ctx);
    REQUIRE(std::holds_alternative<Integer>(r));
    REQUIRE(std::get<Integer>(r) == 5);
}

TEST_CASE("Expression: subtraction", "[expression]") {
    Context ctx(nullptr);
    Object r = eval_expression("10-4", ctx);
    REQUIRE(std::get<Integer>(r) == 6);
}

TEST_CASE("Expression: multiplication", "[expression]") {
    Context ctx(nullptr);
    Object r = eval_expression("6*7", ctx);
    REQUIRE(std::get<Integer>(r) == 42);
}

TEST_CASE("Expression: division produces rational", "[expression]") {
    Context ctx(nullptr);
    Object r = eval_expression("7/2", ctx);
    REQUIRE(std::holds_alternative<Rational>(r));
    Rational expected(Integer(7), Integer(2));
    REQUIRE(std::get<Rational>(r) == expected);
}

TEST_CASE("Expression: power", "[expression]") {
    Context ctx(nullptr);
    Object r = eval_expression("2^10", ctx);
    // Power promotes to Real
    REQUIRE(std::holds_alternative<Real>(r));
    REQUIRE(std::get<Real>(r) == Real(1024));
}

// --- Precedence ---

TEST_CASE("Expression: multiplication before addition", "[expression]") {
    Context ctx(nullptr);
    Object r = eval_expression("2+3*4", ctx);
    REQUIRE(std::get<Integer>(r) == 14);
}

TEST_CASE("Expression: power before multiplication", "[expression]") {
    Context ctx(nullptr);
    // 2*3^2 = 2*9 = 18
    Object r = eval_expression("2*3^2", ctx);
    REQUIRE(std::holds_alternative<Real>(r));
    REQUIRE(std::get<Real>(r) == Real(18));
}

// --- Parentheses ---

TEST_CASE("Expression: parentheses override precedence", "[expression]") {
    Context ctx(nullptr);
    Object r = eval_expression("(2+3)*4", ctx);
    REQUIRE(std::get<Integer>(r) == 20);
}

TEST_CASE("Expression: nested parentheses", "[expression]") {
    Context ctx(nullptr);
    Object r = eval_expression("((1+2)*(3+4))", ctx);
    REQUIRE(std::get<Integer>(r) == 21);
}

// --- Unary negation ---

TEST_CASE("Expression: unary negation", "[expression]") {
    Context ctx(nullptr);
    Object r = eval_expression("-5+3", ctx);
    REQUIRE(std::get<Integer>(r) == -2);
}

TEST_CASE("Expression: negation in parentheses", "[expression]") {
    Context ctx(nullptr);
    Object r = eval_expression("(-3)*(-4)", ctx);
    REQUIRE(std::get<Integer>(r) == 12);
}

// --- Variables ---

TEST_CASE("Expression: global variable", "[expression]") {
    Context ctx(nullptr);
    ctx.exec("10 'X' STO");
    Object r = eval_expression("X*X", ctx);
    REQUIRE(std::get<Integer>(r) == 100);
}

TEST_CASE("Expression: local variable", "[expression]") {
    Context ctx(nullptr);
    std::unordered_map<std::string, Object> frame;
    frame["A"] = Integer(7);
    ctx.push_locals(frame);
    Object r = eval_expression("A+3", ctx);
    REQUIRE(std::get<Integer>(r) == 10);
    ctx.pop_locals();
}

TEST_CASE("Expression: local shadows global", "[expression]") {
    Context ctx(nullptr);
    ctx.exec("100 'X' STO");
    std::unordered_map<std::string, Object> frame;
    frame["X"] = Integer(5);
    ctx.push_locals(frame);
    Object r = eval_expression("X", ctx);
    REQUIRE(std::get<Integer>(r) == 5);
    ctx.pop_locals();
}

// --- Real numbers ---

TEST_CASE("Expression: real number literal", "[expression]") {
    Context ctx(nullptr);
    Object r = eval_expression("3.14*2", ctx);
    REQUIRE(std::holds_alternative<Real>(r));
}

// --- Spaces ---

TEST_CASE("Expression: with spaces", "[expression]") {
    Context ctx(nullptr);
    Object r = eval_expression("2 + 3 * 4", ctx);
    REQUIRE(std::get<Integer>(r) == 14);
}

// --- Integration: EVAL on Symbol ---

TEST_CASE("EVAL on Symbol: simple arithmetic", "[expression][eval]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("'2+3' EVAL"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "5");
}

TEST_CASE("EVAL on Symbol: with global variable", "[expression][eval]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("5 'X' STO"));
    REQUIRE(ctx.exec("'X^2' EVAL"));
    REQUIRE(ctx.depth() == 1);
    // X^2 = 25, power promotes to Real so repr has trailing dot
    REQUIRE(ctx.repr_at(1) == "25.");
}

TEST_CASE("EVAL on Symbol: with local vars via arrow", "[expression][eval]") {
    Context ctx(nullptr);
    // 3 5 << -> X Y 'X*Y' >> EVAL should push the symbol, EVAL it
    REQUIRE(ctx.exec("3 5 << -> X Y 'X*Y' >> EVAL"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "15");
}

TEST_CASE("EVAL on Symbol: complex expression", "[expression][eval]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("'(2+3)*(4-1)' EVAL"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "15");
}
