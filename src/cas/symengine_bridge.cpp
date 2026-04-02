#include "symengine_bridge.hpp"

#include <symengine/parser.h>
#include <symengine/symbol.h>
#include <symengine/integer.h>
#include <symengine/rational.h>
#include <symengine/add.h>
#include <symengine/mul.h>
#include <symengine/pow.h>
#include <symengine/functions.h>
#include <symengine/simplify.h>
#include <symengine/solve.h>
#include <symengine/constants.h>
#include <symengine/sets.h>
#include <symengine/visitor.h>

#include <sstream>
#include <regex>
#include <algorithm>
#include <cctype>

namespace lpr {

SymEngineBridge::SymEngineBridge() {
    // RPL uppercase → SymEngine lowercase function names
    rpl_to_symengine_ = {
        {"SIN",   "sin"},
        {"COS",   "cos"},
        {"TAN",   "tan"},
        {"ASIN",  "asin"},
        {"ACOS",  "acos"},
        {"ATAN",  "atan"},
        {"ATAN2", "atan2"},
        {"EXP",   "exp"},
        {"LN",    "log"},
        {"SQRT",  "sqrt"},
        {"ABS",   "Abs"},
    };

    // Build reverse map
    for (auto& [rpl, se] : rpl_to_symengine_) {
        symengine_to_rpl_[se] = rpl;
    }
}

std::string SymEngineBridge::extract_symbol_string(const Object& obj, const std::string& cmd) {
    if (!std::holds_alternative<Symbol>(obj)) {
        throw std::runtime_error(cmd + " requires a symbolic expression");
    }
    return std::get<Symbol>(obj).value;
}

// Convert an RPL expression string to a SymEngine expression tree.
// Handles: uppercase function names → lowercase, ^ → **, SQ(x) → (x)**2,
//          LOG(x) → log(x)/log(10), variable case lowering
SymEngine::RCP<const SymEngine::Basic> SymEngineBridge::to_symengine(const std::string& expr) {
    std::string transformed;
    transformed.reserve(expr.size() * 2);

    size_t i = 0;
    while (i < expr.size()) {
        // Skip whitespace
        if (std::isspace(static_cast<unsigned char>(expr[i]))) {
            transformed += expr[i++];
            continue;
        }

        // Try to match an identifier (function name or variable)
        if (std::isalpha(static_cast<unsigned char>(expr[i])) || expr[i] == '_') {
            std::string ident;
            size_t start = i;
            while (i < expr.size() && (std::isalnum(static_cast<unsigned char>(expr[i])) || expr[i] == '_')) {
                ident += expr[i++];
            }

            // Check for SQ — special case: SQ(x) → (x)**2
            if (ident == "SQ") {
                // Find the matching parenthesized argument
                if (i < expr.size() && expr[i] == '(') {
                    int depth = 0;
                    size_t arg_start = i + 1;
                    size_t j = i;
                    while (j < expr.size()) {
                        if (expr[j] == '(') depth++;
                        else if (expr[j] == ')') { depth--; if (depth == 0) break; }
                        j++;
                    }
                    std::string arg = expr.substr(arg_start, j - arg_start);
                    transformed += "(" + arg + ")**2";
                    i = j + 1;
                } else {
                    // SQ without parens (shouldn't happen in well-formed input)
                    transformed += ident;
                }
                continue;
            }

            // Check for LOG — special case: LOG(x) → log(x)/log(10)
            if (ident == "LOG") {
                if (i < expr.size() && expr[i] == '(') {
                    int depth = 0;
                    size_t arg_start = i + 1;
                    size_t j = i;
                    while (j < expr.size()) {
                        if (expr[j] == '(') depth++;
                        else if (expr[j] == ')') { depth--; if (depth == 0) break; }
                        j++;
                    }
                    std::string arg = expr.substr(arg_start, j - arg_start);
                    transformed += "log(" + arg + ")/log(10)";
                    i = j + 1;
                } else {
                    transformed += "log";
                }
                continue;
            }

            // Check known RPL function names
            auto it = rpl_to_symengine_.find(ident);
            if (it != rpl_to_symengine_.end()) {
                transformed += it->second;
            } else {
                // Variable name — lowercase it for SymEngine
                std::string lower = ident;
                std::transform(lower.begin(), lower.end(), lower.begin(),
                    [](unsigned char c) { return std::tolower(c); });
                transformed += lower;
            }
            continue;
        }

        // ^ → ** for exponentiation
        if (expr[i] == '^') {
            transformed += "**";
            i++;
            continue;
        }

        // Everything else passes through
        transformed += expr[i++];
    }

    try {
        return SymEngine::parse(transformed);
    } catch (const std::exception& e) {
        throw std::runtime_error("Cannot parse expression for CAS operation: " + std::string(e.what()));
    }
}

// Convert a SymEngine expression back to an RPL string.
// Maps lowercase function names back to uppercase, ** → ^, variable names to uppercase.
std::string SymEngineBridge::from_symengine(const SymEngine::RCP<const SymEngine::Basic>& expr) {
    std::ostringstream oss;
    oss << *expr;
    std::string result = oss.str();

    // Replace ** with ^
    std::string out;
    out.reserve(result.size());
    for (size_t i = 0; i < result.size(); ++i) {
        if (i + 1 < result.size() && result[i] == '*' && result[i + 1] == '*') {
            out += '^';
            i++; // skip second *
        } else {
            out += result[i];
        }
    }
    result = out;

    // Map SymEngine function names back to RPL names AND uppercase variables
    std::string final;
    final.reserve(result.size());
    size_t i = 0;
    while (i < result.size()) {
        if (std::isalpha(static_cast<unsigned char>(result[i])) || result[i] == '_') {
            std::string ident;
            while (i < result.size() && (std::isalnum(static_cast<unsigned char>(result[i])) || result[i] == '_')) {
                ident += result[i++];
            }

            // Check known SymEngine function names
            auto it = symengine_to_rpl_.find(ident);
            if (it != symengine_to_rpl_.end()) {
                final += it->second;
            } else {
                // Variable or unknown — uppercase it
                std::string upper = ident;
                std::transform(upper.begin(), upper.end(), upper.begin(),
                    [](unsigned char c) { return std::toupper(c); });
                final += upper;
            }
        } else {
            final += result[i++];
        }
    }

    return final;
}

// --- Bridge operations ---

Object SymEngineBridge::differentiate(const Object& expr, const std::string& var) {
    std::string expr_str = extract_symbol_string(expr, "DIFF");
    std::string var_lower = var;
    std::transform(var_lower.begin(), var_lower.end(), var_lower.begin(),
        [](unsigned char c) { return std::tolower(c); });

    auto se_expr = to_symengine(expr_str);
    auto se_var = SymEngine::symbol(var_lower);
    auto se_result = se_expr->diff(se_var);
    return Symbol{from_symengine(se_result)};
}

Object SymEngineBridge::integrate(const Object& expr, const std::string& var) {
    std::string expr_str = extract_symbol_string(expr, "INTEGRATE");
    std::string var_lower = var;
    std::transform(var_lower.begin(), var_lower.end(), var_lower.begin(),
        [](unsigned char c) { return std::tolower(c); });

    auto se_expr = to_symengine(expr_str);
    auto se_var = SymEngine::symbol(var_lower);

    // SymEngine doesn't have a top-level integrate() function.
    // We implement term-by-term integration for polynomials + elementary functions.
    auto expanded = SymEngine::expand(se_expr);

    try {
        auto result = integrate_term(expanded, se_var);
        return Symbol{from_symengine(result)};
    } catch (const std::runtime_error&) {
        throw;
    } catch (const std::exception&) {
        throw std::runtime_error("INTEGRATE: unable to find antiderivative");
    }
}

SymEngine::RCP<const SymEngine::Basic> SymEngineBridge::integrate_term(
    const SymEngine::RCP<const SymEngine::Basic>& term,
    const SymEngine::RCP<const SymEngine::Symbol>& var)
{
    // Sum: integrate term-by-term
    if (SymEngine::is_a<SymEngine::Add>(*term)) {
        auto args = term->get_args();
        SymEngine::RCP<const SymEngine::Basic> sum = SymEngine::integer(0);
        for (auto& t : args) {
            sum = SymEngine::add(sum, integrate_term(t, var));
        }
        return sum;
    }

    // Product with constant coefficient: factor out the constant
    if (SymEngine::is_a<SymEngine::Mul>(*term)) {
        auto args = term->get_args();
        SymEngine::RCP<const SymEngine::Basic> coeff = SymEngine::integer(1);
        SymEngine::RCP<const SymEngine::Basic> var_part = SymEngine::integer(1);
        for (auto& arg : args) {
            auto free = SymEngine::free_symbols(*arg);
            if (free.empty()) {
                coeff = SymEngine::mul(coeff, arg);
            } else {
                var_part = SymEngine::mul(var_part, arg);
            }
        }
        if (!SymEngine::eq(*coeff, *SymEngine::integer(1))) {
            return SymEngine::mul(coeff, integrate_term(var_part, var));
        }
    }

    // Constant (no var dependency) → constant * x
    auto free = SymEngine::free_symbols(*term);
    if (free.find(var) == free.end()) {
        return SymEngine::mul(term, var);
    }

    // x → x^2/2
    if (SymEngine::eq(*term, *var)) {
        return SymEngine::div(SymEngine::pow(var, SymEngine::integer(2)),
                              SymEngine::integer(2));
    }

    // x^n → x^(n+1)/(n+1) (integer exponent, n != -1)
    if (SymEngine::is_a<SymEngine::Pow>(*term)) {
        auto base = SymEngine::rcp_static_cast<const SymEngine::Pow>(term)->get_base();
        auto exp_val = SymEngine::rcp_static_cast<const SymEngine::Pow>(term)->get_exp();
        if (SymEngine::eq(*base, *var)) {
            auto new_exp = SymEngine::add(exp_val, SymEngine::integer(1));
            if (SymEngine::eq(*new_exp, *SymEngine::integer(0))) {
                // x^(-1) → ln(x) — return as Log
                return SymEngine::log(var);
            }
            return SymEngine::div(SymEngine::pow(var, new_exp), new_exp);
        }
        // exp(x) is represented as E^x
        if (SymEngine::eq(*base, *SymEngine::E) && SymEngine::eq(*exp_val, *var)) {
            return term; // integral of e^x is e^x
        }
    }

    // sin(x) → -cos(x)
    if (SymEngine::is_a<SymEngine::Sin>(*term)) {
        auto arg = term->get_args()[0];
        if (SymEngine::eq(*arg, *var)) {
            return SymEngine::mul(SymEngine::integer(-1), SymEngine::cos(var));
        }
    }

    // cos(x) → sin(x)
    if (SymEngine::is_a<SymEngine::Cos>(*term)) {
        auto arg = term->get_args()[0];
        if (SymEngine::eq(*arg, *var)) {
            return SymEngine::sin(var);
        }
    }

    throw std::runtime_error("INTEGRATE: unable to find antiderivative");
}

Object SymEngineBridge::solve(const Object& expr, const std::string& var) {
    std::string expr_str = extract_symbol_string(expr, "SOLVE");
    std::string var_lower = var;
    std::transform(var_lower.begin(), var_lower.end(), var_lower.begin(),
        [](unsigned char c) { return std::tolower(c); });

    auto se_expr = to_symengine(expr_str);
    auto se_var = SymEngine::symbol(var_lower);

    auto solution_set = SymEngine::solve(se_expr, se_var);

    // Convert solution set to a List of Objects
    List result;

    if (SymEngine::is_a<SymEngine::FiniteSet>(*solution_set)) {
        auto& container = SymEngine::rcp_static_cast<const SymEngine::FiniteSet>(solution_set)->get_container();
        for (auto& sol : container) {
            if (SymEngine::is_a<SymEngine::Integer>(*sol)) {
                auto& val = SymEngine::rcp_static_cast<const SymEngine::Integer>(sol)->as_integer_class();
                result.items.push_back(Integer(val));
            } else if (SymEngine::is_a<SymEngine::Rational>(*sol)) {
                auto num = SymEngine::rcp_static_cast<const SymEngine::Rational>(sol)->get_num();
                auto den = SymEngine::rcp_static_cast<const SymEngine::Rational>(sol)->get_den();
                Rational r(num->as_integer_class(), den->as_integer_class());
                result.items.push_back(r);
            } else {
                // Symbolic solution
                result.items.push_back(Symbol{from_symengine(sol)});
            }
        }
    } else if (SymEngine::is_a<SymEngine::EmptySet>(*solution_set)) {
        // No solutions — return empty list
    }
    // Other set types (intervals, etc.) — return empty list for unsupported

    return result;
}

Object SymEngineBridge::simplify(const Object& expr) {
    std::string expr_str = extract_symbol_string(expr, "SIMPLIFY");
    auto se_expr = to_symengine(expr_str);
    auto se_result = SymEngine::simplify(se_expr);
    return Symbol{from_symengine(se_result)};
}

Object SymEngineBridge::expand(const Object& expr) {
    std::string expr_str = extract_symbol_string(expr, "EXPAND");
    auto se_expr = to_symengine(expr_str);
    auto se_result = SymEngine::expand(se_expr);
    return Symbol{from_symengine(se_result)};
}

Object SymEngineBridge::factor(const Object& expr) {
    std::string expr_str = extract_symbol_string(expr, "FACTOR");
    auto se_expr = to_symengine(expr_str);
    auto expanded = SymEngine::expand(se_expr);

    // Use solve to find roots, then determine multiplicity via successive derivatives
    auto free_syms = SymEngine::free_symbols(*se_expr);
    if (free_syms.size() == 1) {
        auto se_var = SymEngine::rcp_static_cast<const SymEngine::Symbol>(*free_syms.begin());
        // Solve over reals only — factor over the integers, not complex
        auto solution_set = SymEngine::solve(se_expr, se_var, SymEngine::reals());

        if (SymEngine::is_a<SymEngine::FiniteSet>(*solution_set)) {
            auto& container = SymEngine::rcp_static_cast<const SymEngine::FiniteSet>(
                solution_set)->get_container();

            if (!container.empty()) {
                SymEngine::RCP<const SymEngine::Basic> factored = SymEngine::integer(1);
                int total_degree = 0;

                for (auto& root : container) {
                    // Determine multiplicity: f(root)=0, f'(root)=0, ..., f^(k)(root)!=0
                    // means multiplicity is k
                    int mult = 0;
                    auto deriv = expanded;
                    SymEngine::map_basic_basic subs_map;
                    subs_map[se_var] = root;
                    for (int k = 0; k < 20; ++k) { // safety limit
                        auto val = deriv->subs(subs_map);
                        val = SymEngine::expand(val);
                        if (!SymEngine::eq(*val, *SymEngine::integer(0))) break;
                        mult++;
                        deriv = deriv->diff(se_var);
                    }

                    auto linear = SymEngine::sub(se_var, root);
                    factored = SymEngine::mul(factored,
                        SymEngine::pow(linear, SymEngine::integer(mult)));
                    total_degree += mult;
                }

                // Compute leading coefficient: divide expanded by x^total_degree's coeff
                // by evaluating ratio at a point
                auto factored_expanded = SymEngine::expand(factored);
                SymEngine::map_basic_basic test_map;
                // Use a large test value to extract the leading coefficient
                test_map[se_var] = SymEngine::integer(1000);
                auto orig_val = expanded->subs(test_map);
                auto fact_val = factored_expanded->subs(test_map);
                if (!SymEngine::eq(*fact_val, *SymEngine::integer(0))) {
                    auto ratio = SymEngine::div(orig_val, fact_val);
                    ratio = SymEngine::expand(ratio);
                    if (!SymEngine::eq(*ratio, *SymEngine::integer(1))) {
                        factored = SymEngine::mul(ratio, factored);
                    }
                }

                return Symbol{from_symengine(factored)};
            }
        }
    }

    // If factoring fails, return the original expression
    return Symbol{from_symengine(se_expr)};
}

} // namespace lpr
