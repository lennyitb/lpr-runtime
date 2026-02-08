#pragma once

#include <string>
#include <variant>
#include <vector>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

namespace lpr {

using Integer  = boost::multiprecision::cpp_int;
using Real     = boost::multiprecision::cpp_dec_float_50;
using Rational = boost::multiprecision::cpp_rational;
using Complex  = std::pair<Real, Real>;

struct String  { std::string value; };
struct Name    { std::string value; };
struct Symbol  { std::string value; };
struct Error   { int code; std::string message; };

// Forward declare — Token is used in Program
struct Token;

struct Program { std::vector<Token> tokens; };

// Type tags for serialization
enum class TypeTag : int {
    Integer  = 0,
    Real     = 1,
    Rational = 2,
    Complex  = 3,
    String   = 4,
    Program  = 5,
    Name     = 6,
    Error    = 7,
    Symbol   = 8,
};

using Object = std::variant<
    Integer,   // 0
    Real,      // 1
    Rational,  // 2
    Complex,   // 3
    String,    // 4
    Program,   // 5
    Name,      // 6
    Error,     // 7
    Symbol     // 8
>;

// Token: either a literal to push, or a command name to execute
struct Token {
    enum Kind { Literal, Command };
    Kind kind;
    Object literal;       // used when kind == Literal
    std::string command;  // used when kind == Command

    static Token make_literal(Object obj) {
        Token t;
        t.kind = Literal;
        t.literal = std::move(obj);
        return t;
    }
    static Token make_command(std::string cmd) {
        Token t;
        t.kind = Command;
        t.command = std::move(cmd);
        return t;
    }
private:
    Token() : kind(Command) {}
};

// Display representation
std::string repr(const Object& obj);

// Serialization
TypeTag     type_tag(const Object& obj);
std::string serialize(const Object& obj);
Object      deserialize(TypeTag tag, const std::string& data);

} // namespace lpr
