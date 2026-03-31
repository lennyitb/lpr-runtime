#include <catch2/catch_test_macros.hpp>
#include "lpr/lpr.h"
#include "core/context.hpp"
#include <cstring>
#include <cstdlib>
#include <string>
#include <cmath>

using namespace lpr;

static Context make_ctx() { return Context(nullptr); }

static std::string exec_repr(lpr_ctx* ctx, const char* expr) {
    lpr_exec(ctx, expr);
    char* s = lpr_repr(ctx, 1);
    std::string result = s ? s : "";
    lpr_free(s);
    return result;
}

// ===== 8.1 STD/FIX/SCI/ENG formatting =====

TEST_CASE("FIX mode formats Real", "[display]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    lpr_exec(ctx, "4 FIX");
    std::string r = exec_repr(ctx, "3.14159265");
    REQUIRE(r == "3.1416");
    lpr_close(ctx);
}

TEST_CASE("SCI mode formats Real", "[display]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    lpr_exec(ctx, "3 SCI");
    std::string r = exec_repr(ctx, "12345.6789");
    REQUIRE(r == "1.235E4");
    lpr_close(ctx);
}

TEST_CASE("ENG mode formats Real", "[display]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    lpr_exec(ctx, "3 ENG");
    std::string r = exec_repr(ctx, "12345.6789");
    REQUIRE(r == "12.346E3");
    lpr_close(ctx);
}

TEST_CASE("STD restores full precision", "[display]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    lpr_exec(ctx, "2 FIX");
    std::string fixed = exec_repr(ctx, "3.14159");
    REQUIRE(fixed == "3.14");
    lpr_exec(ctx, "DROP STD");
    std::string full = exec_repr(ctx, "3.14159");
    REQUIRE(full == "3.14159");
    lpr_close(ctx);
}

TEST_CASE("FIX mode on Integer unchanged", "[display]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    lpr_exec(ctx, "4 FIX");
    std::string r = exec_repr(ctx, "42");
    REQUIRE(r == "42");
    lpr_close(ctx);
}

TEST_CASE("FIX mode on Rational unchanged", "[display]") {
    // Rational display is unaffected by FIX mode
    auto ctx = make_ctx();
    ctx.exec("4 FIX");
    ctx.store().push(Rational(Integer(1), Integer(3)));
    DisplaySettings ds = ctx.current_display_settings();
    Object obj = ctx.store().peek(1);
    REQUIRE(repr(obj, ds) == "1/3");
}

TEST_CASE("FIX mode on Complex", "[display]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    lpr_exec(ctx, "2 FIX");
    std::string r = exec_repr(ctx, "(3.14159, 2.71828)");
    REQUIRE(r == "(3.14, 2.72)");
    lpr_close(ctx);
}

TEST_CASE("FIX rejects invalid digits", "[display]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    lpr_result r = lpr_exec(ctx, "12 FIX");
    REQUIRE(r.ok == 0);
    lpr_close(ctx);
}

// ===== 8.2 RECT/POLAR/SPHERICAL Complex display =====

TEST_CASE("RECT mode displays Complex as (re, im)", "[display]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    lpr_exec(ctx, "RECT");
    std::string r = exec_repr(ctx, "(3., 4.)");
    // Default rect display
    REQUIRE(r.find("3") != std::string::npos);
    REQUIRE(r.find("4") != std::string::npos);
    REQUIRE(r.find("\xE2\x88\xA0") == std::string::npos); // no angle symbol
    lpr_close(ctx);
}

TEST_CASE("POLAR mode displays Complex as (r, angle)", "[display]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    lpr_exec(ctx, "POLAR 4 FIX");
    std::string r = exec_repr(ctx, "(3., 4.)");
    // Should contain angle symbol ∠
    REQUIRE(r.find("\xE2\x88\xA0") != std::string::npos);
    // Magnitude should be 5.0000
    REQUIRE(r.find("5.0000") != std::string::npos);
    lpr_close(ctx);
}

// ===== 8.3 SF/CF/FS?/FC?, SFLAG/RFLAG, STOF/RCLF =====

TEST_CASE("SF and FS? set and test boolean flag", "[flags]") {
    auto ctx = make_ctx();
    ctx.exec("'exact_mode' SF");
    ctx.exec("'exact_mode' FS?");
    Object result = ctx.store().peek(1);
    REQUIRE(std::holds_alternative<Integer>(result));
    REQUIRE(std::get<Integer>(result) == 1);
}

TEST_CASE("CF clears boolean flag", "[flags]") {
    auto ctx = make_ctx();
    ctx.exec("'exact_mode' SF");
    ctx.exec("'exact_mode' CF");
    ctx.exec("'exact_mode' FS?");
    Object result = ctx.store().peek(1);
    REQUIRE(std::holds_alternative<Integer>(result));
    REQUIRE(std::get<Integer>(result) == 0);
}

TEST_CASE("FC? returns 1 for nonexistent flag", "[flags]") {
    auto ctx = make_ctx();
    ctx.exec("'nonexistent' FC?");
    Object result = ctx.store().peek(1);
    REQUIRE(std::holds_alternative<Integer>(result));
    REQUIRE(std::get<Integer>(result) == 1);
}

TEST_CASE("FC? returns 0 for set flag", "[flags]") {
    auto ctx = make_ctx();
    ctx.exec("'myflag' SF");
    ctx.exec("'myflag' FC?");
    Object result = ctx.store().peek(1);
    REQUIRE(std::holds_alternative<Integer>(result));
    REQUIRE(std::get<Integer>(result) == 0);
}

TEST_CASE("SFLAG/RFLAG store and recall integer", "[flags]") {
    auto ctx = make_ctx();
    ctx.exec("1000 'max_iterations' SFLAG");
    ctx.exec("'max_iterations' RFLAG");
    Object result = ctx.store().peek(1);
    REQUIRE(std::holds_alternative<Integer>(result));
    REQUIRE(std::get<Integer>(result) == 1000);
}

TEST_CASE("SFLAG/RFLAG store and recall string", "[flags]") {
    auto ctx = make_ctx();
    ctx.exec("\"hello\" 'greeting' SFLAG");
    ctx.exec("'greeting' RFLAG");
    Object result = ctx.store().peek(1);
    REQUIRE(std::holds_alternative<String>(result));
    REQUIRE(std::get<String>(result).value == "hello");
}

TEST_CASE("RFLAG on nonexistent flag errors", "[flags]") {
    auto ctx = make_ctx();
    bool ok = ctx.exec("'nonexistent' RFLAG");
    REQUIRE(!ok);
}

TEST_CASE("STOF/RCLF bulk round-trip", "[flags]") {
    auto ctx = make_ctx();
    ctx.exec("'exact_mode' SF");
    ctx.exec("1000 'max_iterations' SFLAG");
    ctx.exec("STOF");
    Object saved = ctx.store().peek(1);
    REQUIRE(std::holds_alternative<List>(saved));
    auto& list = std::get<List>(saved);
    REQUIRE(list.items.size() == 2);

    // Clear and restore
    ctx.exec("RCLF");
    ctx.exec("'exact_mode' FS?");
    Object result = ctx.store().peek(1);
    REQUIRE(std::holds_alternative<Integer>(result));
    REQUIRE(std::get<Integer>(result) == 1);
}

// ===== 8.4 ->Q rational approximation =====

TEST_CASE("->Q approximates 0.5 as 1/2", "[conversion]") {
    auto ctx = make_ctx();
    ctx.exec("0.5 ->Q");
    Object result = ctx.store().peek(1);
    REQUIRE(std::holds_alternative<Rational>(result));
    auto& r = std::get<Rational>(result);
    REQUIRE(boost::multiprecision::numerator(r) == 1);
    REQUIRE(boost::multiprecision::denominator(r) == 2);
}

TEST_CASE("->Q approximates pi", "[conversion]") {
    auto ctx = make_ctx();
    ctx.exec("3.14159265358979 ->Q");
    Object result = ctx.store().peek(1);
    REQUIRE(std::holds_alternative<Rational>(result));
    auto& r = std::get<Rational>(result);
    // Should be 355/113 or similar good approximation
    double approx = boost::multiprecision::numerator(r).convert_to<double>() /
                    boost::multiprecision::denominator(r).convert_to<double>();
    REQUIRE(std::fabs(approx - 3.14159265358979) < 1e-10);
}

TEST_CASE("->Q on Integer wraps to rational", "[conversion]") {
    auto ctx = make_ctx();
    ctx.exec("5 ->Q");
    Object result = ctx.store().peek(1);
    REQUIRE(std::holds_alternative<Rational>(result));
    auto& r = std::get<Rational>(result);
    REQUIRE(boost::multiprecision::numerator(r) == 5);
    REQUIRE(boost::multiprecision::denominator(r) == 1);
}

// ===== 8.5 HMS<->decimal round-trip =====

TEST_CASE("HMS-> converts H.MMSSss to decimal", "[conversion]") {
    auto ctx = make_ctx();
    ctx.exec("2.3000 HMS->");
    Object result = ctx.store().peek(1);
    REQUIRE(std::holds_alternative<Real>(result));
    double val = std::get<Real>(result).convert_to<double>();
    REQUIRE(std::fabs(val - 2.5) < 1e-10);
}

TEST_CASE("->HMS converts decimal to H.MMSSss", "[conversion]") {
    auto ctx = make_ctx();
    ctx.exec("2.5 ->HMS");
    Object result = ctx.store().peek(1);
    REQUIRE(std::holds_alternative<Real>(result));
    double val = std::get<Real>(result).convert_to<double>();
    REQUIRE(std::fabs(val - 2.3) < 1e-10);
}

TEST_CASE("HMS round-trip", "[conversion]") {
    auto ctx = make_ctx();
    ctx.exec("1.4530 HMS-> ->HMS");
    Object result = ctx.store().peek(1);
    REQUIRE(std::holds_alternative<Real>(result));
    double val = std::get<Real>(result).convert_to<double>();
    REQUIRE(std::fabs(val - 1.4530) < 1e-8);
}

// ===== 8.6 Format persistence across lpr_exec calls =====

TEST_CASE("Format settings persist across exec calls", "[display]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    lpr_exec(ctx, "4 FIX");
    // Settings should persist across a separate exec call
    lpr_exec(ctx, "3.14159265");
    char* s = lpr_repr(ctx, 1);
    REQUIRE(std::string(s) == "3.1416");
    lpr_free(s);
    lpr_close(ctx);
}

// ===== 8.7 History count and entry =====

TEST_CASE("Fresh context has zero history", "[history]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    REQUIRE(lpr_history_count(ctx) == 0);
    lpr_close(ctx);
}

TEST_CASE("History records successful commands", "[history]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    lpr_exec(ctx, "3");
    lpr_exec(ctx, "4");
    lpr_exec(ctx, "+");
    REQUIRE(lpr_history_count(ctx) == 3);

    // 0 = most recent
    char* e0 = lpr_history_entry(ctx, 0);
    REQUIRE(std::string(e0) == "+");
    lpr_free(e0);

    char* e2 = lpr_history_entry(ctx, 2);
    REQUIRE(std::string(e2) == "3");
    lpr_free(e2);

    lpr_close(ctx);
}

TEST_CASE("History entry returns NULL for out of range", "[history]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    lpr_exec(ctx, "3 4 +");
    REQUIRE(lpr_history_entry(ctx, 999) == nullptr);
    lpr_close(ctx);
}

// ===== 8.8 History does not record failed commands =====

TEST_CASE("Failed commands not recorded in history", "[history]") {
    lpr_ctx* ctx = lpr_open(nullptr);
    int before = lpr_history_count(ctx);
    lpr_exec(ctx, "+"); // should fail on empty stack
    int after = lpr_history_count(ctx);
    REQUIRE(after == before);
    lpr_close(ctx);
}
