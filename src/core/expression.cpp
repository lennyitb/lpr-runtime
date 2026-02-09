#include "core/expression.hpp"
#include "core/context.hpp"
#include <vector>
#include <string>
#include <cctype>
#include <stdexcept>
#include <cmath>

namespace lpr {

namespace {

// --- Expression tokenizer ---

enum class ExprTokenType { Number, Name, Op, LParen, RParen };

struct ExprToken {
    ExprTokenType type;
    std::string value;
};

std::vector<ExprToken> tokenize_expression(const std::string& expr) {
    std::vector<ExprToken> tokens;
    size_t i = 0;
    size_t len = expr.size();

    while (i < len) {
        char c = expr[i];

        if (c == ' ' || c == '\t') { ++i; continue; }

        if (c == '(') { tokens.push_back({ExprTokenType::LParen, "("}); ++i; continue; }
        if (c == ')') { tokens.push_back({ExprTokenType::RParen, ")"}); ++i; continue; }

        // Operators: +, -, *, /, ^
        if (c == '+' || c == '*' || c == '/' || c == '^') {
            tokens.push_back({ExprTokenType::Op, std::string(1, c)});
            ++i;
            continue;
        }

        // Minus: could be unary negation or binary subtraction
        if (c == '-') {
            // Unary if at start, or after operator or left paren
            bool unary = tokens.empty() ||
                         tokens.back().type == ExprTokenType::Op ||
                         tokens.back().type == ExprTokenType::LParen;
            if (unary && i + 1 < len && (std::isdigit(static_cast<unsigned char>(expr[i+1])) || expr[i+1] == '.')) {
                // Negative number literal
                size_t start = i;
                ++i;
                bool has_dot = false;
                bool has_e = false;
                while (i < len) {
                    char d = expr[i];
                    if (std::isdigit(static_cast<unsigned char>(d))) { ++i; continue; }
                    if (d == '.' && !has_dot && !has_e) { has_dot = true; ++i; continue; }
                    if ((d == 'E' || d == 'e') && !has_e) {
                        has_e = true; ++i;
                        if (i < len && (expr[i] == '+' || expr[i] == '-')) ++i;
                        continue;
                    }
                    break;
                }
                tokens.push_back({ExprTokenType::Number, expr.substr(start, i - start)});
            } else if (unary) {
                // Unary negation followed by a name or paren — emit NEG operator
                tokens.push_back({ExprTokenType::Op, "NEG"});
                ++i;
            } else {
                tokens.push_back({ExprTokenType::Op, "-"});
                ++i;
            }
            continue;
        }

        // Number literal
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            size_t start = i;
            bool has_dot = (c == '.');
            bool has_e = false;
            ++i;
            while (i < len) {
                char d = expr[i];
                if (std::isdigit(static_cast<unsigned char>(d))) { ++i; continue; }
                if (d == '.' && !has_dot && !has_e) { has_dot = true; ++i; continue; }
                if ((d == 'E' || d == 'e') && !has_e) {
                    has_e = true; ++i;
                    if (i < len && (expr[i] == '+' || expr[i] == '-')) ++i;
                    continue;
                }
                break;
            }
            tokens.push_back({ExprTokenType::Number, expr.substr(start, i - start)});
            continue;
        }

        // Name (variable or function name): starts with letter or underscore
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            size_t start = i;
            ++i;
            while (i < len && (std::isalnum(static_cast<unsigned char>(expr[i])) || expr[i] == '_')) ++i;
            tokens.push_back({ExprTokenType::Name, expr.substr(start, i - start)});
            continue;
        }

        throw std::runtime_error(std::string("Unexpected character in expression: ") + c);
    }

    return tokens;
}

// --- Shunting-yard: infix to RPN ---

int precedence(const std::string& op) {
    if (op == "+" || op == "-") return 1;
    if (op == "*" || op == "/") return 2;
    if (op == "^")              return 3;
    if (op == "NEG")            return 4; // unary negation, highest
    return 0;
}

bool is_right_assoc(const std::string& op) {
    return op == "^" || op == "NEG";
}

std::vector<ExprToken> shunting_yard(const std::vector<ExprToken>& tokens) {
    std::vector<ExprToken> output;
    std::vector<ExprToken> op_stack;

    for (const auto& tok : tokens) {
        switch (tok.type) {
            case ExprTokenType::Number:
            case ExprTokenType::Name:
                output.push_back(tok);
                break;
            case ExprTokenType::Op: {
                while (!op_stack.empty() && op_stack.back().type == ExprTokenType::Op) {
                    auto& top = op_stack.back();
                    if (is_right_assoc(tok.value)
                        ? precedence(top.value) > precedence(tok.value)
                        : precedence(top.value) >= precedence(tok.value)) {
                        output.push_back(top);
                        op_stack.pop_back();
                    } else {
                        break;
                    }
                }
                op_stack.push_back(tok);
                break;
            }
            case ExprTokenType::LParen:
                op_stack.push_back(tok);
                break;
            case ExprTokenType::RParen:
                while (!op_stack.empty() && op_stack.back().type != ExprTokenType::LParen) {
                    output.push_back(op_stack.back());
                    op_stack.pop_back();
                }
                if (op_stack.empty()) {
                    throw std::runtime_error("Mismatched parentheses");
                }
                op_stack.pop_back(); // discard LParen
                break;
        }
    }

    while (!op_stack.empty()) {
        if (op_stack.back().type == ExprTokenType::LParen) {
            throw std::runtime_error("Mismatched parentheses");
        }
        output.push_back(op_stack.back());
        op_stack.pop_back();
    }

    return output;
}

// --- RPN Evaluator ---

// Numeric helpers (reuse same promotion logic as commands.cpp)
int numeric_rank(const Object& obj) {
    if (std::holds_alternative<Integer>(obj))  return 0;
    if (std::holds_alternative<Rational>(obj)) return 1;
    if (std::holds_alternative<Real>(obj))     return 2;
    return -1;
}

Object promote(const Object& obj, int target_rank) {
    int rank = numeric_rank(obj);
    if (rank == target_rank) return obj;
    Object cur = obj;
    int cur_rank = rank;
    while (cur_rank < target_rank) {
        if (cur_rank == 0) {
            cur = Rational(std::get<Integer>(cur), Integer(1));
            cur_rank = 1;
        } else if (cur_rank == 1) {
            cur = Real(std::get<Rational>(cur));
            cur_rank = 2;
        }
    }
    return cur;
}

Object apply_binary(const std::string& op, const Object& a, const Object& b) {
    int ra = numeric_rank(a);
    int rb = numeric_rank(b);
    if (ra < 0 || rb < 0) throw std::runtime_error("Non-numeric value in expression");

    int target = std::max(ra, rb);
    // For division, promote integers to rationals
    if (op == "/" && target == 0) target = 1;

    Object pa = promote(a, target);
    Object pb = promote(b, target);

    if (op == "+") {
        if (target == 0) return Integer(std::get<Integer>(pa) + std::get<Integer>(pb));
        if (target == 1) return Rational(std::get<Rational>(pa) + std::get<Rational>(pb));
        return Real(std::get<Real>(pa) + std::get<Real>(pb));
    }
    if (op == "-") {
        if (target == 0) return Integer(std::get<Integer>(pa) - std::get<Integer>(pb));
        if (target == 1) return Rational(std::get<Rational>(pa) - std::get<Rational>(pb));
        return Real(std::get<Real>(pa) - std::get<Real>(pb));
    }
    if (op == "*") {
        if (target == 0) return Integer(std::get<Integer>(pa) * std::get<Integer>(pb));
        if (target == 1) return Rational(std::get<Rational>(pa) * std::get<Rational>(pb));
        return Real(std::get<Real>(pa) * std::get<Real>(pb));
    }
    if (op == "/") {
        // Check division by zero
        if ((target == 1 && std::get<Rational>(pb) == 0) ||
            (target == 2 && std::get<Real>(pb) == 0)) {
            throw std::runtime_error("Division by zero");
        }
        if (target == 1) return Rational(std::get<Rational>(pa) / std::get<Rational>(pb));
        return Real(std::get<Real>(pa) / std::get<Real>(pb));
    }
    if (op == "^") {
        // Power: promote everything to Real for simplicity
        Object ra_obj = promote(pa, 2);
        Object rb_obj = promote(pb, 2);
        Real base = std::get<Real>(ra_obj);
        Real exp = std::get<Real>(rb_obj);
        return Real(boost::multiprecision::pow(base, exp));
    }

    throw std::runtime_error("Unknown operator: " + op);
}

Object eval_rpn(const std::vector<ExprToken>& rpn, Context& ctx) {
    std::vector<Object> stack;

    for (const auto& tok : rpn) {
        if (tok.type == ExprTokenType::Number) {
            // Parse number: if it contains '.' or 'E'/'e', it's Real; otherwise Integer
            const auto& s = tok.value;
            bool is_real = false;
            for (char c : s) {
                if (c == '.' || c == 'E' || c == 'e') { is_real = true; break; }
            }
            if (is_real) {
                stack.push_back(Real(s));
            } else {
                stack.push_back(Integer(s));
            }
        } else if (tok.type == ExprTokenType::Name) {
            // Resolve variable: locals first, then global store
            auto local = ctx.resolve_local(tok.value);
            if (local.has_value()) {
                stack.push_back(*local);
            } else {
                // Try uppercased name for global store lookup
                std::string upper = tok.value;
                std::transform(upper.begin(), upper.end(), upper.begin(),
                    [](unsigned char c) { return std::toupper(c); });
                Object val = ctx.store().recall_variable(ctx.store().current_dir(), upper);
                if (std::holds_alternative<Error>(val)) {
                    throw std::runtime_error("Undefined variable: " + tok.value);
                }
                stack.push_back(val);
            }
        } else if (tok.type == ExprTokenType::Op) {
            if (tok.value == "NEG") {
                if (stack.empty()) throw std::runtime_error("Malformed expression");
                Object a = stack.back(); stack.pop_back();
                if (std::holds_alternative<Integer>(a)) {
                    stack.push_back(Integer(-std::get<Integer>(a)));
                } else if (std::holds_alternative<Rational>(a)) {
                    stack.push_back(Rational(-std::get<Rational>(a)));
                } else if (std::holds_alternative<Real>(a)) {
                    stack.push_back(Real(-std::get<Real>(a)));
                } else {
                    throw std::runtime_error("Non-numeric value in expression");
                }
            } else {
                if (stack.size() < 2) throw std::runtime_error("Malformed expression");
                Object b = stack.back(); stack.pop_back();
                Object a = stack.back(); stack.pop_back();
                stack.push_back(apply_binary(tok.value, a, b));
            }
        }
    }

    if (stack.size() != 1) throw std::runtime_error("Malformed expression");
    return stack[0];
}

} // anonymous namespace

Object eval_expression(const std::string& expr, Context& ctx) {
    auto tokens = tokenize_expression(expr);
    auto rpn = shunting_yard(tokens);
    return eval_rpn(rpn, ctx);
}

} // namespace lpr
