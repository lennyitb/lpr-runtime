#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"
#include "core/parser.hpp"

using namespace lpr;

// --- Parser tests ---

TEST_CASE("Parser recognizes -> as command inside program", "[arrow][parser]") {
    auto tokens = parse("<< -> X Y 'X*Y' >>");
    REQUIRE(tokens.size() == 1);
    REQUIRE(std::holds_alternative<Program>(tokens[0].literal));
    auto& prog = std::get<Program>(tokens[0].literal);
    // Tokens should be: -> X Y 'X*Y'
    REQUIRE(prog.tokens.size() == 4);
    REQUIRE(prog.tokens[0].kind == Token::Command);
    REQUIRE(prog.tokens[0].command == "->");
    REQUIRE(prog.tokens[1].kind == Token::Command);
    REQUIRE(prog.tokens[1].command == "X");
    REQUIRE(prog.tokens[2].kind == Token::Command);
    REQUIRE(prog.tokens[2].command == "Y");
    REQUIRE(prog.tokens[3].kind == Token::Literal);
    REQUIRE(std::holds_alternative<Symbol>(prog.tokens[3].literal));
}

TEST_CASE("Parser recognizes UTF-8 arrow as command inside program", "[arrow][parser]") {
    auto tokens = parse("<< \xe2\x86\x92 X Y << X Y * >> >>");
    REQUIRE(tokens.size() == 1);
    REQUIRE(std::holds_alternative<Program>(tokens[0].literal));
    auto& prog = std::get<Program>(tokens[0].literal);
    // Tokens should be: → X Y <program>
    REQUIRE(prog.tokens.size() == 4);
    REQUIRE(prog.tokens[0].kind == Token::Command);
    // UTF-8 arrow command (uppercased bytes stay the same)
    REQUIRE(prog.tokens[0].command == "\xe2\x86\x92");
    REQUIRE(prog.tokens[3].kind == Token::Literal);
    REQUIRE(std::holds_alternative<Program>(prog.tokens[3].literal));
}

// --- Execution tests ---

TEST_CASE("Arrow basic binding with program body", "[arrow][exec]") {
    Context ctx(nullptr);
    // 3 5 << -> X Y << X Y * >> >> EVAL => 15
    REQUIRE(ctx.exec("3 5 << -> X Y << X Y * >> >> EVAL"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "15");
}

TEST_CASE("Arrow basic binding with ASCII arrow", "[arrow][exec]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("10 20 << -> A B << A B + >> >> EVAL"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "30");
}

TEST_CASE("Arrow with UTF-8 arrow and program body", "[arrow][exec]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("7 << \xe2\x86\x92 N << N N * >> >> EVAL"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "49");
}

TEST_CASE("Arrow nested scopes", "[arrow][exec]") {
    Context ctx(nullptr);
    // Outer binds X=2, inner binds X=5, inner body uses X (should get 5)
    REQUIRE(ctx.exec("2 << -> X << 5 << -> X << X >> >> EVAL >> >> EVAL"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "5");
}

TEST_CASE("Arrow single variable", "[arrow][exec]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("42 << -> N << N >> >> EVAL"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "42");
}

// --- Name resolution tests ---

TEST_CASE("Local variable takes precedence over global", "[arrow][names]") {
    Context ctx(nullptr);
    // Store X=100 globally, then bind X=5 locally
    REQUIRE(ctx.exec("100 'X' STO"));
    REQUIRE(ctx.exec("5 << -> X << X >> >> EVAL"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "5");
}

TEST_CASE("Unresolved name triggers error (unknown command)", "[arrow][names]") {
    Context ctx(nullptr);
    // ZZZZZ is not a local, not a global, not a command — should error
    bool ok = ctx.exec("ZZZZZ");
    REQUIRE(!ok);
}
