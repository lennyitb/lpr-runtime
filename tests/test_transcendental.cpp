#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "core/context.hpp"
#include "lpr/lpr.h"
#include <cmath>
#include <cstdlib>

using namespace lpr;

static Context make_ctx() { return Context(nullptr); }

// Helper: get top of stack as double
static double top_as_double(Context& ctx) {
    Object obj = ctx.store().peek(1);
    if (std::holds_alternative<Real>(obj))
        return std::get<Real>(obj).convert_to<double>();
    if (std::holds_alternative<Integer>(obj))
        return std::get<Integer>(obj).convert_to<double>();
    return 0.0;
}

// --- Angle Mode ---

TEST_CASE("Default angle mode is RAD", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.store().get_meta("angle_mode", "RAD") == "RAD");
}

TEST_CASE("DEG sets angle mode", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("DEG"));
    REQUIRE(ctx.store().get_meta("angle_mode", "RAD") == "DEG");
}

TEST_CASE("RAD restores angle mode", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("DEG RAD"));
    REQUIRE(ctx.store().get_meta("angle_mode", "RAD") == "RAD");
}

TEST_CASE("GRAD sets angle mode", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("GRAD"));
    REQUIRE(ctx.store().get_meta("angle_mode", "RAD") == "GRAD");
}

// --- Trig in RAD mode ---

TEST_CASE("SIN 0 = 0", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 SIN"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(0.0, 1e-10));
}

TEST_CASE("COS 0 = 1", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 COS"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(1.0, 1e-10));
}

TEST_CASE("TAN 0 = 0", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 TAN"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(0.0, 1e-10));
}

TEST_CASE("SIN PI/2 = 1 (RAD)", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("PI 2 / SIN"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(1.0, 1e-10));
}

// --- Trig in DEG mode ---

TEST_CASE("SIN 90 = 1 (DEG)", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("DEG 90 SIN"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(1.0, 1e-10));
}

TEST_CASE("COS 180 = -1 (DEG)", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("DEG 180 COS"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(-1.0, 1e-10));
}

// --- Inverse trig ---

TEST_CASE("ASIN 1 = PI/2 (RAD)", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 ASIN"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(M_PI / 2, 1e-10));
}

TEST_CASE("ASIN 1 = 90 (DEG)", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("DEG 1 ASIN"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(90.0, 1e-10));
}

TEST_CASE("ACOS 0 = PI/2 (RAD)", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 ACOS"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(M_PI / 2, 1e-10));
}

TEST_CASE("ATAN 1 = PI/4 (RAD)", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 ATAN"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(M_PI / 4, 1e-10));
}

TEST_CASE("ATAN2", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 1 ATAN2"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(M_PI / 4, 1e-10));
}

// --- Exp / Log ---

TEST_CASE("EXP 0 = 1", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 EXP"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(1.0, 1e-10));
}

TEST_CASE("EXP 1 = e", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("1 EXP"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(M_E, 1e-10));
}

TEST_CASE("LN e = 1", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("E LN"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(1.0, 1e-10));
}

TEST_CASE("LOG 100 = 2", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("100 LOG"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(2.0, 1e-10));
}

TEST_CASE("ALOG 2 = 100", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("2 ALOG"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(100.0, 1e-10));
}

TEST_CASE("LN of negative fails", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE_FALSE(ctx.exec("-1 LN"));
}

// --- SQRT, SQ ---

TEST_CASE("SQRT 16 = 4", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("16 SQRT"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(4.0, 1e-10));
}

TEST_CASE("SQ 7 = 49", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("7 SQ"));
    REQUIRE(ctx.repr_at(1) == "49");
}

// --- Constants ---

TEST_CASE("PI constant", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("PI"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(M_PI, 1e-10));
}

TEST_CASE("E constant", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("E"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(M_E, 1e-10));
}

// --- Rounding ---

TEST_CASE("FLOOR 3.7 = 3", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("3.7 FLOOR"));
    REQUIRE(ctx.repr_at(1) == "3");
}

TEST_CASE("FLOOR -3.2 = -4", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("-3.2 FLOOR"));
    REQUIRE(ctx.repr_at(1) == "-4");
}

TEST_CASE("CEIL 3.2 = 4", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("3.2 CEIL"));
    REQUIRE(ctx.repr_at(1) == "4");
}

TEST_CASE("IP 3.7 = 3", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("3.7 IP"));
    REQUIRE(ctx.repr_at(1) == "3");
}

TEST_CASE("FP 3.7", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("3.7 FP"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(0.7, 1e-10));
}

TEST_CASE("FLOOR of integer is identity", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("5 FLOOR"));
    REQUIRE(ctx.repr_at(1) == "5");
}

// --- MIN, MAX, SIGN ---

TEST_CASE("MIN", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("3 7 MIN"));
    REQUIRE(ctx.repr_at(1) == "3");
}

TEST_CASE("MAX", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("3 7 MAX"));
    REQUIRE(ctx.repr_at(1) == "7");
}

TEST_CASE("SIGN positive", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("42 SIGN"));
    REQUIRE(ctx.repr_at(1) == "1");
}

TEST_CASE("SIGN negative", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("-5 SIGN"));
    REQUIRE(ctx.repr_at(1) == "-1");
}

TEST_CASE("SIGN zero", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 SIGN"));
    REQUIRE(ctx.repr_at(1) == "0");
}

// --- Combinatorics ---

TEST_CASE("Factorial 0! = 1", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 !"));
    REQUIRE(ctx.repr_at(1) == "1");
}

TEST_CASE("Factorial 5! = 120", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("5 !"));
    REQUIRE(ctx.repr_at(1) == "120");
}

TEST_CASE("COMB(5,2) = 10", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("5 2 COMB"));
    REQUIRE(ctx.repr_at(1) == "10");
}

TEST_CASE("PERM(5,2) = 20", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("5 2 PERM"));
    REQUIRE(ctx.repr_at(1) == "20");
}

// --- Percentage ---

TEST_CASE("% - 200 15% = 30", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("200 15 %"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(30.0, 1e-10));
}

TEST_CASE("%T - 25 is what % of 200", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("200 25 %T"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(12.5, 1e-10));
}

TEST_CASE("%CH - percent change 100 to 120", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("100 120 %CH"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(20.0, 1e-10));
}

// --- Angle Conversion ---

TEST_CASE("D->R 180 = PI", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("180 D->R"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(M_PI, 1e-10));
}

TEST_CASE("R->D PI = 180", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("PI R->D"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(180.0, 1e-10));
}

// --- Type promotion ---

TEST_CASE("SIN works with integer input", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("0 SIN"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(0.0, 1e-10));
}

TEST_CASE("SQRT works with integer input", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("4 SQRT"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(2.0, 1e-10));
}

// --- GRAD mode ---

TEST_CASE("SIN 100 = 1 (GRAD)", "[transcendental]") {
    auto ctx = make_ctx();
    REQUIRE(ctx.exec("GRAD 100 SIN"));
    REQUIRE_THAT(top_as_double(ctx), Catch::Matchers::WithinAbs(1.0, 1e-10));
}

// --- lpr_get_setting C API ---

TEST_CASE("lpr_get_setting - angle_mode after DEG", "[api]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    REQUIRE(ctx != nullptr);
    lpr_exec(ctx, "DEG");
    char* val = lpr_get_setting(ctx, "angle_mode");
    REQUIRE(val != nullptr);
    REQUIRE(std::string(val) == "DEG");
    lpr_free(val);
    lpr_close(ctx);
}

TEST_CASE("lpr_get_setting - angle_mode after RAD", "[api]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    REQUIRE(ctx != nullptr);
    lpr_exec(ctx, "RAD");
    char* val = lpr_get_setting(ctx, "angle_mode");
    REQUIRE(val != nullptr);
    REQUIRE(std::string(val) == "RAD");
    lpr_free(val);
    lpr_close(ctx);
}

TEST_CASE("lpr_get_setting - angle_mode after GRAD", "[api]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    REQUIRE(ctx != nullptr);
    lpr_exec(ctx, "GRAD");
    char* val = lpr_get_setting(ctx, "angle_mode");
    REQUIRE(val != nullptr);
    REQUIRE(std::string(val) == "GRAD");
    lpr_free(val);
    lpr_close(ctx);
}

TEST_CASE("lpr_get_setting - nonexistent key returns NULL", "[api]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    REQUIRE(ctx != nullptr);
    char* val = lpr_get_setting(ctx, "nonexistent_key");
    REQUIRE(val == nullptr);
    lpr_close(ctx);
}

TEST_CASE("lpr_get_setting - current_dir default", "[api]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    REQUIRE(ctx != nullptr);
    char* val = lpr_get_setting(ctx, "current_dir");
    REQUIRE(val != nullptr);
    lpr_free(val);
    lpr_close(ctx);
}
