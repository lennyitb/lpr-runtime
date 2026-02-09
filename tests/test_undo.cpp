#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"
#include "lpr/lpr.h"

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

TEST_CASE("lpr_get_state tracks undo/redo levels", "[undo]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    REQUIRE(ctx);

    // Fresh context: nothing to undo or redo
    lpr_state st = lpr_get_state(ctx);
    REQUIRE(st.undo_levels == 0);
    REQUIRE(st.redo_levels == 0);

    // After 3 execs: 3 undo levels, 0 redo
    lpr_exec(ctx, "1");
    lpr_exec(ctx, "2");
    lpr_exec(ctx, "3");
    st = lpr_get_state(ctx);
    REQUIRE(st.undo_levels == 3);
    REQUIRE(st.redo_levels == 0);

    // Undo once: 2 undo, 1 redo
    lpr_undo(ctx);
    st = lpr_get_state(ctx);
    REQUIRE(st.undo_levels == 2);
    REQUIRE(st.redo_levels == 1);

    // Undo all the way back
    lpr_undo(ctx);
    lpr_undo(ctx);
    st = lpr_get_state(ctx);
    REQUIRE(st.undo_levels == 0);
    REQUIRE(st.redo_levels == 3);

    // Redo one
    lpr_redo(ctx);
    st = lpr_get_state(ctx);
    REQUIRE(st.undo_levels == 1);
    REQUIRE(st.redo_levels == 2);

    lpr_close(ctx);
}
