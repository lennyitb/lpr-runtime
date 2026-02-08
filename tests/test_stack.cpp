#include <catch2/catch_test_macros.hpp>
#include "core/store.hpp"

using namespace lpr;

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
