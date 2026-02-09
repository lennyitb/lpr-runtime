#pragma once

#include "core/object.hpp"
#include <string>

namespace lpr {

class Context;

// Evaluate an infix expression string (from a Symbol).
// Resolves variables via context (locals first, then global store).
// Returns the numeric result.
Object eval_expression(const std::string& expr, Context& ctx);

} // namespace lpr
