#include <catch2/catch_test_macros.hpp>
#include "core/context.hpp"

using namespace lpr;

TEST_CASE("Push and resolve local variable", "[locals]") {
    Context ctx(nullptr);
    std::unordered_map<std::string, Object> frame;
    frame["X"] = Integer(42);
    ctx.push_locals(frame);
    auto val = ctx.resolve_local("X");
    REQUIRE(val.has_value());
    REQUIRE(std::holds_alternative<Integer>(*val));
    REQUIRE(std::get<Integer>(*val) == 42);
    ctx.pop_locals();
}

TEST_CASE("Resolve returns nullopt for missing name", "[locals]") {
    Context ctx(nullptr);
    auto val = ctx.resolve_local("X");
    REQUIRE(!val.has_value());
}

TEST_CASE("Nested scopes shadow outer bindings", "[locals]") {
    Context ctx(nullptr);
    std::unordered_map<std::string, Object> outer;
    outer["X"] = Integer(1);
    ctx.push_locals(outer);

    std::unordered_map<std::string, Object> inner;
    inner["X"] = Integer(2);
    ctx.push_locals(inner);

    auto val = ctx.resolve_local("X");
    REQUIRE(val.has_value());
    REQUIRE(std::get<Integer>(*val) == 2);

    ctx.pop_locals();
    val = ctx.resolve_local("X");
    REQUIRE(val.has_value());
    REQUIRE(std::get<Integer>(*val) == 1);

    ctx.pop_locals();
    val = ctx.resolve_local("X");
    REQUIRE(!val.has_value());
}

TEST_CASE("Inner scope can see outer scope variables", "[locals]") {
    Context ctx(nullptr);
    std::unordered_map<std::string, Object> outer;
    outer["X"] = Integer(10);
    ctx.push_locals(outer);

    std::unordered_map<std::string, Object> inner;
    inner["Y"] = Integer(20);
    ctx.push_locals(inner);

    auto x = ctx.resolve_local("X");
    auto y = ctx.resolve_local("Y");
    REQUIRE(x.has_value());
    REQUIRE(y.has_value());
    REQUIRE(std::get<Integer>(*x) == 10);
    REQUIRE(std::get<Integer>(*y) == 20);

    ctx.pop_locals();
    ctx.pop_locals();
}
