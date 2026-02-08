#include <catch2/catch_test_macros.hpp>
#include "core/object.hpp"

using namespace lpr;

TEST_CASE("Integer construction and repr", "[types]") {
    Object obj = Integer(42);
    REQUIRE(repr(obj) == "42");
    REQUIRE(type_tag(obj) == TypeTag::Integer);
}

TEST_CASE("Large integer construction", "[types]") {
    Object obj = Integer("99999999999999999999999999999999");
    REQUIRE(repr(obj) == "99999999999999999999999999999999");
}

TEST_CASE("Real construction", "[types]") {
    Object obj = Real("3.14159");
    std::string r = repr(obj);
    REQUIRE(r.find("3.14159") != std::string::npos);
}

TEST_CASE("Real scientific notation", "[types]") {
    Object obj = Real("1.5E-10");
    REQUIRE(type_tag(obj) == TypeTag::Real);
}

TEST_CASE("Rational construction", "[types]") {
    Object obj = Rational(Integer(355), Integer(113));
    REQUIRE(repr(obj) == "355/113");
}

TEST_CASE("Complex construction", "[types]") {
    Object obj = Complex{Real(3), Real(4)};
    std::string r = repr(obj);
    REQUIRE(r.find("3") != std::string::npos);
    REQUIRE(r.find("4") != std::string::npos);
}

TEST_CASE("String construction", "[types]") {
    Object obj = String{"hello world"};
    REQUIRE(repr(obj) == "\"hello world\"");
}

TEST_CASE("Program construction", "[types]") {
    Program p;
    p.tokens.push_back(Token::make_command("DUP"));
    p.tokens.push_back(Token::make_command("*"));
    Object obj = std::move(p);
    std::string r = repr(obj);
    REQUIRE(r.find("DUP") != std::string::npos);
    REQUIRE(r.find("*") != std::string::npos);
}

TEST_CASE("Name construction", "[types]") {
    Object obj = Name{"myvar"};
    REQUIRE(repr(obj) == "'myvar'");
}

TEST_CASE("Error construction", "[types]") {
    Object obj = Error{1, "Stack underflow"};
    std::string r = repr(obj);
    REQUIRE(r.find("1") != std::string::npos);
    REQUIRE(r.find("Stack underflow") != std::string::npos);
}

TEST_CASE("Symbol stub construction", "[types]") {
    Object obj = Symbol{"X^2 + 1"};
    REQUIRE(repr(obj) == "'X^2 + 1'");
}

TEST_CASE("Serialize/deserialize Integer roundtrip", "[types]") {
    Object orig = Integer("123456789012345678901234567890");
    auto tag = type_tag(orig);
    auto data = serialize(orig);
    Object restored = deserialize(tag, data);
    REQUIRE(repr(restored) == repr(orig));
}

TEST_CASE("Serialize/deserialize Real roundtrip", "[types]") {
    Object orig = Real("3.14159265358979323846264338327950288");
    auto tag = type_tag(orig);
    auto data = serialize(orig);
    Object restored = deserialize(tag, data);
    REQUIRE(repr(restored) == repr(orig));
}

TEST_CASE("Serialize/deserialize Rational roundtrip", "[types]") {
    Object orig = Rational(Integer(355), Integer(113));
    auto tag = type_tag(orig);
    auto data = serialize(orig);
    Object restored = deserialize(tag, data);
    REQUIRE(repr(restored) == "355/113");
}

TEST_CASE("Serialize/deserialize Complex roundtrip", "[types]") {
    Object orig = Complex{Real(3), Real(4)};
    auto tag = type_tag(orig);
    auto data = serialize(orig);
    Object restored = deserialize(tag, data);
    REQUIRE(repr(restored) == repr(orig));
}

TEST_CASE("Serialize/deserialize String roundtrip", "[types]") {
    Object orig = String{"hello"};
    auto tag = type_tag(orig);
    auto data = serialize(orig);
    Object restored = deserialize(tag, data);
    REQUIRE(repr(restored) == repr(orig));
}

TEST_CASE("Serialize/deserialize Name roundtrip", "[types]") {
    Object orig = Name{"myvar"};
    auto tag = type_tag(orig);
    auto data = serialize(orig);
    Object restored = deserialize(tag, data);
    REQUIRE(repr(restored) == "'myvar'");
}

TEST_CASE("Serialize/deserialize Error roundtrip", "[types]") {
    Object orig = Error{42, "Something went wrong"};
    auto tag = type_tag(orig);
    auto data = serialize(orig);
    Object restored = deserialize(tag, data);
    std::string r = repr(restored);
    REQUIRE(r.find("42") != std::string::npos);
    REQUIRE(r.find("Something went wrong") != std::string::npos);
}

TEST_CASE("Serialize/deserialize Symbol roundtrip", "[types]") {
    Object orig = Symbol{"X^2 + 1"};
    auto tag = type_tag(orig);
    auto data = serialize(orig);
    Object restored = deserialize(tag, data);
    REQUIRE(repr(restored) == "'X^2 + 1'");
}
