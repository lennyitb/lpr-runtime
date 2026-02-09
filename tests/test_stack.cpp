#include <catch2/catch_test_macros.hpp>
#include "core/store.hpp"
#include "core/context.hpp"

using namespace lpr;

static Context make_ctx() { return Context(nullptr); }

TEST_CASE("Push and pop", "[stack]") {
    Store store(nullptr);
    store.push(Integer(42));
    REQUIRE(store.depth() == 1);
    Object obj = store.pop();
    REQUIRE(std::holds_alternative<Integer>(obj));
    REQUIRE(std::get<Integer>(obj) == 42);
    REQUIRE(store.depth() == 0);
}

TEST_CASE("Pop empty stack returns error", "[stack]") {
    Store store(nullptr);
    Object obj = store.pop();
    REQUIRE(std::holds_alternative<Error>(obj));
}

TEST_CASE("Peek at arbitrary level", "[stack]") {
    Store store(nullptr);
    store.push(Integer(1));
    store.push(Integer(2));
    store.push(Integer(3));

    // Level 1 = top = 3, level 2 = 2, level 3 = 1
    Object top = store.peek(1);
    REQUIRE(std::get<Integer>(top) == 3);

    Object mid = store.peek(2);
    REQUIRE(std::get<Integer>(mid) == 2);

    Object bot = store.peek(3);
    REQUIRE(std::get<Integer>(bot) == 1);

    // Stack unmodified
    REQUIRE(store.depth() == 3);
}

TEST_CASE("Stack depth", "[stack]") {
    Store store(nullptr);
    REQUIRE(store.depth() == 0);
    store.push(Integer(1));
    store.push(Integer(2));
    store.push(Integer(3));
    REQUIRE(store.depth() == 3);
}

TEST_CASE("Clear stack", "[stack]") {
    Store store(nullptr);
    store.push(Integer(1));
    store.push(Integer(2));
    store.clear_stack();
    REQUIRE(store.depth() == 0);
}

TEST_CASE("Snapshot and restore", "[stack]") {
    Store store(nullptr);
    store.push(Integer(42));
    int seq = store.snapshot_stack();
    store.push(Integer(99));
    REQUIRE(store.depth() == 2);
    store.restore_stack(seq);
    REQUIRE(store.depth() == 1);
    Object obj = store.peek(1);
    REQUIRE(std::get<Integer>(obj) == 42);
}

// ---- Stack Command Integration Tests ----

TEST_CASE("UNROT command", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 2 3 UNROT"));
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.repr_at(1) == "2");  // top
    REQUIRE(ctx.repr_at(2) == "1");
    REQUIRE(ctx.repr_at(3) == "3");  // bottom
}

TEST_CASE("DUP2 command", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 2 DUP2"));
    REQUIRE(ctx.depth() == 4);
    REQUIRE(ctx.repr_at(1) == "2");
    REQUIRE(ctx.repr_at(2) == "1");
    REQUIRE(ctx.repr_at(3) == "2");
    REQUIRE(ctx.repr_at(4) == "1");
}

TEST_CASE("DUPN command", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 30 2 DUPN"));
    REQUIRE(ctx.depth() == 5);
    REQUIRE(ctx.repr_at(1) == "30");
    REQUIRE(ctx.repr_at(2) == "20");
    REQUIRE(ctx.repr_at(3) == "30");
    REQUIRE(ctx.repr_at(4) == "20");
    REQUIRE(ctx.repr_at(5) == "10");
}

TEST_CASE("DUPN with 0", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 2 3 0 DUPN"));
    REQUIRE(ctx.depth() == 3);
}

TEST_CASE("DROP2 command", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 2 3 DROP2"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "1");
}

TEST_CASE("DROPN command", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 30 40 3 DROPN"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "10");
}

TEST_CASE("DROPN with 0", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 2 3 0 DROPN"));
    REQUIRE(ctx.depth() == 3);
}

TEST_CASE("PICK command", "[stack]") {
    auto ctx = make_ctx();
    // 1 PICK == DUP
    REQUIRE(ctx.exec("42 1 PICK"));
    REQUIRE(ctx.depth() == 2);
    REQUIRE(ctx.repr_at(1) == "42");
    REQUIRE(ctx.repr_at(2) == "42");
}

TEST_CASE("PICK level 3", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 30 3 PICK"));
    REQUIRE(ctx.depth() == 4);
    REQUIRE(ctx.repr_at(1) == "10");  // copied from level 3
    REQUIRE(ctx.repr_at(2) == "30");
    REQUIRE(ctx.repr_at(3) == "20");
    REQUIRE(ctx.repr_at(4) == "10");
}

TEST_CASE("ROLL command", "[stack]") {
    auto ctx = make_ctx();
    // 3 ROLL == ROT
    REQUIRE(ctx.exec("1 2 3 3 ROLL"));
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.repr_at(1) == "1");  // was level 3, now top
    REQUIRE(ctx.repr_at(2) == "3");
    REQUIRE(ctx.repr_at(3) == "2");
}

TEST_CASE("ROLL with 1", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 30 1 ROLL"));
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.repr_at(1) == "30");
    REQUIRE(ctx.repr_at(2) == "20");
    REQUIRE(ctx.repr_at(3) == "10");
}

TEST_CASE("ROLLD command", "[stack]") {
    auto ctx = make_ctx();
    // 3 ROLLD == UNROT
    REQUIRE(ctx.exec("1 2 3 3 ROLLD"));
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.repr_at(1) == "2");
    REQUIRE(ctx.repr_at(2) == "1");
    REQUIRE(ctx.repr_at(3) == "3");  // was top, now level 3
}

TEST_CASE("ROLLD with 1", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 30 1 ROLLD"));
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.repr_at(1) == "30");
}

TEST_CASE("UNPICK command", "[stack]") {
    auto ctx = make_ctx();
    // Replace level 3 with 99
    REQUIRE(ctx.exec("10 20 30 99 3 UNPICK"));
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.repr_at(1) == "30");
    REQUIRE(ctx.repr_at(2) == "20");
    REQUIRE(ctx.repr_at(3) == "99");  // replaced 10
}

TEST_CASE("UNPICK level 1", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 99 1 UNPICK"));
    REQUIRE(ctx.depth() == 2);
    REQUIRE(ctx.repr_at(1) == "99");  // replaced 20
    REQUIRE(ctx.repr_at(2) == "10");
}

// ---- Error / edge-case tests ----

TEST_CASE("UNROT too few args", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 2 UNROT"));
}

TEST_CASE("DUP2 too few args", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 DUP2"));
}

TEST_CASE("DUP2 preserves types", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("\"hello\" 42 DUP2"));
    REQUIRE(ctx.depth() == 4);
    REQUIRE(ctx.repr_at(1) == "42");
    REQUIRE(ctx.repr_at(2) == "\"hello\"");
    REQUIRE(ctx.repr_at(3) == "42");
    REQUIRE(ctx.repr_at(4) == "\"hello\"");
}

TEST_CASE("DUPN bad type", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 2 3.5 DUPN"));
}

TEST_CASE("DUPN too few for N", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 5 DUPN"));
}

TEST_CASE("DUPN full stack", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 30 3 DUPN"));
    REQUIRE(ctx.depth() == 6);
    REQUIRE(ctx.repr_at(1) == "30");
    REQUIRE(ctx.repr_at(2) == "20");
    REQUIRE(ctx.repr_at(3) == "10");
    REQUIRE(ctx.repr_at(4) == "30");
    REQUIRE(ctx.repr_at(5) == "20");
    REQUIRE(ctx.repr_at(6) == "10");
}

TEST_CASE("DROP2 too few args", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 DROP2"));
}

TEST_CASE("DROPN bad type", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 2 3.0 DROPN"));
}

TEST_CASE("DROPN too few for N", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 5 DROPN"));
}

TEST_CASE("DROPN clears all", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 30 3 DROPN"));
    REQUIRE(ctx.depth() == 0);
}

TEST_CASE("PICK bad type", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 2 3.5 PICK"));
}

TEST_CASE("PICK too few for N", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 5 PICK"));
}

TEST_CASE("PICK 2 is OVER", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 2 PICK"));
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.repr_at(1) == "10");
    REQUIRE(ctx.repr_at(2) == "20");
    REQUIRE(ctx.repr_at(3) == "10");
}

TEST_CASE("ROLL bad type", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 2 3.5 ROLL"));
}

TEST_CASE("ROLL too few for N", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 2 5 ROLL"));
}

TEST_CASE("ROLL with 4", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 30 40 4 ROLL"));
    REQUIRE(ctx.depth() == 4);
    REQUIRE(ctx.repr_at(1) == "10");
    REQUIRE(ctx.repr_at(2) == "40");
    REQUIRE(ctx.repr_at(3) == "30");
    REQUIRE(ctx.repr_at(4) == "20");
}

TEST_CASE("ROLL 2 is SWAP", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 2 ROLL"));
    REQUIRE(ctx.depth() == 2);
    REQUIRE(ctx.repr_at(1) == "10");
    REQUIRE(ctx.repr_at(2) == "20");
}

TEST_CASE("ROLLD bad type", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 2 3.5 ROLLD"));
}

TEST_CASE("ROLLD too few for N", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 2 5 ROLLD"));
}

TEST_CASE("ROLLD with 4", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 30 40 4 ROLLD"));
    REQUIRE(ctx.depth() == 4);
    REQUIRE(ctx.repr_at(1) == "30");
    REQUIRE(ctx.repr_at(2) == "20");
    REQUIRE(ctx.repr_at(3) == "10");
    REQUIRE(ctx.repr_at(4) == "40");
}

TEST_CASE("ROLLD 2 is SWAP", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 2 ROLLD"));
    REQUIRE(ctx.depth() == 2);
    REQUIRE(ctx.repr_at(1) == "10");
    REQUIRE(ctx.repr_at(2) == "20");
}

TEST_CASE("ROLL then ROLLD roundtrip", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 30 40 4 ROLL 4 ROLLD"));
    REQUIRE(ctx.depth() == 4);
    REQUIRE(ctx.repr_at(1) == "40");
    REQUIRE(ctx.repr_at(2) == "30");
    REQUIRE(ctx.repr_at(3) == "20");
    REQUIRE(ctx.repr_at(4) == "10");
}

TEST_CASE("UNPICK bad type", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 2 99 3.5 UNPICK"));
}

TEST_CASE("UNPICK too few for N", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("1 99 5 UNPICK"));
}

TEST_CASE("UNPICK level 2", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 30 99 2 UNPICK"));
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.repr_at(1) == "30");
    REQUIRE(ctx.repr_at(2) == "99");  // replaced 20
    REQUIRE(ctx.repr_at(3) == "10");
}

TEST_CASE("UNPICK with string replacement", "[stack]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("10 20 30 \"replaced\" 3 UNPICK"));
    REQUIRE(ctx.depth() == 3);
    REQUIRE(ctx.repr_at(1) == "30");
    REQUIRE(ctx.repr_at(2) == "20");
    REQUIRE(ctx.repr_at(3) == "\"replaced\"");
}
