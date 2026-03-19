#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"
#include "core/expression.hpp"

using namespace lpr;

static Context make_ctx() { return Context(nullptr); }

// ============================================================
// Phase 1: Symbolic function support
// ============================================================

// --- Unary commands produce symbolic output from symbolic input ---

TEST_CASE("SQ of Symbol produces symbolic output", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X' SQ"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "'SQ(X)'");
}

TEST_CASE("SQRT of Symbol produces symbolic output", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X+1' SQRT"));
    REQUIRE(ctx.repr_at(1) == "'SQRT(X+1)'");
}

TEST_CASE("SIN of Name produces symbolic output", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X' SIN"));
    REQUIRE(ctx.repr_at(1) == "'SIN(X)'");
}

TEST_CASE("COS of Symbol produces symbolic output", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X' COS"));
    REQUIRE(ctx.repr_at(1) == "'COS(X)'");
}

TEST_CASE("TAN of Symbol produces symbolic output", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X' TAN"));
    REQUIRE(ctx.repr_at(1) == "'TAN(X)'");
}

TEST_CASE("ASIN of Symbol produces symbolic output", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X' ASIN"));
    REQUIRE(ctx.repr_at(1) == "'ASIN(X)'");
}

TEST_CASE("ACOS of Symbol produces symbolic output", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X' ACOS"));
    REQUIRE(ctx.repr_at(1) == "'ACOS(X)'");
}

TEST_CASE("ATAN of Symbol produces symbolic output", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X' ATAN"));
    REQUIRE(ctx.repr_at(1) == "'ATAN(X)'");
}

TEST_CASE("EXP of Symbol produces symbolic output", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X' EXP"));
    REQUIRE(ctx.repr_at(1) == "'EXP(X)'");
}

TEST_CASE("LN of Symbol produces symbolic output", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X' LN"));
    REQUIRE(ctx.repr_at(1) == "'LN(X)'");
}

TEST_CASE("ABS of Symbol produces symbolic output", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X' ABS"));
    REQUIRE(ctx.repr_at(1) == "'ABS(X)'");
}

TEST_CASE("INV of Symbol produces symbolic output", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X' INV"));
    REQUIRE(ctx.repr_at(1) == "'INV(X)'");
}

// --- Numeric inputs are unchanged ---

TEST_CASE("SQ of integer unchanged", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("5 SQ"));
    REQUIRE(ctx.repr_at(1) == "25");
}

TEST_CASE("SIN of zero unchanged", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 SIN"));
    // Still produces numeric result
    REQUIRE(ctx.depth() == 1);
}

// --- Multi-arg symbolic output ---

TEST_CASE("IFT symbolic produces symbolic output", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X' 'Y' IFT"));
    REQUIRE(ctx.repr_at(1) == "'IFT(X, Y)'");
}

TEST_CASE("IFTE symbolic produces symbolic output", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'A' 'B' 'C' IFTE"));
    REQUIRE(ctx.repr_at(1) == "'IFTE(A, B, C)'");
}

// ============================================================
// Phase 2: Comma syntax and SUBST
// ============================================================

TEST_CASE("Tokenizer recognizes commas", "[symbolic][expression]") {
    auto tokens = tokenize_expression("FUNC(A, B, C)");
    // Should contain: Name, LParen, Name, Comma, Name, Comma, Name, RParen
    REQUIRE(tokens.size() == 8);
    REQUIRE(tokens[0].type == ExprTokenType::Name);
    REQUIRE(tokens[0].value == "FUNC");
    REQUIRE(tokens[3].type == ExprTokenType::Comma);
}

TEST_CASE("SUBST simple variable replacement", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X+1' 'X' 'Y' SUBST"));
    REQUIRE(ctx.repr_at(1) == "'Y+1'");
}

TEST_CASE("SUBST expression replacement with parens", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X*2' 'X' 'A+B' SUBST"));
    REQUIRE(ctx.repr_at(1) == "'(A+B)*2'");
}

TEST_CASE("SUBST no-match", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X+1' 'Y' 'Z' SUBST"));
    REQUIRE(ctx.repr_at(1) == "'X+1'");
}

TEST_CASE("SUBST multiple occurrences", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'X+X' 'X' 'Y' SUBST"));
    REQUIRE(ctx.repr_at(1) == "'Y+Y'");
}

TEST_CASE("SUBST expression with commas", "[symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'IFTE(X, Y, Z)' 'X' 'A' SUBST"));
    REQUIRE(ctx.repr_at(1) == "'IFTE(A, Y, Z)'");
}

// ============================================================
// Phase 3: Stash infrastructure
// ============================================================

TEST_CASE("Stash push/pop round-trip", "[symbolic][stash]") {
    auto ctx = make_ctx();
    ctx.store().begin();
    ctx.store().stash_push({Integer(42)});
    REQUIRE(ctx.store().stash_depth() == 1);
    auto group = ctx.store().stash_pop();
    REQUIRE(group.size() == 1);
    REQUIRE(std::get<Integer>(group[0]) == 42);
    REQUIRE(ctx.store().stash_depth() == 0);
    ctx.store().commit();
}

TEST_CASE("Stash LIFO ordering", "[symbolic][stash]") {
    auto ctx = make_ctx();
    ctx.store().begin();
    ctx.store().stash_push({Integer(1)});
    ctx.store().stash_push({Integer(2)});
    auto g1 = ctx.store().stash_pop();
    REQUIRE(std::get<Integer>(g1[0]) == 2);
    auto g2 = ctx.store().stash_pop();
    REQUIRE(std::get<Integer>(g2[0]) == 1);
    ctx.store().commit();
}

TEST_CASE("Stash multi-item group", "[symbolic][stash]") {
    auto ctx = make_ctx();
    ctx.store().begin();
    ctx.store().stash_push({Integer(10), Integer(20), Integer(30)});
    REQUIRE(ctx.store().stash_depth() == 1);
    auto group = ctx.store().stash_pop();
    REQUIRE(group.size() == 3);
    REQUIRE(std::get<Integer>(group[0]) == 10);
    REQUIRE(std::get<Integer>(group[1]) == 20);
    REQUIRE(std::get<Integer>(group[2]) == 30);
    ctx.store().commit();
}

TEST_CASE("Stash empty pop throws", "[symbolic][stash]") {
    auto ctx = make_ctx();
    ctx.store().begin();
    REQUIRE_THROWS(ctx.store().stash_pop());
    ctx.store().commit();
}

TEST_CASE("Stash undo/redo preserves stash state", "[symbolic][stash]") {
    auto ctx = make_ctx();
    // Store something on stash via STASH command
    REQUIRE(ctx.exec("42 STASH"));
    // Stash should have one group
    REQUIRE(ctx.store().stash_depth() == 1);
    // Undo should restore stash to empty
    REQUIRE(ctx.undo());
    REQUIRE(ctx.store().stash_depth() == 0);
    // Redo should restore stash back
    REQUIRE(ctx.redo());
    REQUIRE(ctx.store().stash_depth() == 1);
}

// ============================================================
// Phase 4: STASH, STASHN, UNSTASH commands
// ============================================================

TEST_CASE("STASH/UNSTASH round-trip", "[symbolic][stash]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("42 STASH"));
    REQUIRE(ctx.depth() == 0);
    REQUIRE(ctx.exec("UNSTASH"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "42");
}

TEST_CASE("STASHN with various counts", "[symbolic][stash]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 2 3 3 STASHN"));
    REQUIRE(ctx.depth() == 0);
    REQUIRE(ctx.exec("UNSTASH"));
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.repr_at(1) == "3");
    REQUIRE(ctx.repr_at(2) == "2");
    REQUIRE(ctx.repr_at(3) == "1");
}

TEST_CASE("STASH error on empty stack", "[symbolic][stash]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("STASH"));
}

TEST_CASE("UNSTASH error on empty stash", "[symbolic][stash]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("UNSTASH"));
}

TEST_CASE("Undo coherence: STASHN then undo", "[symbolic][stash]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 30"));
    REQUIRE(ctx.exec("3 STASHN"));
    REQUIRE(ctx.depth() == 0);
    REQUIRE(ctx.store().stash_depth() == 1);
    REQUIRE(ctx.undo());
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.store().stash_depth() == 0);
}

// ============================================================
// Phase 5: EXPLODE command
// ============================================================

TEST_CASE("EXPLODE binary op: A+B", "[symbolic][explode]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'A+B' EXPLODE"));
    REQUIRE(ctx.depth() == 3);
    // Stack: level 3=A, level 2=B, level 1=<< + >>
    REQUIRE(ctx.repr_at(3) == "'A'");
    REQUIRE(ctx.repr_at(2) == "'B'");
    REQUIRE(ctx.repr_at(1) == "« + »");
}

TEST_CASE("EXPLODE binary op: X*Y-3", "[symbolic][explode]") {
    auto ctx = make_ctx();
    // lowest prec at depth 0 is - (prec 1), so splits X*Y and 3
    REQUIRE(ctx.exec("'X*Y-3' EXPLODE"));
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.repr_at(3) == "'X*Y'");
    REQUIRE(ctx.repr_at(2) == "3");
    REQUIRE(ctx.repr_at(1) == "« - »");
}

TEST_CASE("EXPLODE unary function: SIN(X)", "[symbolic][explode]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'SIN(X)' EXPLODE"));
    REQUIRE(ctx.depth() == 2);
    REQUIRE(ctx.repr_at(2) == "'X'");
    REQUIRE(ctx.repr_at(1) == "« SIN »");
}

TEST_CASE("EXPLODE multi-arg function: IFTE(A, B, C)", "[symbolic][explode]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'IFTE(A, B, C)' EXPLODE"));
    REQUIRE(ctx.depth() == 4);
    REQUIRE(ctx.repr_at(4) == "'A'");
    REQUIRE(ctx.repr_at(3) == "'B'");
    REQUIRE(ctx.repr_at(2) == "'C'");
    REQUIRE(ctx.repr_at(1) == "« IFTE »");
}

TEST_CASE("EXPLODE nested expression", "[symbolic][explode]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("'SQ(X)+3' EXPLODE"));
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.repr_at(3) == "'SQ(X)'");
    REQUIRE(ctx.repr_at(2) == "3");
    REQUIRE(ctx.repr_at(1) == "« + »");
}

TEST_CASE("EXPLODE atomic expression error", "[symbolic][explode]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("'X' EXPLODE"));
}

// ============================================================
// Phase 6: ASSEMBLE command
// ============================================================

TEST_CASE("ASSEMBLE single-level reassembly", "[symbolic][assemble]") {
    auto ctx = make_ctx();
    // Explode 'A+B', stash operator, swap operands, unstash, assemble
    REQUIRE(ctx.exec("'A+B' EXPLODE"));
    // Stack: A B << + >>
    REQUIRE(ctx.exec("STASH")); // stash << + >>
    // Stack: A B; Stash: [<< + >>]
    REQUIRE(ctx.exec("ASSEMBLE"));
    // UNSTASH pushes << + >>, EVAL executes + on A and B
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "'A+B'");
}

TEST_CASE("ASSEMBLE empty stash is no-op", "[symbolic][assemble]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("42"));
    REQUIRE(ctx.exec("ASSEMBLE"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "42");
}

TEST_CASE("ASSEMBLE multi-level reassembly", "[symbolic][assemble]") {
    auto ctx = make_ctx();
    // Build 'SQ(X)+3': explode, dig into SQ(X), replace X, reassemble
    REQUIRE(ctx.exec("'SQ(X)+3' EXPLODE"));   // 'SQ(X)' 3 << + >>
    REQUIRE(ctx.exec("2 STASHN"));             // stash [3, << + >>]
    REQUIRE(ctx.exec("EXPLODE"));              // 'X' << SQ >>
    REQUIRE(ctx.exec("STASH"));                // stash << SQ >>
    REQUIRE(ctx.exec("DROP 'Y'"));             // replace X with Y
    REQUIRE(ctx.exec("ASSEMBLE"));             // unstash << SQ >> -> SQ(Y), then unstash [3, << + >>] -> SQ(Y)+3
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "'SQ(Y)+3'");
}

// ============================================================
// Phase 7: Integration test
// ============================================================

TEST_CASE("E2E: SQ(X)+3 -> EXPLODE -> STASHN -> EXPLODE -> STASH -> DROP -> ASSEMBLE", "[symbolic][integration]") {
    auto ctx = make_ctx();
    // Start with 'SQ(X)+3'
    REQUIRE(ctx.exec("'SQ(X)+3' EXPLODE"));
    // Stack: 'SQ(X)' 3 << + >>
    REQUIRE(ctx.exec("2 STASHN"));
    // Stack: 'SQ(X)'; Stash: [3, << + >>]
    REQUIRE(ctx.exec("EXPLODE"));
    // Stack: 'X' << SQ >>
    REQUIRE(ctx.exec("STASH"));
    // Stack: 'X'; Stash: [<< SQ >>], [3, << + >>]
    REQUIRE(ctx.exec("DROP"));
    // Stack: (empty)
    REQUIRE(ctx.exec("'Y+1'"));
    // Stack: 'Y+1'
    REQUIRE(ctx.exec("ASSEMBLE"));
    // First unstash: << SQ >> -> SQ(Y+1)
    // Second unstash: 3 << + >> -> SQ(Y+1)+3
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "'SQ(Y+1)+3'");
}
