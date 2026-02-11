#include "core/commands.hpp"
#include "core/context.hpp"
#include "core/parser.hpp"
#include "core/expression.hpp"
#include <cmath>
#include <algorithm>

namespace lpr {

// ---- Numeric helpers: type promotion ----

namespace {

// Returns the "rank" in the numeric tower
int numeric_rank(const Object& obj) {
    if (std::holds_alternative<Integer>(obj))  return 0;
    if (std::holds_alternative<Rational>(obj)) return 1;
    if (std::holds_alternative<Real>(obj))     return 2;
    if (std::holds_alternative<Complex>(obj))  return 3;
    return -1;
}

bool is_numeric(const Object& obj) {
    return numeric_rank(obj) >= 0;
}

// Promote an object to the target rank
Object promote(const Object& obj, int target_rank) {
    int rank = numeric_rank(obj);
    if (rank == target_rank) return obj;

    // Promote step by step
    Object cur = obj;
    int cur_rank = rank;

    while (cur_rank < target_rank) {
        if (cur_rank == 0) {
            // Integer -> Rational
            auto& v = std::get<Integer>(cur);
            cur = Rational(v, Integer(1));
            cur_rank = 1;
        } else if (cur_rank == 1) {
            // Rational -> Real
            auto& v = std::get<Rational>(cur);
            Real result(v);
            cur = result;
            cur_rank = 2;
        } else if (cur_rank == 2) {
            // Real -> Complex
            auto& v = std::get<Real>(cur);
            cur = Complex{v, Real(0)};
            cur_rank = 3;
        }
    }
    return cur;
}

// Helper to do binary numeric ops
using IntOp  = std::function<Integer(const Integer&, const Integer&)>;
using RatOp  = std::function<Rational(const Rational&, const Rational&)>;
using RealOp = std::function<Real(const Real&, const Real&)>;
using CmplOp = std::function<Complex(const Complex&, const Complex&)>;

Object binary_numeric(const Object& a, const Object& b,
                      IntOp iop, RatOp rop, RealOp reop, CmplOp cop,
                      bool int_div_to_rational = false) {
    int ra = numeric_rank(a);
    int rb = numeric_rank(b);
    if (ra < 0 || rb < 0) return Error{10, "Bad argument type"};

    int target = std::max(ra, rb);
    if (int_div_to_rational && target == 0) target = 1;

    Object pa = promote(a, target);
    Object pb = promote(b, target);

    switch (target) {
        case 0: return iop(std::get<Integer>(pa), std::get<Integer>(pb));
        case 1: return rop(std::get<Rational>(pa), std::get<Rational>(pb));
        case 2: return reop(std::get<Real>(pa), std::get<Real>(pb));
        case 3: return cop(std::get<Complex>(pa), std::get<Complex>(pb));
    }
    return Error{10, "Bad argument type"};
}

// Check if an object is symbolic (Name or Symbol)
bool is_symbolic(const Object& obj) {
    return std::holds_alternative<Name>(obj) || std::holds_alternative<Symbol>(obj);
}

// Convert an object to its expression string form for building symbolic expressions
std::string to_expr_string(const Object& obj) {
    if (std::holds_alternative<Name>(obj))   return std::get<Name>(obj).value;
    if (std::holds_alternative<Symbol>(obj)) return std::get<Symbol>(obj).value;
    return repr(obj);
}

// Determine if an expression string needs parentheses when used as an operand
// of an operator with the given precedence
bool needs_parens(const std::string& expr, int outer_prec) {
    // Simple heuristic: if the expression contains +/- at the top level,
    // it has precedence 1. Wrap it if outer operator has higher precedence.
    int depth = 0;
    int min_prec = 10;
    for (char c : expr) {
        if (c == '(') { ++depth; continue; }
        if (c == ')') { --depth; continue; }
        if (depth > 0) continue;
        if (c == '+' || c == '-') min_prec = std::min(min_prec, 1);
        if (c == '*' || c == '/') min_prec = std::min(min_prec, 2);
    }
    return min_prec < outer_prec;
}

// Build a symbolic expression from a binary operation
Object symbolic_binary(const Object& a, const Object& b, const std::string& op) {
    std::string sa = to_expr_string(a);
    std::string sb = to_expr_string(b);

    // Add parens based on operator precedence
    int prec = (op == "+" || op == "-") ? 1 : 2;  // * and / are 2
    if (needs_parens(sa, prec)) sa = "(" + sa + ")";
    if (needs_parens(sb, prec)) sb = "(" + sb + ")";

    return Symbol{sa + op + sb};
}

// Check if an object is "truthy" (non-zero numeric)
bool is_truthy(const Object& obj) {
    if (std::holds_alternative<Integer>(obj))  return std::get<Integer>(obj) != 0;
    if (std::holds_alternative<Real>(obj))     return std::get<Real>(obj) != 0;
    if (std::holds_alternative<Rational>(obj)) return std::get<Rational>(obj) != 0;
    if (std::holds_alternative<Complex>(obj)) {
        auto& c = std::get<Complex>(obj);
        return c.first != 0 || c.second != 0;
    }
    return false;
}

// Extract a Real from a numeric Object (Integer, Rational, or Real)
Real to_real_value(const Object& obj) {
    if (std::holds_alternative<Integer>(obj)) return Real(std::get<Integer>(obj));
    if (std::holds_alternative<Rational>(obj)) return Real(std::get<Rational>(obj));
    if (std::holds_alternative<Real>(obj)) return std::get<Real>(obj);
    throw std::runtime_error("Bad argument type");
}

double to_double_value(const Object& obj) {
    if (std::holds_alternative<Integer>(obj)) return std::get<Integer>(obj).convert_to<double>();
    if (std::holds_alternative<Rational>(obj)) return std::get<Rational>(obj).convert_to<double>();
    if (std::holds_alternative<Real>(obj)) return std::get<Real>(obj).convert_to<double>();
    throw std::runtime_error("Bad argument type");
}

} // anonymous namespace

// ---- Command Registry ----

CommandRegistry::CommandRegistry() {
    register_stack_commands();
    register_arithmetic_commands();
    register_comparison_commands();
    register_type_commands();
    register_filesystem_commands();
    register_program_commands();
    register_logic_commands();
    register_transcendental_commands();
    register_string_commands();
}

void CommandRegistry::register_command(const std::string& name, CommandFn fn) {
    commands_[name] = std::move(fn);
}

bool CommandRegistry::has(const std::string& name) const {
    return commands_.count(name) > 0;
}

void CommandRegistry::execute(const std::string& name, Store& store, Context& ctx) const {
    auto it = commands_.find(name);
    if (it == commands_.end()) {
        store.push(Error{4, "Unknown command: " + name});
        throw std::runtime_error("Unknown command: " + name);
    }
    it->second(store, ctx);
}

// ---- Stack Commands ----

void CommandRegistry::register_stack_commands() {
    register_command("DUP", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object top = s.peek(1);
        s.push(top);
    });

    register_command("DROP", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        s.pop();
    });

    register_command("SWAP", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object a = s.pop(); // level 1
        Object b = s.pop(); // level 2
        s.push(a);
        s.push(b);
    });

    register_command("OVER", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object lv2 = s.peek(2);
        s.push(lv2);
    });

    register_command("ROT", [](Store& s, Context&) {
        if (s.depth() < 3) throw std::runtime_error("Too few arguments");
        Object c = s.pop(); // level 1
        Object b = s.pop(); // level 2
        Object a = s.pop(); // level 3
        s.push(b);
        s.push(c);
        s.push(a);
    });

    register_command("DEPTH", [](Store& s, Context&) {
        int d = s.depth();
        s.push(Integer(d));
    });

    register_command("CLEAR", [](Store& s, Context&) {
        s.clear_stack();
    });

    // UNROT: reverse of ROT — move top to level 3
    // ( a b c -- c a b )
    register_command("UNROT", [](Store& s, Context&) {
        if (s.depth() < 3) throw std::runtime_error("Too few arguments");
        Object c = s.pop(); // level 1
        Object b = s.pop(); // level 2
        Object a = s.pop(); // level 3
        s.push(c);
        s.push(a);
        s.push(b);
    });

    // DUP2: duplicate top two items
    // ( a b -- a b a b )
    register_command("DUP2", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object lv2 = s.peek(2);
        Object lv1 = s.peek(1);
        s.push(lv2);
        s.push(lv1);
    });

    // DUPN: pop n, duplicate top n items
    // ( ... x1..xn n -- ... x1..xn x1..xn )
    register_command("DUPN", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object n_obj = s.pop();
        if (!std::holds_alternative<Integer>(n_obj))
            throw std::runtime_error("Bad argument type");
        int n = static_cast<int>(std::get<Integer>(n_obj));
        if (n < 0 || s.depth() < n) throw std::runtime_error("Too few arguments");
        // Collect items from top n levels (deepest first)
        std::vector<Object> items;
        for (int i = n; i >= 1; i--) {
            items.push_back(s.peek(i));
        }
        // Push copies
        for (auto& obj : items) {
            s.push(obj);
        }
    });

    // DROP2: drop top two items
    // ( a b -- )
    register_command("DROP2", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        s.pop();
        s.pop();
    });

    // DROPN: pop n, drop top n items
    // ( ... x1..xn n -- ... )
    register_command("DROPN", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object n_obj = s.pop();
        if (!std::holds_alternative<Integer>(n_obj))
            throw std::runtime_error("Bad argument type");
        int n = static_cast<int>(std::get<Integer>(n_obj));
        if (n < 0 || s.depth() < n) throw std::runtime_error("Too few arguments");
        for (int i = 0; i < n; i++) {
            s.pop();
        }
    });

    // PICK: pop n, copy nth item to top
    // ( ... xn ... x1 n -- ... xn ... x1 xn )
    register_command("PICK", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object n_obj = s.pop();
        if (!std::holds_alternative<Integer>(n_obj))
            throw std::runtime_error("Bad argument type");
        int n = static_cast<int>(std::get<Integer>(n_obj));
        if (n < 1 || s.depth() < n) throw std::runtime_error("Too few arguments");
        Object picked = s.peek(n);
        s.push(picked);
    });

    // ROLL: pop n, roll nth item to top
    // ( ... xn xn-1 ... x1 n -- ... xn-1 ... x1 xn )
    register_command("ROLL", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object n_obj = s.pop();
        if (!std::holds_alternative<Integer>(n_obj))
            throw std::runtime_error("Bad argument type");
        int n = static_cast<int>(std::get<Integer>(n_obj));
        if (n < 1 || s.depth() < n) throw std::runtime_error("Too few arguments");
        if (n == 1) return; // no-op
        // Pop top n-1 items
        std::vector<Object> saved;
        for (int i = 0; i < n - 1; i++) {
            saved.push_back(s.pop());
        }
        // Pop the target (nth item)
        Object target = s.pop();
        // Push back saved items in reverse order
        for (int i = static_cast<int>(saved.size()) - 1; i >= 0; i--) {
            s.push(saved[i]);
        }
        // Push target on top
        s.push(target);
    });

    // ROLLD: pop n, roll top item down to nth position
    // ( ... xn xn-1 ... x1 n -- ... x1 xn xn-1 ... x2 )
    register_command("ROLLD", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object n_obj = s.pop();
        if (!std::holds_alternative<Integer>(n_obj))
            throw std::runtime_error("Bad argument type");
        int n = static_cast<int>(std::get<Integer>(n_obj));
        if (n < 1 || s.depth() < n) throw std::runtime_error("Too few arguments");
        if (n == 1) return; // no-op
        // Pop top item
        Object top = s.pop();
        // Pop remaining n-1 items
        std::vector<Object> remaining;
        for (int i = 0; i < n - 1; i++) {
            remaining.push_back(s.pop());
        }
        // Push top first (goes to deepest position)
        s.push(top);
        // Push remaining back in reverse order
        for (int i = static_cast<int>(remaining.size()) - 1; i >= 0; i--) {
            s.push(remaining[i]);
        }
    });

    // UNPICK: pop n, then pop obj, replace nth item with obj
    // ( ... xn ... x1 obj n -- ... obj ... x1 )
    register_command("UNPICK", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object n_obj = s.pop();
        if (!std::holds_alternative<Integer>(n_obj))
            throw std::runtime_error("Bad argument type");
        int n = static_cast<int>(std::get<Integer>(n_obj));
        Object obj = s.pop();
        if (n < 1 || s.depth() < n) throw std::runtime_error("Too few arguments");
        // Pop top n-1 items
        std::vector<Object> saved;
        for (int i = 0; i < n - 1; i++) {
            saved.push_back(s.pop());
        }
        // Pop and discard the item at level n
        s.pop();
        // Push the replacement
        s.push(obj);
        // Push back saved items in reverse order
        for (int i = static_cast<int>(saved.size()) - 1; i >= 0; i--) {
            s.push(saved[i]);
        }
    });
}

// ---- Arithmetic Commands ----

void CommandRegistry::register_arithmetic_commands() {
    // +
    register_command("+", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        // String concatenation
        if (std::holds_alternative<String>(a) && std::holds_alternative<String>(b)) {
            s.push(String{std::get<String>(a).value + std::get<String>(b).value});
            return;
        }
        if (std::holds_alternative<String>(a) || std::holds_alternative<String>(b)) {
            s.push(a); s.push(b);
            throw std::runtime_error("Bad argument type");
        }
        if (is_symbolic(a) || is_symbolic(b)) {
            s.push(symbolic_binary(a, b, "+"));
            return;
        }
        Object result = binary_numeric(a, b,
            [](const Integer& x, const Integer& y) -> Integer { return x + y; },
            [](const Rational& x, const Rational& y) -> Rational { return x + y; },
            [](const Real& x, const Real& y) -> Real { return x + y; },
            [](const Complex& x, const Complex& y) -> Complex {
                return {x.first + y.first, x.second + y.second};
            });
        s.push(result);
    });

    // -
    register_command("-", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop(); // level 1
        Object a = s.pop(); // level 2
        if (is_symbolic(a) || is_symbolic(b)) {
            s.push(symbolic_binary(a, b, "-"));
            return;
        }
        Object result = binary_numeric(a, b,
            [](const Integer& x, const Integer& y) -> Integer { return x - y; },
            [](const Rational& x, const Rational& y) -> Rational { return x - y; },
            [](const Real& x, const Real& y) -> Real { return x - y; },
            [](const Complex& x, const Complex& y) -> Complex {
                return {x.first - y.first, x.second - y.second};
            });
        s.push(result);
    });

    // *
    register_command("*", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (is_symbolic(a) || is_symbolic(b)) {
            s.push(symbolic_binary(a, b, "*"));
            return;
        }
        Object result = binary_numeric(a, b,
            [](const Integer& x, const Integer& y) -> Integer { return x * y; },
            [](const Rational& x, const Rational& y) -> Rational { return x * y; },
            [](const Real& x, const Real& y) -> Real { return x * y; },
            [](const Complex& x, const Complex& y) -> Complex {
                return {x.first * y.first - x.second * y.second,
                        x.first * y.second + x.second * y.first};
            });
        s.push(result);
    });

    // /
    register_command("/", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop(); // divisor (level 1)
        Object a = s.pop(); // dividend (level 2)

        if (is_symbolic(a) || is_symbolic(b)) {
            s.push(symbolic_binary(a, b, "/"));
            return;
        }

        // Check for division by zero
        if ((std::holds_alternative<Integer>(b) && std::get<Integer>(b) == 0) ||
            (std::holds_alternative<Real>(b) && std::get<Real>(b) == 0) ||
            (std::holds_alternative<Rational>(b) && std::get<Rational>(b) == 0)) {
            s.push(a);
            s.push(b);
            throw std::runtime_error("Division by zero");
        }

        Object result = binary_numeric(a, b,
            // Integer / Integer -> Rational
            [](const Integer& x, const Integer& y) -> Integer { return x / y; }, // won't be called
            [](const Rational& x, const Rational& y) -> Rational { return x / y; },
            [](const Real& x, const Real& y) -> Real { return x / y; },
            [](const Complex& x, const Complex& y) -> Complex {
                Real denom = y.first * y.first + y.second * y.second;
                return {(x.first * y.first + x.second * y.second) / denom,
                        (x.second * y.first - x.first * y.second) / denom};
            },
            true); // int_div_to_rational
        s.push(result);
    });

    // NEG
    register_command("NEG", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (is_symbolic(a)) {
            std::string sa = to_expr_string(a);
            // Always wrap in parens for negation to be unambiguous
            s.push(Symbol{"-("+sa+")"});
            return;
        }
        std::visit([&s](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, Integer>) {
                s.push(Integer(-v));
            } else if constexpr (std::is_same_v<T, Real>) {
                s.push(Real(-v));
            } else if constexpr (std::is_same_v<T, Rational>) {
                s.push(Rational(-v));
            } else if constexpr (std::is_same_v<T, Complex>) {
                s.push(Complex{-v.first, -v.second});
            } else {
                s.push(Error{10, "Bad argument type"});
                throw std::runtime_error("Bad argument type");
            }
        }, a);
    });

    // INV (1/x)
    register_command("INV", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (std::holds_alternative<Integer>(a)) {
            auto& v = std::get<Integer>(a);
            if (v == 0) {
                s.push(a);
                throw std::runtime_error("Division by zero");
            }
            s.push(Rational(Integer(1), v));
        } else if (std::holds_alternative<Rational>(a)) {
            auto& v = std::get<Rational>(a);
            if (v == 0) {
                s.push(a);
                throw std::runtime_error("Division by zero");
            }
            s.push(Rational(boost::multiprecision::denominator(v),
                            boost::multiprecision::numerator(v)));
        } else if (std::holds_alternative<Real>(a)) {
            auto& v = std::get<Real>(a);
            if (v == 0) {
                s.push(a);
                throw std::runtime_error("Division by zero");
            }
            s.push(Real(Real(1) / v));
        } else if (std::holds_alternative<Complex>(a)) {
            auto& v = std::get<Complex>(a);
            Real denom = v.first * v.first + v.second * v.second;
            if (denom == 0) {
                s.push(a);
                throw std::runtime_error("Division by zero");
            }
            s.push(Complex{v.first / denom, -v.second / denom});
        } else {
            s.push(a);
            throw std::runtime_error("Bad argument type");
        }
    });

    // ABS
    register_command("ABS", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (std::holds_alternative<Integer>(a)) {
            auto& v = std::get<Integer>(a);
            s.push(v < 0 ? Integer(-v) : v);
        } else if (std::holds_alternative<Rational>(a)) {
            auto& v = std::get<Rational>(a);
            s.push(v < 0 ? Rational(-v) : v);
        } else if (std::holds_alternative<Real>(a)) {
            auto& v = std::get<Real>(a);
            s.push(v < 0 ? Real(-v) : v);
        } else if (std::holds_alternative<Complex>(a)) {
            auto& v = std::get<Complex>(a);
            // |z| = sqrt(re^2 + im^2)
            Real mag = boost::multiprecision::sqrt(v.first * v.first + v.second * v.second);
            s.push(mag);
        } else {
            s.push(a);
            throw std::runtime_error("Bad argument type");
        }
    });

    // MOD
    register_command("MOD", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (std::holds_alternative<Integer>(a) && std::holds_alternative<Integer>(b)) {
            auto& va = std::get<Integer>(a);
            auto& vb = std::get<Integer>(b);
            if (vb == 0) {
                s.push(a);
                s.push(b);
                throw std::runtime_error("Division by zero");
            }
            s.push(Integer(va % vb));
        } else {
            s.push(a);
            s.push(b);
            throw std::runtime_error("Bad argument type");
        }
    });
}

// ---- Comparison Commands ----

void CommandRegistry::register_comparison_commands() {
    auto compare = [](Store& s, auto pred) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop(); // level 1
        Object a = s.pop(); // level 2

        int ra = numeric_rank(a);
        int rb = numeric_rank(b);
        if (ra < 0 || rb < 0) {
            s.push(a);
            s.push(b);
            throw std::runtime_error("Bad argument type");
        }

        int target = std::max(ra, rb);
        Object pa = promote(a, target);
        Object pb = promote(b, target);

        bool result = false;
        switch (target) {
            case 0: result = pred(std::get<Integer>(pa), std::get<Integer>(pb)); break;
            case 1: result = pred(std::get<Rational>(pa), std::get<Rational>(pb)); break;
            case 2: result = pred(std::get<Real>(pa), std::get<Real>(pb)); break;
            case 3: {
                // For complex, only == and != are meaningful
                auto& ca = std::get<Complex>(pa);
                auto& cb = std::get<Complex>(pb);
                result = pred(ca.first, cb.first); // simplified
                break;
            }
        }
        s.push(Integer(result ? 1 : 0));
    };

    register_command("==", [compare](Store& s, Context&) {
        compare(s, [](const auto& a, const auto& b) { return a == b; });
    });
    register_command("!=", [compare](Store& s, Context&) {
        compare(s, [](const auto& a, const auto& b) { return a != b; });
    });
    register_command("<", [compare](Store& s, Context&) {
        compare(s, [](const auto& a, const auto& b) { return a < b; });
    });
    register_command(">", [compare](Store& s, Context&) {
        compare(s, [](const auto& a, const auto& b) { return a > b; });
    });
    register_command("<=", [compare](Store& s, Context&) {
        compare(s, [](const auto& a, const auto& b) { return a <= b; });
    });
    register_command(">=", [compare](Store& s, Context&) {
        compare(s, [](const auto& a, const auto& b) { return a >= b; });
    });
}

// ---- Type Conversion Commands ----

void CommandRegistry::register_type_commands() {
    register_command("TYPE", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        s.push(Integer(static_cast<int>(type_tag(a))));
    });

    // →NUM (also accept ->NUM)
    auto to_num = [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (std::holds_alternative<Integer>(a)) {
            s.push(Real(std::get<Integer>(a)));
        } else if (std::holds_alternative<Rational>(a)) {
            s.push(Real(std::get<Rational>(a)));
        } else if (std::holds_alternative<Real>(a)) {
            s.push(a); // already real
        } else {
            s.push(a);
            throw std::runtime_error("Bad argument type");
        }
    };
    register_command("\xe2\x86\x92NUM", to_num); // →NUM (UTF-8)
    register_command("->NUM", to_num);            // ASCII fallback

    // →STR
    auto to_str = [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        s.push(String{repr(a)});
    };
    register_command("\xe2\x86\x92STR", to_str);
    register_command("->STR", to_str);

    // STR→
    auto str_eval = [](Store& s, Context& ctx) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<String>(a)) {
            s.push(a);
            throw std::runtime_error("Bad argument type");
        }
        auto& str = std::get<String>(a).value;
        ctx.execute_tokens(parse(str));
    };
    register_command("STR\xe2\x86\x92", str_eval);
    register_command("STR->", str_eval);
}

// ---- Filesystem Commands ----

void CommandRegistry::register_filesystem_commands() {
    register_command("STO", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object name_obj = s.pop(); // level 1
        Object value = s.pop();    // level 2
        if (!std::holds_alternative<Name>(name_obj)) {
            s.push(value);
            s.push(name_obj);
            throw std::runtime_error("Expected a name");
        }
        auto& name = std::get<Name>(name_obj).value;
        s.store_variable(s.current_dir(), name, value);
    });

    register_command("RCL", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object name_obj = s.pop();
        if (!std::holds_alternative<Name>(name_obj)) {
            s.push(name_obj);
            throw std::runtime_error("Expected a name");
        }
        auto& name = std::get<Name>(name_obj).value;
        Object val = s.recall_variable(s.current_dir(), name);
        if (std::holds_alternative<Error>(val)) {
            throw std::runtime_error("Undefined Name");
        }
        s.push(val);
    });

    register_command("PURGE", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object name_obj = s.pop();
        if (!std::holds_alternative<Name>(name_obj)) {
            s.push(name_obj);
            throw std::runtime_error("Expected a name");
        }
        auto& name = std::get<Name>(name_obj).value;
        s.purge_variable(s.current_dir(), name);
    });

    register_command("HOME", [](Store& s, Context&) {
        s.set_current_dir(s.home_dir_id());
    });

    register_command("PATH", [](Store& s, Context&) {
        // For bootstrap, just push "HOME" — no path traversal
        s.push(String{"HOME"});
    });

    register_command("CRDIR", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object name_obj = s.pop();
        if (!std::holds_alternative<Name>(name_obj)) {
            s.push(name_obj);
            throw std::runtime_error("Expected a name");
        }
        auto& name = std::get<Name>(name_obj).value;
        s.create_directory(s.current_dir(), name);
    });

    register_command("VARS", [](Store& s, Context&) {
        auto vars = s.list_variables(s.current_dir());
        std::string list = "{ ";
        for (size_t i = 0; i < vars.size(); ++i) {
            if (i > 0) list += " ";
            list += vars[i];
        }
        list += " }";
        s.push(String{list});
    });
}

// ---- Program Execution Commands ----

void CommandRegistry::register_program_commands() {
    register_command("EVAL", [](Store& s, Context& ctx) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (std::holds_alternative<Program>(a)) {
            ctx.execute_tokens(std::get<Program>(a).tokens);
        } else if (std::holds_alternative<Name>(a)) {
            auto& name = std::get<Name>(a).value;
            Object val = s.recall_variable(s.current_dir(), name);
            if (std::holds_alternative<Error>(val)) {
                throw std::runtime_error("Undefined Name");
            }
            if (std::holds_alternative<Program>(val)) {
                ctx.execute_tokens(std::get<Program>(val).tokens);
            } else {
                s.push(val);
            }
        } else if (std::holds_alternative<Symbol>(a)) {
            auto& expr = std::get<Symbol>(a).value;
            Object result = eval_expression(expr, ctx);
            s.push(result);
        } else {
            s.push(a); // non-program/name/symbol: just push back
        }
    });

    register_command("IFT", [](Store& s, Context& ctx) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object cond = s.pop();      // level 1
        Object then_prog = s.pop(); // level 2
        if (is_truthy(cond)) {
            if (std::holds_alternative<Program>(then_prog)) {
                ctx.execute_tokens(std::get<Program>(then_prog).tokens);
            } else {
                s.push(then_prog);
            }
        }
    });

    register_command("IFTE", [](Store& s, Context& ctx) {
        if (s.depth() < 3) throw std::runtime_error("Too few arguments");
        Object cond = s.pop();      // level 1
        Object then_prog = s.pop(); // level 2
        Object else_prog = s.pop(); // level 3
        Object& chosen = is_truthy(cond) ? then_prog : else_prog;
        if (std::holds_alternative<Program>(chosen)) {
            ctx.execute_tokens(std::get<Program>(chosen).tokens);
        } else {
            s.push(chosen);
        }
    });
}

// ---- Logic & Bitwise Commands ----

void CommandRegistry::register_logic_commands() {
    // Boolean logic (on integers: 0 = false, nonzero = true, result is 0 or 1)
    register_command("AND", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<Integer>(a) || !std::holds_alternative<Integer>(b))
            throw std::runtime_error("Bad argument type");
        bool ba = std::get<Integer>(a) != 0;
        bool bb = std::get<Integer>(b) != 0;
        s.push(Integer(ba && bb ? 1 : 0));
    });

    register_command("OR", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<Integer>(a) || !std::holds_alternative<Integer>(b))
            throw std::runtime_error("Bad argument type");
        bool ba = std::get<Integer>(a) != 0;
        bool bb = std::get<Integer>(b) != 0;
        s.push(Integer(ba || bb ? 1 : 0));
    });

    register_command("NOT", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<Integer>(a))
            throw std::runtime_error("Bad argument type");
        s.push(Integer(std::get<Integer>(a) == 0 ? 1 : 0));
    });

    register_command("XOR", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<Integer>(a) || !std::holds_alternative<Integer>(b))
            throw std::runtime_error("Bad argument type");
        bool ba = std::get<Integer>(a) != 0;
        bool bb = std::get<Integer>(b) != 0;
        s.push(Integer(ba != bb ? 1 : 0));
    });

    // Bitwise operations (on integers)
    register_command("BAND", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<Integer>(a) || !std::holds_alternative<Integer>(b))
            throw std::runtime_error("Bad argument type");
        s.push(Integer(std::get<Integer>(a) & std::get<Integer>(b)));
    });

    register_command("BOR", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<Integer>(a) || !std::holds_alternative<Integer>(b))
            throw std::runtime_error("Bad argument type");
        s.push(Integer(std::get<Integer>(a) | std::get<Integer>(b)));
    });

    register_command("BXOR", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<Integer>(a) || !std::holds_alternative<Integer>(b))
            throw std::runtime_error("Bad argument type");
        s.push(Integer(std::get<Integer>(a) ^ std::get<Integer>(b)));
    });

    register_command("BNOT", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<Integer>(a))
            throw std::runtime_error("Bad argument type");
        s.push(Integer(~std::get<Integer>(a)));
    });

    register_command("SL", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<Integer>(a) || !std::holds_alternative<Integer>(b))
            throw std::runtime_error("Bad argument type");
        auto shift = static_cast<int>(std::get<Integer>(b));
        s.push(Integer(std::get<Integer>(a) << shift));
    });

    register_command("SR", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<Integer>(a) || !std::holds_alternative<Integer>(b))
            throw std::runtime_error("Bad argument type");
        auto shift = static_cast<int>(std::get<Integer>(b));
        s.push(Integer(std::get<Integer>(a) >> shift));
    });

    register_command("ASR", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<Integer>(a) || !std::holds_alternative<Integer>(b))
            throw std::runtime_error("Bad argument type");
        // Arithmetic shift right is the same as >> for cpp_int (sign-extending)
        auto shift = static_cast<int>(std::get<Integer>(b));
        s.push(Integer(std::get<Integer>(a) >> shift));
    });

    // SAME: deep structural equality (same type AND same value)
    register_command("SAME", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        bool same = false;
        if (a.index() == b.index()) {
            // Same variant index — compare values
            std::visit([&same, &b](auto&& va) {
                using T = std::decay_t<decltype(va)>;
                auto& vb = std::get<T>(b);
                if constexpr (std::is_same_v<T, Integer> || std::is_same_v<T, Real> ||
                              std::is_same_v<T, Rational>) {
                    same = (va == vb);
                } else if constexpr (std::is_same_v<T, Complex>) {
                    same = (va.first == vb.first && va.second == vb.second);
                } else if constexpr (std::is_same_v<T, String>) {
                    same = (va.value == vb.value);
                } else if constexpr (std::is_same_v<T, Name>) {
                    same = (va.value == vb.value);
                } else if constexpr (std::is_same_v<T, Symbol>) {
                    same = (va.value == vb.value);
                } else if constexpr (std::is_same_v<T, Error>) {
                    same = (va.code == vb.code && va.message == vb.message);
                } else if constexpr (std::is_same_v<T, Program>) {
                    // Programs are same if their repr matches
                    same = (repr(Object(va)) == repr(Object(vb)));
                }
            }, a);
        }
        s.push(Integer(same ? 1 : 0));
    });
}

// ---- Transcendental & Scientific Commands ----

void CommandRegistry::register_transcendental_commands() {
    // Angle mode commands
    register_command("DEG", [](Store& s, Context&) {
        s.set_meta("angle_mode", "DEG");
    });
    register_command("RAD", [](Store& s, Context&) {
        s.set_meta("angle_mode", "RAD");
    });
    register_command("GRAD", [](Store& s, Context&) {
        s.set_meta("angle_mode", "GRAD");
    });

    // Helper lambdas for angle conversion
    // to_radians: convert from current angle mode to radians
    auto to_rad = [](const Real& val, Store& s) -> double {
        std::string mode = s.get_meta("angle_mode", "RAD");
        double v = val.convert_to<double>();
        if (mode == "DEG") return v * 3.14159265358979323846 / 180.0;
        if (mode == "GRAD") return v * 3.14159265358979323846 / 200.0;
        return v; // RAD
    };
    // from_radians: convert radians to current angle mode
    auto from_rad = [](double val, Store& s) -> Real {
        std::string mode = s.get_meta("angle_mode", "RAD");
        if (mode == "DEG") return Real(val * 180.0 / 3.14159265358979323846);
        if (mode == "GRAD") return Real(val * 200.0 / 3.14159265358979323846);
        return Real(val);
    };

    // Trig functions (angle-mode-aware)
    register_command("SIN", [to_rad](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        s.push(Real(std::sin(to_rad(to_real_value(a), s))));
    });

    register_command("COS", [to_rad](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        s.push(Real(std::cos(to_rad(to_real_value(a), s))));
    });

    register_command("TAN", [to_rad](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        s.push(Real(std::tan(to_rad(to_real_value(a), s))));
    });

    register_command("ASIN", [from_rad](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        s.push(from_rad(std::asin(to_double_value(a)), s));
    });

    register_command("ACOS", [from_rad](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        s.push(from_rad(std::acos(to_double_value(a)), s));
    });

    register_command("ATAN", [from_rad](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        s.push(from_rad(std::atan(to_double_value(a)), s));
    });

    register_command("ATAN2", [from_rad](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop(); // x
        Object a = s.pop(); // y
        s.push(from_rad(std::atan2(to_double_value(a), to_double_value(b)), s));
    });

    // Exponential / logarithmic
    register_command("EXP", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        s.push(Real(std::exp(to_double_value(a))));
    });

    register_command("LN", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        double v = to_double_value(a);
        if (v <= 0) throw std::runtime_error("Bad argument value");
        s.push(Real(std::log(v)));
    });

    register_command("LOG", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        double v = to_double_value(a);
        if (v <= 0) throw std::runtime_error("Bad argument value");
        s.push(Real(std::log10(v)));
    });

    register_command("ALOG", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        s.push(Real(std::pow(10.0, to_double_value(a))));
    });

    // SQRT, SQ
    register_command("SQRT", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (std::holds_alternative<Integer>(a)) {
            auto& v = std::get<Integer>(a);
            if (v < 0) throw std::runtime_error("Bad argument value");
            s.push(Real(std::sqrt(v.convert_to<double>())));
        } else if (std::holds_alternative<Real>(a)) {
            auto& v = std::get<Real>(a);
            if (v < 0) throw std::runtime_error("Bad argument value");
            s.push(Real(boost::multiprecision::sqrt(v)));
        } else if (std::holds_alternative<Rational>(a)) {
            auto& v = std::get<Rational>(a);
            if (v < 0) throw std::runtime_error("Bad argument value");
            s.push(Real(std::sqrt(v.convert_to<double>())));
        } else {
            throw std::runtime_error("Bad argument type");
        }
    });

    register_command("SQ", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        Object result = binary_numeric(a, a,
            [](const Integer& x, const Integer& y) -> Integer { return x * y; },
            [](const Rational& x, const Rational& y) -> Rational { return x * y; },
            [](const Real& x, const Real& y) -> Real { return x * y; },
            [](const Complex& x, const Complex& y) -> Complex {
                return {x.first * y.first - x.second * y.second,
                        x.first * y.second + x.second * y.first};
            });
        s.push(result);
    });

    // Constants
    register_command("PI", [](Store& s, Context&) {
        s.push(Real("3.14159265358979323846264338327950288419716939937510"));
    });

    register_command("E", [](Store& s, Context&) {
        s.push(Real("2.71828182845904523536028747135266249775724709369995"));
    });

    // Rounding
    register_command("FLOOR", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (std::holds_alternative<Integer>(a)) {
            s.push(a);
        } else if (std::holds_alternative<Real>(a)) {
            auto& v = std::get<Real>(a);
            s.push(Integer(static_cast<long long>(boost::multiprecision::floor(v))));
        } else if (std::holds_alternative<Rational>(a)) {
            Real r(std::get<Rational>(a));
            s.push(Integer(static_cast<long long>(boost::multiprecision::floor(r))));
        } else {
            throw std::runtime_error("Bad argument type");
        }
    });

    register_command("CEIL", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (std::holds_alternative<Integer>(a)) {
            s.push(a);
        } else if (std::holds_alternative<Real>(a)) {
            auto& v = std::get<Real>(a);
            s.push(Integer(static_cast<long long>(boost::multiprecision::ceil(v))));
        } else if (std::holds_alternative<Rational>(a)) {
            Real r(std::get<Rational>(a));
            s.push(Integer(static_cast<long long>(boost::multiprecision::ceil(r))));
        } else {
            throw std::runtime_error("Bad argument type");
        }
    });

    register_command("IP", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (std::holds_alternative<Integer>(a)) {
            s.push(a);
        } else if (std::holds_alternative<Real>(a)) {
            auto& v = std::get<Real>(a);
            s.push(Integer(static_cast<long long>(boost::multiprecision::trunc(v))));
        } else if (std::holds_alternative<Rational>(a)) {
            Real r(std::get<Rational>(a));
            s.push(Integer(static_cast<long long>(boost::multiprecision::trunc(r))));
        } else {
            throw std::runtime_error("Bad argument type");
        }
    });

    register_command("FP", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (std::holds_alternative<Integer>(a)) {
            s.push(Real(0));
        } else if (std::holds_alternative<Real>(a)) {
            auto& v = std::get<Real>(a);
            Real ip = boost::multiprecision::trunc(v);
            s.push(Real(v - ip));
        } else if (std::holds_alternative<Rational>(a)) {
            Real r(std::get<Rational>(a));
            Real ip = boost::multiprecision::trunc(r);
            s.push(Real(r - ip));
        } else {
            throw std::runtime_error("Bad argument type");
        }
    });

    // MIN, MAX, SIGN
    register_command("MIN", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        Object result = binary_numeric(a, b,
            [](const Integer& x, const Integer& y) -> Integer { return x < y ? x : y; },
            [](const Rational& x, const Rational& y) -> Rational { return x < y ? x : y; },
            [](const Real& x, const Real& y) -> Real { return x < y ? x : y; },
            [](const Complex&, const Complex&) -> Complex {
                throw std::runtime_error("Bad argument type");
            });
        s.push(result);
    });

    register_command("MAX", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        Object result = binary_numeric(a, b,
            [](const Integer& x, const Integer& y) -> Integer { return x > y ? x : y; },
            [](const Rational& x, const Rational& y) -> Rational { return x > y ? x : y; },
            [](const Real& x, const Real& y) -> Real { return x > y ? x : y; },
            [](const Complex&, const Complex&) -> Complex {
                throw std::runtime_error("Bad argument type");
            });
        s.push(result);
    });

    register_command("SIGN", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (std::holds_alternative<Integer>(a)) {
            auto& v = std::get<Integer>(a);
            s.push(Integer(v > 0 ? 1 : (v < 0 ? -1 : 0)));
        } else if (std::holds_alternative<Real>(a)) {
            auto& v = std::get<Real>(a);
            s.push(Integer(v > 0 ? 1 : (v < 0 ? -1 : 0)));
        } else if (std::holds_alternative<Rational>(a)) {
            auto& v = std::get<Rational>(a);
            s.push(Integer(v > 0 ? 1 : (v < 0 ? -1 : 0)));
        } else {
            throw std::runtime_error("Bad argument type");
        }
    });

    // Combinatorics
    // Factorial (!)
    register_command("!", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<Integer>(a))
            throw std::runtime_error("Bad argument type");
        auto n = std::get<Integer>(a);
        if (n < 0) throw std::runtime_error("Bad argument value");
        Integer result = 1;
        for (Integer i = 2; i <= n; ++i) result *= i;
        s.push(result);
    });

    // COMB(n, k) = n! / (k! * (n-k)!)
    register_command("COMB", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object k_obj = s.pop();
        Object n_obj = s.pop();
        if (!std::holds_alternative<Integer>(n_obj) || !std::holds_alternative<Integer>(k_obj))
            throw std::runtime_error("Bad argument type");
        auto n = std::get<Integer>(n_obj);
        auto k = std::get<Integer>(k_obj);
        if (n < 0 || k < 0 || k > n) throw std::runtime_error("Bad argument value");
        // Use iterative approach to avoid overflow
        if (k > n - k) k = n - k;
        Integer result = 1;
        for (Integer i = 0; i < k; ++i) {
            result = result * (n - i) / (i + 1);
        }
        s.push(result);
    });

    // PERM(n, k) = n! / (n-k)!
    register_command("PERM", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object k_obj = s.pop();
        Object n_obj = s.pop();
        if (!std::holds_alternative<Integer>(n_obj) || !std::holds_alternative<Integer>(k_obj))
            throw std::runtime_error("Bad argument type");
        auto n = std::get<Integer>(n_obj);
        auto k = std::get<Integer>(k_obj);
        if (n < 0 || k < 0 || k > n) throw std::runtime_error("Bad argument value");
        Integer result = 1;
        for (Integer i = 0; i < k; ++i) {
            result *= (n - i);
        }
        s.push(result);
    });

    // Percentage commands
    register_command("%", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        s.push(Real(to_double_value(a) * to_double_value(b) / 100.0));
    });

    register_command("%T", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        double total = to_double_value(a);
        if (total == 0) throw std::runtime_error("Division by zero");
        s.push(Real(to_double_value(b) / total * 100.0));
    });

    register_command("%CH", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        double old_val = to_double_value(a);
        if (old_val == 0) throw std::runtime_error("Division by zero");
        s.push(Real((to_double_value(b) - old_val) / old_val * 100.0));
    });

    // Angle conversion
    auto d2r_fn = [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        s.push(Real(to_double_value(a) * 3.14159265358979323846 / 180.0));
    };
    register_command("D->R", d2r_fn);
    register_command("D" "\xe2\x86\x92" "R", d2r_fn);

    auto r2d_fn = [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        s.push(Real(to_double_value(a) * 180.0 / 3.14159265358979323846));
    };
    register_command("R->D", r2d_fn);
    register_command("R" "\xe2\x86\x92" "D", r2d_fn);
}

// ---- String Manipulation Commands ----

void CommandRegistry::register_string_commands() {
    register_command("SIZE", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<String>(a))
            throw std::runtime_error("Bad argument type");
        s.push(Integer(static_cast<int>(std::get<String>(a).value.size())));
    });

    register_command("HEAD", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<String>(a))
            throw std::runtime_error("Bad argument type");
        auto& str = std::get<String>(a).value;
        if (str.empty()) throw std::runtime_error("Bad argument value");
        s.push(String{str.substr(0, 1)});
    });

    register_command("TAIL", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<String>(a))
            throw std::runtime_error("Bad argument type");
        auto& str = std::get<String>(a).value;
        if (str.empty()) throw std::runtime_error("Bad argument value");
        s.push(String{str.substr(1)});
    });

    // SUB: 1-based substring (string start end -- substring)
    register_command("SUB", [](Store& s, Context&) {
        if (s.depth() < 3) throw std::runtime_error("Too few arguments");
        Object end_obj = s.pop();
        Object start_obj = s.pop();
        Object str_obj = s.pop();
        if (!std::holds_alternative<String>(str_obj) ||
            !std::holds_alternative<Integer>(start_obj) ||
            !std::holds_alternative<Integer>(end_obj))
            throw std::runtime_error("Bad argument type");
        auto& str = std::get<String>(str_obj).value;
        int start = static_cast<int>(std::get<Integer>(start_obj));
        int end = static_cast<int>(std::get<Integer>(end_obj));
        if (start < 1) start = 1;
        if (end > static_cast<int>(str.size())) end = static_cast<int>(str.size());
        if (start > end) {
            s.push(String{""});
        } else {
            s.push(String{str.substr(start - 1, end - start + 1)});
        }
    });

    // POS: find substring (string search -- position), 0 if not found
    register_command("POS", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object search_obj = s.pop();
        Object str_obj = s.pop();
        if (!std::holds_alternative<String>(str_obj) ||
            !std::holds_alternative<String>(search_obj))
            throw std::runtime_error("Bad argument type");
        auto& str = std::get<String>(str_obj).value;
        auto& search = std::get<String>(search_obj).value;
        auto pos = str.find(search);
        s.push(Integer(pos == std::string::npos ? 0 : static_cast<int>(pos) + 1));
    });

    // REPL: replace first occurrence (string search replace -- result)
    register_command("REPL", [](Store& s, Context&) {
        if (s.depth() < 3) throw std::runtime_error("Too few arguments");
        Object repl_obj = s.pop();
        Object search_obj = s.pop();
        Object str_obj = s.pop();
        if (!std::holds_alternative<String>(str_obj) ||
            !std::holds_alternative<String>(search_obj) ||
            !std::holds_alternative<String>(repl_obj))
            throw std::runtime_error("Bad argument type");
        std::string result = std::get<String>(str_obj).value;
        auto& search = std::get<String>(search_obj).value;
        auto& repl = std::get<String>(repl_obj).value;
        auto pos = result.find(search);
        if (pos != std::string::npos) {
            result.replace(pos, search.size(), repl);
        }
        s.push(String{result});
    });

    // NUM: first char -> codepoint
    register_command("NUM", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<String>(a))
            throw std::runtime_error("Bad argument type");
        auto& str = std::get<String>(a).value;
        if (str.empty()) throw std::runtime_error("Bad argument value");
        s.push(Integer(static_cast<unsigned char>(str[0])));
    });

    // CHR: codepoint -> char
    register_command("CHR", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<Integer>(a))
            throw std::runtime_error("Bad argument type");
        int cp = static_cast<int>(std::get<Integer>(a));
        if (cp < 0 || cp > 127) throw std::runtime_error("Bad argument value");
        s.push(String{std::string(1, static_cast<char>(cp))});
    });
}

} // namespace lpr
