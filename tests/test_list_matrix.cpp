#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"
#include "core/parser.hpp"

using namespace lpr;

static Context make_ctx() { return Context(nullptr); }

// ==================== Parser ====================

TEST_CASE("Parse simple list", "[list][parser]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 }"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "{ 1 2 3 }");
}

TEST_CASE("Parse nested list", "[list][parser]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 { 2 3 } 4 }"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "{ 1 { 2 3 } 4 }");
}

TEST_CASE("Parse empty list", "[list][parser]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ }"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "{  }");
}

TEST_CASE("Parse heterogeneous list", "[list][parser]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 \"hello\" 3.14 }"));
    REQUIRE(ctx.repr_at(1) == "{ 1 \"hello\" 3.14 }");
}

TEST_CASE("Parse inner body tokens", "[list][parser]") {
    auto tokens = parse("1 { 2 3 } 4");
    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0].kind == Token::Literal);
    REQUIRE(repr(tokens[0].literal) == "1");
    REQUIRE(tokens[1].kind == Token::Literal);
    REQUIRE(repr(tokens[1].literal) == "{ 2 3 }");
    REQUIRE(tokens[2].kind == Token::Literal);
    REQUIRE(repr(tokens[2].literal) == "4");
}

TEST_CASE("Parse simple matrix", "[matrix][parser]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 ][ 3 4 ]]"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "[[ 1 2 ][ 3 4 ]]");
}

TEST_CASE("Parse vector (1-row matrix)", "[matrix][parser]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 3 ]]"));
    REQUIRE(ctx.repr_at(1) == "[[ 1 2 3 ]]");
}

TEST_CASE("Matrix row length validation", "[matrix][parser]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("[[ 1 2 ][ 3 ]]"));
}

TEST_CASE("Matrix element type validation", "[matrix][parser]") {
    auto ctx = make_ctx();
    // Symbolic elements allowed
    REQUIRE(ctx.exec("[[ 'x' 2 ][ 3 'y' ]]"));
    REQUIRE(ctx.repr_at(1) == "[[ 'x' 2 ][ 3 'y' ]]");
}

// ==================== Serialization roundtrip ====================

TEST_CASE("List serialization roundtrip", "[list][serialization]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 }"));
    REQUIRE(ctx.exec("'mylist' STO"));
    REQUIRE(ctx.exec("'mylist' RCL"));
    REQUIRE(ctx.repr_at(1) == "{ 1 2 3 }");
}

TEST_CASE("Nested list serialization roundtrip", "[list][serialization]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 { 2 3 } 4 }"));
    REQUIRE(ctx.exec("'nested' STO"));
    REQUIRE(ctx.exec("'nested' RCL"));
    REQUIRE(ctx.repr_at(1) == "{ 1 { 2 3 } 4 }");
}

TEST_CASE("Matrix serialization roundtrip", "[matrix][serialization]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 ][ 3 4 ]]"));
    REQUIRE(ctx.exec("'mat' STO"));
    REQUIRE(ctx.exec("'mat' RCL"));
    REQUIRE(ctx.repr_at(1) == "[[ 1 2 ][ 3 4 ]]");
}

// ==================== Core List Commands ====================

TEST_CASE("LIST-> explode", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 10 20 30 } LIST->"));
    REQUIRE(ctx.depth() == 4);
    REQUIRE(ctx.repr_at(1) == "3");   // count
    REQUIRE(ctx.repr_at(2) == "30");
    REQUIRE(ctx.repr_at(3) == "20");
    REQUIRE(ctx.repr_at(4) == "10");
}

TEST_CASE("->LIST collect", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 30 3 ->LIST"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "{ 10 20 30 }");
}

TEST_CASE("GET from list", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 10 20 30 } 2 GET"));
    REQUIRE(ctx.repr_at(1) == "20");
}

TEST_CASE("PUT into list", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 10 20 30 } 2 99 PUT"));
    REQUIRE(ctx.repr_at(1) == "{ 10 99 30 }");
}

TEST_CASE("GETI auto-increment", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 10 20 30 } 1 GETI"));
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.repr_at(1) == "10");  // element
    REQUIRE(ctx.repr_at(2) == "2");   // next index
}

TEST_CASE("PUTI auto-increment", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 10 20 30 } 1 99 PUTI"));
    REQUIRE(ctx.depth() == 2);
    REQUIRE(ctx.repr_at(2) == "{ 99 20 30 }");  // modified list
    REQUIRE(ctx.repr_at(1) == "2");              // next index
}

TEST_CASE("HEAD of list", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 10 20 30 } HEAD"));
    REQUIRE(ctx.repr_at(1) == "10");
}

TEST_CASE("TAIL of list", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 10 20 30 } TAIL"));
    REQUIRE(ctx.repr_at(1) == "{ 20 30 }");
}

TEST_CASE("SIZE of list", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 10 20 30 } SIZE"));
    REQUIRE(ctx.repr_at(1) == "3");
}

TEST_CASE("POS finds element", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 10 20 30 } 20 POS"));
    REQUIRE(ctx.repr_at(1) == "2");
}

TEST_CASE("POS returns 0 when not found", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 10 20 30 } 99 POS"));
    REQUIRE(ctx.repr_at(1) == "0");
}

TEST_CASE("SUB of list", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 10 20 30 40 50 } 2 4 SUB"));
    REQUIRE(ctx.repr_at(1) == "{ 20 30 40 }");
}

TEST_CASE("REVLIST", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 } REVLIST"));
    REQUIRE(ctx.repr_at(1) == "{ 3 2 1 }");
}

TEST_CASE("SORT numeric list", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 5 3 1 4 2 } SORT"));
    REQUIRE(ctx.repr_at(1) == "{ 1 2 3 4 5 }");
}

TEST_CASE("SORT string list", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ \"cherry\" \"apple\" \"banana\" } SORT"));
    REQUIRE(ctx.repr_at(1) == "{ \"apple\" \"banana\" \"cherry\" }");
}

TEST_CASE("ADD element to list", "[list][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 } 3 ADD"));
    REQUIRE(ctx.repr_at(1) == "{ 1 2 3 }");
}

// ==================== Higher-order List Commands ====================

TEST_CASE("MAP", "[list][higher-order]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 } << 2 * >> MAP"));
    REQUIRE(ctx.repr_at(1) == "{ 2 4 6 }");
}

TEST_CASE("FILTER", "[list][higher-order]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 4 5 } << 2 MOD >> FILTER"));
    REQUIRE(ctx.repr_at(1) == "{ 1 3 5 }");
}

TEST_CASE("STREAM reduce", "[list][higher-order]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 4 } << + >> STREAM"));
    REQUIRE(ctx.repr_at(1) == "10");
}

TEST_CASE("DOLIST two lists", "[list][higher-order]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 } { 10 20 30 } 2 << + >> DOLIST"));
    REQUIRE(ctx.repr_at(1) == "{ 11 22 33 }");
}

TEST_CASE("SEQ", "[list][higher-order]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 1 5 << 2 * >> SEQ"));
    REQUIRE(ctx.repr_at(1) == "{ 2 4 6 8 10 }");
}

TEST_CASE("DOSUBS sliding window", "[list][higher-order]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 4 } 2 << + >> DOSUBS"));
    REQUIRE(ctx.repr_at(1) == "{ 3 5 7 }");
}

TEST_CASE("ZIP", "[list][higher-order]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 } { 4 5 6 } 2 ZIP"));
    REQUIRE(ctx.repr_at(1) == "{ { 1 4 } { 2 5 } { 3 6 } }");
}

// ==================== Set Operations ====================

TEST_CASE("UNION", "[list][set]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 } { 2 3 4 } UNION"));
    REQUIRE(ctx.repr_at(1) == "{ 1 2 3 4 }");
}

TEST_CASE("INTERSECT", "[list][set]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 } { 2 3 4 } INTERSECT"));
    REQUIRE(ctx.repr_at(1) == "{ 2 3 }");
}

TEST_CASE("DIFFERENCE", "[list][set]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 } { 2 3 4 } DIFFERENCE"));
    REQUIRE(ctx.repr_at(1) == "{ 1 }");
}

// ==================== Matrix/Vector Commands ====================

TEST_CASE("->V2", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("3 4 ->V2"));
    REQUIRE(ctx.repr_at(1) == "[[ 3 4 ]]");
}

TEST_CASE("->V3", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 2 3 ->V3"));
    REQUIRE(ctx.repr_at(1) == "[[ 1 2 3 ]]");
}

TEST_CASE("V-> explode vector", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 3 4 5 ]] V->"));
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.repr_at(3) == "3");
    REQUIRE(ctx.repr_at(2) == "4");
    REQUIRE(ctx.repr_at(1) == "5");
}

TEST_CASE("CON constant vector", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 3 } 0 CON"));
    REQUIRE(ctx.repr_at(1) == "[[ 0 0 0 ]]");
}

TEST_CASE("CON constant matrix", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 2 3 } 1 CON"));
    REQUIRE(ctx.repr_at(1) == "[[ 1 1 1 ][ 1 1 1 ]]");
}

TEST_CASE("IDN identity matrix", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("3 IDN"));
    REQUIRE(ctx.repr_at(1) == "[[ 1 0 0 ][ 0 1 0 ][ 0 0 1 ]]");
}

TEST_CASE("TRN transpose", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 3 ][ 4 5 6 ]] TRN"));
    REQUIRE(ctx.repr_at(1) == "[[ 1 4 ][ 2 5 ][ 3 6 ]]");
}

TEST_CASE("DET 2x2", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 ][ 3 4 ]] DET"));
    REQUIRE(ctx.repr_at(1) == "-2");
}

TEST_CASE("DET 3x3", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 3 ][ 4 5 6 ][ 7 8 0 ]] DET"));
    REQUIRE(ctx.repr_at(1) == "27");
}

TEST_CASE("CROSS product", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 0 0 ]] [[ 0 1 0 ]] CROSS"));
    REQUIRE(ctx.repr_at(1) == "[[ 0 0 1 ]]");
}

TEST_CASE("DOT product", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 3 ]] [[ 4 5 6 ]] DOT"));
    REQUIRE(ctx.repr_at(1) == "32");
}

TEST_CASE("ABS on vector (Euclidean norm)", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 3 4 ]] ABS"));
    REQUIRE(ctx.repr_at(1) == "5.");
}

TEST_CASE("GET from matrix", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 ][ 3 4 ]] { 2 1 } GET"));
    REQUIRE(ctx.repr_at(1) == "3");
}

TEST_CASE("PUT into matrix", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 ][ 3 4 ]] { 1 2 } 99 PUT"));
    REQUIRE(ctx.repr_at(1) == "[[ 1 99 ][ 3 4 ]]");
}

TEST_CASE("SIZE of matrix", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 3 ][ 4 5 6 ]] SIZE"));
    REQUIRE(ctx.repr_at(1) == "{ 2 3 }");
}

TEST_CASE("RDM redimension", "[matrix][commands]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 3 4 5 6 ]] { 2 3 } RDM"));
    REQUIRE(ctx.repr_at(1) == "[[ 1 2 3 ][ 4 5 6 ]]");
}

// ==================== Arithmetic Overloads ====================

TEST_CASE("List + List", "[list][arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 } { 4 5 6 } +"));
    REQUIRE(ctx.repr_at(1) == "{ 5 7 9 }");
}

TEST_CASE("Scalar + List", "[list][arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 { 1 2 3 } +"));
    REQUIRE(ctx.repr_at(1) == "{ 11 12 13 }");
}

TEST_CASE("List + Scalar", "[list][arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 } 10 +"));
    REQUIRE(ctx.repr_at(1) == "{ 11 12 13 }");
}

TEST_CASE("List - List", "[list][arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 10 20 30 } { 1 2 3 } -"));
    REQUIRE(ctx.repr_at(1) == "{ 9 18 27 }");
}

TEST_CASE("List * List", "[list][arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 2 3 4 } { 5 6 7 } *"));
    REQUIRE(ctx.repr_at(1) == "{ 10 18 28 }");
}

TEST_CASE("List / List", "[list][arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 10 20 30 } { 2 4 5 } /"));
    REQUIRE(ctx.repr_at(1) == "{ 5 5 6 }");
}

TEST_CASE("NEG on list", "[list][arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 -2 3 } NEG"));
    REQUIRE(ctx.repr_at(1) == "{ -1 2 -3 }");
}

TEST_CASE("Matrix + Matrix", "[matrix][arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 ][ 3 4 ]] [[ 5 6 ][ 7 8 ]] +"));
    REQUIRE(ctx.repr_at(1) == "[[ 6 8 ][ 10 12 ]]");
}

TEST_CASE("Matrix * Matrix", "[matrix][arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 ][ 3 4 ]] [[ 5 6 ][ 7 8 ]] *"));
    REQUIRE(ctx.repr_at(1) == "[[ 19 22 ][ 43 50 ]]");
}

TEST_CASE("Scalar * Matrix", "[matrix][arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("2 [[ 1 2 ][ 3 4 ]] *"));
    REQUIRE(ctx.repr_at(1) == "[[ 2 4 ][ 6 8 ]]");
}

TEST_CASE("Matrix * Vector", "[matrix][arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 ][ 3 4 ]] [[ 5 6 ]] *"));
    REQUIRE(ctx.repr_at(1) == "[[ 17 39 ]]");
}

TEST_CASE("NEG on matrix", "[matrix][arithmetic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 -2 ][ 3 -4 ]] NEG"));
    REQUIRE(ctx.repr_at(1) == "[[ -1 2 ][ -3 4 ]]");
}

// ==================== Symbolic matrix operations ====================

TEST_CASE("DET symbolic 2x2", "[matrix][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 'a' 'b' ][ 'c' 'd' ]] DET"));
    // a*d - b*c
    REQUIRE(ctx.repr_at(1) == "'a*d-b*c'");
}

TEST_CASE("TRN symbolic matrix", "[matrix][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 'a' 'b' ][ 'c' 'd' ]] TRN"));
    REQUIRE(ctx.repr_at(1) == "[[ 'a' 'c' ][ 'b' 'd' ]]");
}

TEST_CASE("Symbolic matrix arithmetic", "[matrix][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 'a' 'b' ]] [[ 'c' 'd' ]] +"));
    REQUIRE(ctx.repr_at(1) == "[[ 'a+c' 'b+d' ]]");
}

TEST_CASE("CROSS symbolic", "[matrix][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 'a' 'b' 'c' ]] [[ 'd' 'e' 'f' ]] CROSS"));
    // cross = [b*f-c*e, c*d-a*f, a*e-b*d]
    REQUIRE(ctx.repr_at(1) == "[[ 'b*f-c*e' 'c*d-a*f' 'a*e-b*d' ]]");
}

TEST_CASE("DOT symbolic", "[matrix][symbolic]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 'a' 'b' ]] [[ 'c' 'd' ]] DOT"));
    REQUIRE(ctx.repr_at(1) == "'a*c+b*d'");
}

// ==================== Integration ====================

TEST_CASE("STO/RCL with List", "[list][integration]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 } 'mylist' STO"));
    REQUIRE(ctx.exec("'mylist' RCL"));
    REQUIRE(ctx.repr_at(1) == "{ 1 2 3 }");
}

TEST_CASE("STO/RCL with Matrix", "[matrix][integration]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 ][ 3 4 ]] 'mymat' STO"));
    REQUIRE(ctx.exec("'mymat' RCL"));
    REQUIRE(ctx.repr_at(1) == "[[ 1 2 ][ 3 4 ]]");
}

TEST_CASE("DUP with List", "[list][integration]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 } DUP"));
    REQUIRE(ctx.depth() == 2);
    REQUIRE(ctx.repr_at(1) == "{ 1 2 3 }");
    REQUIRE(ctx.repr_at(2) == "{ 1 2 3 }");
}

TEST_CASE("TYPE returns correct tag for List", "[list][integration]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 } TYPE"));
    REQUIRE(ctx.repr_at(1) == "9");
}

TEST_CASE("TYPE returns correct tag for Matrix", "[matrix][integration]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("[[ 1 2 ][ 3 4 ]] TYPE"));
    REQUIRE(ctx.repr_at(1) == "10");
}

TEST_CASE("Undo/redo with List on stack", "[list][integration]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("{ 1 2 3 }"));
    REQUIRE(ctx.exec("{ 4 5 6 }"));
    REQUIRE(ctx.depth() == 2);
    REQUIRE(ctx.undo());
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "{ 1 2 3 }");
    REQUIRE(ctx.redo());
    REQUIRE(ctx.depth() == 2);
}

TEST_CASE("INV numeric matrix", "[matrix][commands]") {
    auto ctx = make_ctx();
    // Inverse of 2x2 identity is itself
    REQUIRE(ctx.exec("2 IDN INV"));
    REQUIRE(ctx.repr_at(1) == "[[ 1. 0. ][ 0. 1. ]]");
}
