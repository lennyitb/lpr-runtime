#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"

using namespace lpr;

static Context make_ctx() { return Context(nullptr); }

// --- IF / THEN / END ---

TEST_CASE("IF THEN END - true", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 IF 1 THEN 42 END"));
    REQUIRE(ctx.depth() == 2);
    REQUIRE(ctx.repr_at(1) == "42");
}

TEST_CASE("IF THEN END - false", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 IF 0 THEN 42 END"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "1");
}

TEST_CASE("IF THEN ELSE END - true branch", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("IF 1 THEN 10 ELSE 20 END"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "10");
}

TEST_CASE("IF THEN ELSE END - false branch", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("IF 0 THEN 10 ELSE 20 END"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "20");
}

TEST_CASE("Nested IF", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("IF 1 THEN IF 1 THEN 99 END END"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "99");
}

// --- CASE ---

TEST_CASE("CASE - first match", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("CASE 1 THEN 10 END 0 THEN 20 END END"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "10");
}

TEST_CASE("CASE - second match", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("CASE 0 THEN 10 END 1 THEN 20 END END"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "20");
}

TEST_CASE("CASE - default clause", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("CASE 0 THEN 10 END 0 THEN 20 END 99 END"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "99");
}

TEST_CASE("CASE - no match no default", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("CASE 0 THEN 10 END 0 THEN 20 END END"));
    REQUIRE(ctx.depth() == 0);
}

// --- FOR / NEXT ---

TEST_CASE("FOR NEXT - basic", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 5 FOR I I NEXT"));
    REQUIRE(ctx.depth() == 5);
    REQUIRE(ctx.repr_at(5) == "1");
    REQUIRE(ctx.repr_at(1) == "5");
}

TEST_CASE("FOR NEXT - zero iterations", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("5 1 FOR I I NEXT"));
    REQUIRE(ctx.depth() == 0);
}

TEST_CASE("FOR NEXT - single iteration", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("3 3 FOR I I NEXT"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "3");
}

// --- FOR / STEP ---

TEST_CASE("FOR STEP - step by 2", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 10 FOR I I 2 STEP"));
    REQUIRE(ctx.depth() == 5);
    REQUIRE(ctx.repr_at(5) == "1");
    REQUIRE(ctx.repr_at(4) == "3");
    REQUIRE(ctx.repr_at(1) == "9");
}

TEST_CASE("FOR STEP - negative step", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("5 1 FOR I I -1 STEP"));
    REQUIRE(ctx.depth() == 5);
    REQUIRE(ctx.repr_at(5) == "5");
    REQUIRE(ctx.repr_at(1) == "1");
}

// --- START / NEXT ---

TEST_CASE("START NEXT - basic", "[control-flow]") {
    auto ctx = make_ctx();
    // Push 0, then count 1 to 3, adding 1 each time
    REQUIRE(ctx.exec("0 1 3 START 1 + NEXT"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "3");
}

TEST_CASE("START NEXT - zero iterations", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("99 5 1 START 42 NEXT"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "99");
}

// --- START / STEP ---

TEST_CASE("START STEP - step by 2", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 1 5 START 1 + 2 STEP"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "3");
}

// --- WHILE / REPEAT / END ---

TEST_CASE("WHILE REPEAT END - countdown", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("5 WHILE DUP 0 > REPEAT 1 - END"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "0");
}

TEST_CASE("WHILE - false on first iteration", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 WHILE DUP 0 > REPEAT 1 - END"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "0");
}

// --- DO / UNTIL / END ---

TEST_CASE("DO UNTIL END - basic", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("5 DO 1 - DUP 0 == UNTIL END"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "0");
}

TEST_CASE("DO UNTIL - executes at least once", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 DO 1 + 1 UNTIL END"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "1");
}

// --- Nesting ---

TEST_CASE("Nested FOR in IF", "[control-flow]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("IF 1 THEN 0 1 3 FOR I I + NEXT END"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "6"); // 0+1+2+3
}

TEST_CASE("IF inside FOR", "[control-flow]") {
    auto ctx = make_ctx();
    // Push even numbers 1-4
    REQUIRE(ctx.exec("1 4 FOR I IF I 2 MOD 0 == THEN I END NEXT"));
    REQUIRE(ctx.depth() == 2);
    REQUIRE(ctx.repr_at(2) == "2");
    REQUIRE(ctx.repr_at(1) == "4");
}

TEST_CASE("FOR inside WHILE", "[control-flow]") {
    auto ctx = make_ctx();
    // WHILE counts down from 2, FOR sums 1..2 each iteration
    // Each WHILE iteration: push sum of 1+2=3 via FOR
    REQUIRE(ctx.exec("2 WHILE DUP 0 > REPEAT 0 1 2 FOR I I + NEXT SWAP 1 - END DROP"));
    // Two WHILE iterations produce two sums of 3
    REQUIRE(ctx.depth() == 2);
    REQUIRE(ctx.repr_at(1) == "3");
    REQUIRE(ctx.repr_at(2) == "3");
}
