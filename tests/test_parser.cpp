#include <catch2/catch_test_macros.hpp>
#include "core/parser.hpp"

using namespace lpr;

TEST_CASE("Parse integer literals", "[parser]") {
    auto tokens = parse("42");
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].kind == Token::Literal);
    REQUIRE(std::holds_alternative<Integer>(tokens[0].literal));
    REQUIRE(std::get<Integer>(tokens[0].literal) == 42);
}

TEST_CASE("Parse negative integer", "[parser]") {
    auto tokens = parse("-7");
    REQUIRE(tokens.size() == 1);
    REQUIRE(std::holds_alternative<Integer>(tokens[0].literal));
    REQUIRE(std::get<Integer>(tokens[0].literal) == -7);
}

TEST_CASE("Parse real literals", "[parser]") {
    auto tokens = parse("3.14159");
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].kind == Token::Literal);
    REQUIRE(std::holds_alternative<Real>(tokens[0].literal));
}

TEST_CASE("Parse scientific notation", "[parser]") {
    auto tokens = parse("1.5E-10");
    REQUIRE(tokens.size() == 1);
    REQUIRE(std::holds_alternative<Real>(tokens[0].literal));
}

TEST_CASE("Parse complex literal", "[parser]") {
    auto tokens = parse("(3.0, 4.0)");
    REQUIRE(tokens.size() == 1);
    REQUIRE(std::holds_alternative<Complex>(tokens[0].literal));
}

TEST_CASE("Parse string literal", "[parser]") {
    auto tokens = parse("\"hello\"");
    REQUIRE(tokens.size() == 1);
    REQUIRE(std::holds_alternative<String>(tokens[0].literal));
    REQUIRE(std::get<String>(tokens[0].literal).value == "hello");
}

TEST_CASE("Parse quoted name", "[parser]") {
    auto tokens = parse("'myvar'");
    REQUIRE(tokens.size() == 1);
    REQUIRE(std::holds_alternative<Name>(tokens[0].literal));
    REQUIRE(std::get<Name>(tokens[0].literal).value == "myvar");
}

TEST_CASE("Parse quoted expression (Symbol)", "[parser]") {
    auto tokens = parse("'X^2 + 1'");
    REQUIRE(tokens.size() == 1);
    REQUIRE(std::holds_alternative<Symbol>(tokens[0].literal));
    REQUIRE(std::get<Symbol>(tokens[0].literal).value == "X^2 + 1");
}

TEST_CASE("Parse program literal", "[parser]") {
    auto tokens = parse("\xC2\xAB DUP * \xC2\xBB");
    REQUIRE(tokens.size() == 1);
    REQUIRE(std::holds_alternative<Program>(tokens[0].literal));
    auto& prog = std::get<Program>(tokens[0].literal);
    REQUIRE(prog.tokens.size() == 2);
    REQUIRE(prog.tokens[0].command == "DUP");
    REQUIRE(prog.tokens[1].command == "*");
}

TEST_CASE("Parse nested programs", "[parser]") {
    auto tokens = parse("\xC2\xAB 1 \xC2\xAB 2 3 + \xC2\xBB EVAL \xC2\xBB");
    REQUIRE(tokens.size() == 1);
    auto& prog = std::get<Program>(tokens[0].literal);
    REQUIRE(prog.tokens.size() == 3); // 1, inner_prog, EVAL
    REQUIRE(std::holds_alternative<Program>(prog.tokens[1].literal));
}

TEST_CASE("Parse command names are uppercased", "[parser]") {
    auto tokens = parse("dup swap");
    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].kind == Token::Command);
    REQUIRE(tokens[0].command == "DUP");
    REQUIRE(tokens[1].command == "SWAP");
}

TEST_CASE("Parse simple expression", "[parser]") {
    auto tokens = parse("3 4 +");
    REQUIRE(tokens.size() == 3);
    REQUIRE(std::holds_alternative<Integer>(tokens[0].literal));
    REQUIRE(std::holds_alternative<Integer>(tokens[1].literal));
    REQUIRE(tokens[2].kind == Token::Command);
    REQUIRE(tokens[2].command == "+");
}
