#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"

using namespace lpr;

TEST_CASE("Undo restores previous state", "[undo]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("42"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.exec("DROP"));
    REQUIRE(ctx.depth() == 0);
    REQUIRE(ctx.undo());
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "42");
}

TEST_CASE("Redo re-applies undone operation", "[undo]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("42"));
    REQUIRE(ctx.exec("DROP"));
    REQUIRE(ctx.undo());
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.redo());
    REQUIRE(ctx.depth() == 0);
}

TEST_CASE("Undo at beginning of history returns false", "[undo]") {
    Context ctx(nullptr);
    REQUIRE_FALSE(ctx.undo());
    REQUIRE(ctx.depth() == 0);
}

TEST_CASE("Multiple undo steps", "[undo]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("1"));
    REQUIRE(ctx.exec("2"));
    REQUIRE(ctx.exec("3"));
    REQUIRE(ctx.depth() == 3);

    REQUIRE(ctx.undo()); // undo pushing 3
    REQUIRE(ctx.depth() == 2);

    REQUIRE(ctx.undo()); // undo pushing 2
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "1");
}
