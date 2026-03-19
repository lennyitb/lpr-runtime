#pragma once

#include "core/object.hpp"
#include <string>
#include <vector>

namespace lpr {

class Context;

// --- Expression token types (exposed for EXPLODE/SUBST) ---

enum class ExprTokenType { Number, Name, Op, LParen, RParen, Comma };

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
// Returns the numeric result.
Object eval_expression(const std::string& expr, Context& ctx);

} // namespace lpr
