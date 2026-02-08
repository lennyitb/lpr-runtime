#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"

using namespace lpr;

TEST_CASE("Program push onto stack", "[programs]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("\xC2\xAB 2 3 + \xC2\xBB"));
    REQUIRE(ctx.depth() == 1);
    std::string r = ctx.repr_at(1);
    REQUIRE(r.find("2") != std::string::npos);
    REQUIRE(r.find("3") != std::string::npos);
}

TEST_CASE("EVAL executes program", "[programs]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("\xC2\xAB 2 3 + \xC2\xBB EVAL"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "5");
}

TEST_CASE("EVAL recalls name", "[programs]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("42 'x' STO"));
    REQUIRE(ctx.exec("'x' EVAL"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "42");
}

TEST_CASE("IFT true condition", "[programs]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("\xC2\xAB \"yes\" \xC2\xBB 1 IFT"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "\"yes\"");
}

TEST_CASE("IFT false condition", "[programs]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("\xC2\xAB \"yes\" \xC2\xBB 0 IFT"));
    REQUIRE(ctx.depth() == 0);
}

TEST_CASE("IFTE true branch", "[programs]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("\xC2\xAB \"no\" \xC2\xBB \xC2\xAB \"yes\" \xC2\xBB 1 IFTE"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "\"yes\"");
}

TEST_CASE("IFTE false branch", "[programs]") {
    Context ctx(nullptr);
    REQUIRE(ctx.exec("\xC2\xAB \"no\" \xC2\xBB \xC2\xAB \"yes\" \xC2\xBB 0 IFTE"));
    REQUIRE(ctx.depth() == 1);
    REQUIRE(ctx.repr_at(1) == "\"no\"");
}
