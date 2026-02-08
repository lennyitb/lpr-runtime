#include "core/object.hpp"
#include "core/parser.hpp"
#include <sstream>
#include <stdexcept>

namespace lpr {

// ---------- repr ----------

static std::string repr_tokens(const std::vector<Token>& tokens);

std::string repr(const Object& obj) {
    return std::visit([](auto&& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, Integer>) {
            return v.str();
        } else if constexpr (std::is_same_v<T, Real>) {
            return v.str();
        } else if constexpr (std::is_same_v<T, Rational>) {
            return v.str();
        } else if constexpr (std::is_same_v<T, Complex>) {
            return "(" + v.first.str() + ", " + v.second.str() + ")";
        } else if constexpr (std::is_same_v<T, String>) {
            return "\"" + v.value + "\"";
        } else if constexpr (std::is_same_v<T, Program>) {
            return "\xC2\xAB " + repr_tokens(v.tokens) + " \xC2\xBB";
        } else if constexpr (std::is_same_v<T, Name>) {
            return "'" + v.value + "'";
        } else if constexpr (std::is_same_v<T, Error>) {
            return "Error " + std::to_string(v.code) + ": " + v.message;
        } else if constexpr (std::is_same_v<T, Symbol>) {
            return "'" + v.value + "'";
        }
    }, obj);
}

static std::string repr_tokens(const std::vector<Token>& tokens) {
    std::string result;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) result += " ";
        if (tokens[i].kind == Token::Literal) {
            result += repr(tokens[i].literal);
        } else {
            result += tokens[i].command;
        }
    }
    return result;
}

// ---------- type_tag ----------

TypeTag type_tag(const Object& obj) {
    return static_cast<TypeTag>(obj.index());
}

// ---------- serialize ----------

static std::string serialize_tokens(const std::vector<Token>& tokens);

std::string serialize(const Object& obj) {
    return std::visit([](auto&& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, Integer>) {
            return v.str();
        } else if constexpr (std::is_same_v<T, Real>) {
            return v.str();
        } else if constexpr (std::is_same_v<T, Rational>) {
            // Store as "numerator/denominator"
            return boost::multiprecision::numerator(v).str() + "/" +
                   boost::multiprecision::denominator(v).str();
        } else if constexpr (std::is_same_v<T, Complex>) {
            return v.first.str() + "|" + v.second.str();
        } else if constexpr (std::is_same_v<T, String>) {
            return v.value;
        } else if constexpr (std::is_same_v<T, Program>) {
            return serialize_tokens(v.tokens);
        } else if constexpr (std::is_same_v<T, Name>) {
            return v.value;
        } else if constexpr (std::is_same_v<T, Error>) {
            return std::to_string(v.code) + "|" + v.message;
        } else if constexpr (std::is_same_v<T, Symbol>) {
            return v.value;
        }
    }, obj);
}

// Serialize tokens as the repr form (« ... ») but without outer delimiters
static std::string serialize_tokens(const std::vector<Token>& tokens) {
    return repr_tokens(tokens);
}

// ---------- deserialize ----------

// Forward declarations for the parser (used in Program deserialization)
namespace {

Object deserialize_impl(TypeTag tag, const std::string& data) {
    switch (tag) {
        case TypeTag::Integer:
            return Integer(data);
        case TypeTag::Real:
            return Real(data);
        case TypeTag::Rational: {
            auto pos = data.find('/');
            if (pos == std::string::npos) return Integer(data);
            Integer num(data.substr(0, pos));
            Integer den(data.substr(pos + 1));
            return Rational(num, den);
        }
        case TypeTag::Complex: {
            auto pos = data.find('|');
            Real re(data.substr(0, pos));
            Real im(data.substr(pos + 1));
            return Complex{re, im};
        }
        case TypeTag::String:
            return String{data};
        case TypeTag::Program: {
            Program p;
            if (!data.empty()) {
                p.tokens = parse(data);
            }
            return p;
        }
        case TypeTag::Name:
            return Name{data};
        case TypeTag::Error: {
            auto pos = data.find('|');
            int code = std::stoi(data.substr(0, pos));
            std::string msg = data.substr(pos + 1);
            return Error{code, msg};
        }
        case TypeTag::Symbol:
            return Symbol{data};
    }
    return Error{99, "Unknown type tag"};
}

} // anonymous namespace

Object deserialize(TypeTag tag, const std::string& data) {
    return deserialize_impl(tag, data);
}

} // namespace lpr
