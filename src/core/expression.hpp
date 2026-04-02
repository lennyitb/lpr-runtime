#pragma once

#include "core/object.hpp"
#include <string>
#include <vector>

namespace lpr {

class Context;

// --- Expression token types (exposed for EXPLODE/SUBST) ---

enum class ExprTokenType { Number, Name, Op, LParen, RParen, Comma, Func };

struct ExprToken {
    ExprTokenType type;
    std::string value;
};

// Tokenize an infix expression string into tokens.
std::vector<ExprToken> tokenize_expression(const std::string& expr);

// Operator precedence (exposed for EXPLODE).
int precedence(const std::string& op);

// Check if an expression string needs parentheses when used as an operand
// of an operator with the given precedence.
bool needs_parens(const std::string& expr, int outer_prec);

// Evaluate an infix expression string (from a Symbol).
// Resolves variables via context (locals first, then global store).
// Uses exact arithmetic: operations that would lose exactness (e.g. SQRT
// on a non-perfect square) produce symbolic results instead of Reals.
Object eval_expression(const std::string& expr, Context& ctx);

// Evaluate forcing numeric results (used by ->NUM).
Object eval_expression_numeric(const std::string& expr, Context& ctx);

} // namespace lpr
