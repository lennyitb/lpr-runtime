#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"

using namespace lpr;

TEST_CASE("STO and RCL", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("42 'x' STO"));
    REQUIRE(ctx.depth() == 0);
    REQUIRE(ctx.exec("'x' RCL"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "42");
}

TEST_CASE("RCL nonexistent variable produces error", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE_FALSE(ctx.exec("'unknown' RCL"));
}

TEST_CASE("PURGE removes variable", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("42 'x' STO"));
    REQUIRE(ctx.exec("'x' PURGE"));
    REQUIRE_FALSE(ctx.exec("'x' RCL"));
}

TEST_CASE("HOME command", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("HOME"));
    // Should not error
    REQUIRE(ctx.depth() == 0);
}

TEST_CASE("PATH command", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("PATH"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "\"HOME\"");

    // PATH after CD into subdirectory
    REQUIRE(ctx.exec("DROP"));
    REQUIRE(ctx.exec("'MYDIR' CRDIR"));
    REQUIRE(ctx.exec("'MYDIR' CD"));
    REQUIRE(ctx.exec("PATH"));
    REQUIRE(ctx.repr_at(1) == "\"HOME/MYDIR\"");

    // PATH after nested CD
    REQUIRE(ctx.exec("DROP"));
    REQUIRE(ctx.exec("'SUB' CRDIR"));
    REQUIRE(ctx.exec("'SUB' CD"));
    REQUIRE(ctx.exec("PATH"));
    REQUIRE(ctx.repr_at(1) == "\"HOME/MYDIR/SUB\"");
}

TEST_CASE("CRDIR creates subdirectory", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("'MYDIR' CRDIR"));
    REQUIRE(ctx.depth() == 0);
}

TEST_CASE("CD into existing subdirectory", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("'MYDIR' CRDIR"));
    REQUIRE(ctx.exec("'MYDIR' CD"));
    REQUIRE(ctx.depth() == 0);
    REQUIRE(ctx.exec("PATH"));
    REQUIRE(ctx.repr_at(1) == "\"HOME/MYDIR\"");
}

TEST_CASE("CD into nonexistent subdirectory errors", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE_FALSE(ctx.exec("'NODIR' CD"));
    REQUIRE(ctx.depth() == 1);
}

TEST_CASE("UPDIR from subdirectory", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("'MYDIR' CRDIR"));
    REQUIRE(ctx.exec("'MYDIR' CD"));
    REQUIRE(ctx.exec("UPDIR"));
    REQUIRE(ctx.exec("PATH"));
    REQUIRE(ctx.repr_at(1) == "\"HOME\"");
}

TEST_CASE("UPDIR at HOME is no-op", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("HOME"));
    REQUIRE(ctx.exec("UPDIR"));
    REQUIRE(ctx.exec("PATH"));
    REQUIRE(ctx.repr_at(1) == "\"HOME\"");
}

TEST_CASE("PGDIR removes directory and contents", "[filesystem]") {
    Context ctx(nullptr);
    // Create dir with a variable and a sub-subdirectory
    REQUIRE(ctx.exec("'MYDIR' CRDIR"));
    REQUIRE(ctx.exec("'MYDIR' CD"));
    REQUIRE(ctx.exec("42 'x' STO"));
    REQUIRE(ctx.exec("'SUB' CRDIR"));
    REQUIRE(ctx.exec("HOME"));
    // Purge it
    REQUIRE(ctx.exec("'MYDIR' PGDIR"));
    REQUIRE(ctx.depth() == 0);
    // CD into it should now fail
    REQUIRE_FALSE(ctx.exec("'MYDIR' CD"));
}

TEST_CASE("PGDIR nonexistent directory errors", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE_FALSE(ctx.exec("'NODIR' PGDIR"));
    REQUIRE(ctx.depth() == 1);
}

TEST_CASE("VARS lists variables and subdirectories", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("10 'a' STO"));
    REQUIRE(ctx.exec("20 'b' STO"));
    REQUIRE(ctx.exec("'MYDIR' CRDIR"));
    REQUIRE(ctx.exec("VARS"));
    REQUIRE(ctx.depth() == 1);
    std::string vars = ctx.repr_at(1);
    // Variables first as Names, then dirs with / suffix
    REQUIRE(vars.find("'a'") != std::string::npos);
    REQUIRE(vars.find("'b'") != std::string::npos);
    REQUIRE(vars.find("'MYDIR/'") != std::string::npos);
    // Should be a list
    REQUIRE(vars.front() == '{');
}

TEST_CASE("VARS on empty directory returns empty list", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("'EMPTYDIR' CRDIR"));
    REQUIRE(ctx.exec("'EMPTYDIR' CD"));
    REQUIRE(ctx.exec("VARS"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "{  }");
}

// ---- Implicit variable recall (name fallthrough) ----

TEST_CASE("Bare name recalls stored variable", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("42 'X' STO"));
    REQUIRE(ctx.exec("X"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "42");
}

TEST_CASE("Bare name executes stored program", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("<< 1 2 + >> 'ADD3' STO"));
    REQUIRE(ctx.exec("ADD3"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "3");
}

TEST_CASE("Bare unknown name still errors", "[filesystem]") {
    Context ctx(nullptr);
    REQUIRE_FALSE(ctx.exec("NOSUCHVAR"));
}
