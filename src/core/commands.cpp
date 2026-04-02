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

bool is_right_assoc(const std::string& op) {
    return op == "^" || op == "NEG";
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

// Build a symbolic function call: Symbol{"FUNC(a, b, ...)"} for any arity
Object symbolic_func(const std::string& func, const std::vector<Object>& args) {
    std::string result = func + "(";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) result += ", ";
        result += to_expr_string(args[i]);
    }
    result += ")";
    return Symbol{result};
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
    register_symbolic_commands();
    register_list_commands();
    register_matrix_commands();
    register_display_commands();
    register_flag_commands();
    register_conversion_commands();
    register_cas_commands();
}

void CommandRegistry::register_command(const std::string& name, CommandFn fn) {
    commands_[name] = std::move(fn);
}

bool CommandRegistry::has(const std::string& name) const {
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return commands_.count(upper) > 0;
}

void CommandRegistry::execute(const std::string& name, Store& store, Context& ctx) const {
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(),
        [](unsigned char c) { return std::toupper(c); });
    auto it = commands_.find(upper);
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

// ---- Arithmetic helpers for compound types ----

namespace {

// Element-wise binary op on two lists of matching length
void list_elementwise(Store& s, Context& ctx, const List& a, const List& b, const std::string& op, CommandRegistry& cmds) {
    if (a.items.size() != b.items.size())
        throw std::runtime_error("Lists must have same length");
    List result;
    for (size_t i = 0; i < a.items.size(); ++i) {
        s.push(a.items[i]);
        s.push(b.items[i]);
        cmds.execute(op, s, ctx);
        result.items.push_back(s.pop());
    }
    s.push(std::move(result));
}

// Scalar broadcast: apply op(scalar, elem) or op(elem, scalar) for each element
void list_scalar_op(Store& s, Context& ctx, const List& lst, const Object& scalar, const std::string& op, bool scalar_first, CommandRegistry& cmds) {
    List result;
    for (auto& item : lst.items) {
        if (scalar_first) { s.push(scalar); s.push(item); }
        else              { s.push(item); s.push(scalar); }
        cmds.execute(op, s, ctx);
        result.items.push_back(s.pop());
    }
    s.push(std::move(result));
}

// Element-wise binary op on two matrices of matching dimensions
void matrix_elementwise(Store& s, Context& ctx, const Matrix& a, const Matrix& b, const std::string& op, CommandRegistry& cmds) {
    if (a.rows.size() != b.rows.size())
        throw std::runtime_error("Matrix dimensions must match");
    if (!a.rows.empty() && a.rows[0].size() != b.rows[0].size())
        throw std::runtime_error("Matrix dimensions must match");
    Matrix result;
    for (size_t r = 0; r < a.rows.size(); ++r) {
        std::vector<Object> row;
        for (size_t c = 0; c < a.rows[r].size(); ++c) {
            s.push(a.rows[r][c]);
            s.push(b.rows[r][c]);
            cmds.execute(op, s, ctx);
            row.push_back(s.pop());
        }
        result.rows.push_back(std::move(row));
    }
    s.push(std::move(result));
}

// Scalar * Matrix / Matrix * Scalar
void matrix_scalar_op(Store& s, Context& ctx, const Matrix& mat, const Object& scalar, const std::string& op, bool scalar_first, CommandRegistry& cmds) {
    Matrix result;
    for (auto& row : mat.rows) {
        std::vector<Object> rrow;
        for (auto& elem : row) {
            if (scalar_first) { s.push(scalar); s.push(elem); }
            else              { s.push(elem); s.push(scalar); }
            cmds.execute(op, s, ctx);
            rrow.push_back(s.pop());
        }
        result.rows.push_back(std::move(rrow));
    }
    s.push(std::move(result));
}

} // anonymous namespace

// ---- Arithmetic Commands ----

void CommandRegistry::register_arithmetic_commands() {
    // +
    register_command("+", [this](Store& s, Context& ctx) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        // List + List (element-wise)
        if (std::holds_alternative<List>(a) && std::holds_alternative<List>(b)) {
            list_elementwise(s, ctx, std::get<List>(a), std::get<List>(b), "+", *this);
            return;
        }
        // Scalar + List / List + Scalar
        if (std::holds_alternative<List>(a) && !std::holds_alternative<List>(b)) {
            list_scalar_op(s, ctx, std::get<List>(a), b, "+", false, *this);
            return;
        }
        if (!std::holds_alternative<List>(a) && std::holds_alternative<List>(b)) {
            list_scalar_op(s, ctx, std::get<List>(b), a, "+", true, *this);
            return;
        }
        // Matrix + Matrix (element-wise)
        if (std::holds_alternative<Matrix>(a) && std::holds_alternative<Matrix>(b)) {
            matrix_elementwise(s, ctx, std::get<Matrix>(a), std::get<Matrix>(b), "+", *this);
            return;
        }
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
    register_command("-", [this](Store& s, Context& ctx) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop(); // level 1
        Object a = s.pop(); // level 2
        // List - List (element-wise)
        if (std::holds_alternative<List>(a) && std::holds_alternative<List>(b)) {
            list_elementwise(s, ctx, std::get<List>(a), std::get<List>(b), "-", *this);
            return;
        }
        // Scalar - List / List - Scalar
        if (std::holds_alternative<List>(a) && !std::holds_alternative<List>(b)) {
            list_scalar_op(s, ctx, std::get<List>(a), b, "-", false, *this);
            return;
        }
        if (!std::holds_alternative<List>(a) && std::holds_alternative<List>(b)) {
            list_scalar_op(s, ctx, std::get<List>(b), a, "-", true, *this);
            return;
        }
        // Matrix - Matrix (element-wise)
        if (std::holds_alternative<Matrix>(a) && std::holds_alternative<Matrix>(b)) {
            matrix_elementwise(s, ctx, std::get<Matrix>(a), std::get<Matrix>(b), "-", *this);
            return;
        }
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
    register_command("*", [this](Store& s, Context& ctx) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        // List * List (element-wise)
        if (std::holds_alternative<List>(a) && std::holds_alternative<List>(b)) {
            list_elementwise(s, ctx, std::get<List>(a), std::get<List>(b), "*", *this);
            return;
        }
        // Scalar * List / List * Scalar
        if (std::holds_alternative<List>(a) && !std::holds_alternative<List>(b)) {
            list_scalar_op(s, ctx, std::get<List>(a), b, "*", false, *this);
            return;
        }
        if (!std::holds_alternative<List>(a) && std::holds_alternative<List>(b)) {
            list_scalar_op(s, ctx, std::get<List>(b), a, "*", true, *this);
            return;
        }
        // Matrix * Matrix (true matrix multiplication)
        if (std::holds_alternative<Matrix>(a) && std::holds_alternative<Matrix>(b)) {
            auto& ar = std::get<Matrix>(a).rows;
            auto& br = std::get<Matrix>(b).rows;
            // Check if b is a vector (1-row) and a is a matrix — treat as matrix*vector
            int a_rows = static_cast<int>(ar.size());
            int a_cols = ar.empty() ? 0 : static_cast<int>(ar[0].size());
            int b_rows = static_cast<int>(br.size());
            int b_cols = br.empty() ? 0 : static_cast<int>(br[0].size());
            // For matrix multiplication: a_cols must equal b_rows
            // For matrix*vector: b is 1-row, treat as column vector (b_cols elements)
            if (b_rows == 1 && a_cols == b_cols) {
                // Matrix * vector → vector
                Matrix result;
                std::vector<Object> rv;
                for (int r = 0; r < a_rows; ++r) {
                    Object sum = Integer(0);
                    for (int c = 0; c < a_cols; ++c) {
                        s.push(ar[r][c]); s.push(br[0][c]);
                        this->execute("*", s, ctx);
                        s.push(sum); s.push(s.pop());
                        this->execute("+", s, ctx);
                        sum = s.pop();
                    }
                    rv.push_back(std::move(sum));
                }
                result.rows.push_back(std::move(rv));
                s.push(std::move(result));
                return;
            }
            if (a_cols != b_rows)
                throw std::runtime_error("Matrix dimensions incompatible for multiplication");
            Matrix result;
            for (int r = 0; r < a_rows; ++r) {
                std::vector<Object> row;
                for (int c = 0; c < b_cols; ++c) {
                    Object sum = Integer(0);
                    for (int k = 0; k < a_cols; ++k) {
                        s.push(ar[r][k]); s.push(br[k][c]);
                        this->execute("*", s, ctx);
                        s.push(sum); s.push(s.pop());
                        this->execute("+", s, ctx);
                        sum = s.pop();
                    }
                    row.push_back(std::move(sum));
                }
                result.rows.push_back(std::move(row));
            }
            s.push(std::move(result));
            return;
        }
        // Scalar * Matrix / Matrix * Scalar
        if (std::holds_alternative<Matrix>(a) && !std::holds_alternative<Matrix>(b)) {
            matrix_scalar_op(s, ctx, std::get<Matrix>(a), b, "*", false, *this);
            return;
        }
        if (!std::holds_alternative<Matrix>(a) && std::holds_alternative<Matrix>(b)) {
            matrix_scalar_op(s, ctx, std::get<Matrix>(b), a, "*", true, *this);
            return;
        }
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
    register_command("/", [this](Store& s, Context& ctx) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop(); // divisor (level 1)
        Object a = s.pop(); // dividend (level 2)

        // List / List (element-wise)
        if (std::holds_alternative<List>(a) && std::holds_alternative<List>(b)) {
            list_elementwise(s, ctx, std::get<List>(a), std::get<List>(b), "/", *this);
            return;
        }
        // List / Scalar
        if (std::holds_alternative<List>(a) && !std::holds_alternative<List>(b)) {
            list_scalar_op(s, ctx, std::get<List>(a), b, "/", false, *this);
            return;
        }
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
    register_command("NEG", [this](Store& s, Context& ctx) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        // NEG on list: negate each element
        if (std::holds_alternative<List>(a)) {
            List result;
            for (auto& item : std::get<List>(a).items) {
                s.push(item);
                this->execute("NEG", s, ctx);
                result.items.push_back(s.pop());
            }
            s.push(std::move(result));
            return;
        }
        // NEG on matrix: negate each element
        if (std::holds_alternative<Matrix>(a)) {
            Matrix result;
            for (auto& row : std::get<Matrix>(a).rows) {
                std::vector<Object> rrow;
                for (auto& elem : row) {
                    s.push(elem);
                    this->execute("NEG", s, ctx);
                    rrow.push_back(s.pop());
                }
                result.rows.push_back(std::move(rrow));
            }
            s.push(std::move(result));
            return;
        }
        if (is_symbolic(a)) {
            std::string sa = to_expr_string(a);
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
        }, a.as_variant());
    });

    // INV (1/x or matrix inverse)
    register_command("INV", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        // Matrix inverse
        if (std::holds_alternative<Matrix>(a)) {
            auto& rows = std::get<Matrix>(a).rows;
            int n = static_cast<int>(rows.size());
            if (n == 0 || static_cast<int>(rows[0].size()) != n) {
                s.push(a);
                throw std::runtime_error("INV requires a square matrix");
            }
            for (auto& row : rows)
                for (auto& elem : row)
                    if (!is_numeric(elem)) {
                        s.push(a);
                        throw std::runtime_error("INV requires numeric matrix");
                    }
            // Gauss-Jordan elimination on [A | I]
            std::vector<std::vector<Real>> aug(n, std::vector<Real>(2 * n, Real(0)));
            for (int r = 0; r < n; ++r) {
                for (int c = 0; c < n; ++c)
                    aug[r][c] = to_real_value(rows[r][c]);
                aug[r][n + r] = Real(1);
            }
            for (int col = 0; col < n; ++col) {
                int pivot = -1;
                Real max_val = 0;
                for (int r = col; r < n; ++r) {
                    Real abs_val = aug[r][col] < 0 ? -aug[r][col] : aug[r][col];
                    if (abs_val > max_val) { max_val = abs_val; pivot = r; }
                }
                if (pivot < 0 || max_val == 0) {
                    s.push(a);
                    throw std::runtime_error("Singular matrix");
                }
                if (pivot != col) std::swap(aug[col], aug[pivot]);
                Real scale = aug[col][col];
                for (int c = 0; c < 2 * n; ++c) aug[col][c] /= scale;
                for (int r = 0; r < n; ++r) {
                    if (r == col) continue;
                    Real factor = aug[r][col];
                    for (int c = 0; c < 2 * n; ++c) aug[r][c] -= factor * aug[col][c];
                }
            }
            Matrix result;
            for (int r = 0; r < n; ++r) {
                std::vector<Object> row;
                for (int c = 0; c < n; ++c) row.push_back(aug[r][n + c]);
                result.rows.push_back(std::move(row));
            }
            s.push(std::move(result));
            return;
        }
        if (is_symbolic(a)) {
            s.push(symbolic_func("INV", {a}));
            return;
        }
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
    register_command("ABS", [this](Store& s, Context& ctx) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        // ABS on vector: Euclidean norm (numeric only)
        if (std::holds_alternative<Matrix>(a)) {
            auto& rows = std::get<Matrix>(a).rows;
            if (rows.size() != 1)
                throw std::runtime_error("ABS requires a vector (1-row matrix)");
            for (auto& elem : rows[0])
                if (!is_numeric(elem))
                    throw std::runtime_error("ABS on vector requires numeric elements");
            Real sum(0);
            for (auto& elem : rows[0]) {
                Real v = to_real_value(elem);
                sum += v * v;
            }
            s.push(boost::multiprecision::sqrt(sum));
            return;
        }
        if (is_symbolic(a)) {
            s.push(symbolic_func("ABS", {a}));
            return;
        }
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

    // ^ (power)
    register_command("^", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop(); // exponent (level 1)
        Object a = s.pop(); // base (level 2)

        if (is_symbolic(a) || is_symbolic(b)) {
            s.push(symbolic_binary(a, b, "^"));
            return;
        }

        // Helper: extract integer exponent from any exact numeric type
        auto get_int_exp = [](const Object& obj, bool& ok) -> long long {
            ok = false;
            if (std::holds_alternative<Integer>(obj)) {
                auto& v = std::get<Integer>(obj);
                if (v >= -1000000 && v <= 1000000) {
                    ok = true;
                    return v.convert_to<long long>();
                }
            } else if (std::holds_alternative<Rational>(obj)) {
                auto& v = std::get<Rational>(obj);
                if (boost::multiprecision::denominator(v) == 1) {
                    auto num = boost::multiprecision::numerator(v);
                    if (num >= -1000000 && num <= 1000000) {
                        ok = true;
                        return num.convert_to<long long>();
                    }
                }
            }
            return 0;
        };

        bool has_int_exp = false;
        long long iexp = get_int_exp(b, has_int_exp);

        if (std::holds_alternative<Integer>(a) && has_int_exp) {
            auto& base = std::get<Integer>(a);
            if (iexp >= 0) {
                s.push(Integer(boost::multiprecision::pow(base, static_cast<unsigned>(iexp))));
            } else {
                Integer denom(boost::multiprecision::pow(base, static_cast<unsigned>(-iexp)));
                if (denom == 0) throw std::runtime_error("Division by zero");
                s.push(Rational(Integer(1), denom));
            }
        } else if (std::holds_alternative<Rational>(a) && has_int_exp) {
            auto& base = std::get<Rational>(a);
            Integer num = boost::multiprecision::numerator(base);
            Integer den = boost::multiprecision::denominator(base);
            if (iexp >= 0) {
                auto n = static_cast<unsigned>(iexp);
                s.push(Rational(Integer(boost::multiprecision::pow(num, n)),
                                Integer(boost::multiprecision::pow(den, n))));
            } else {
                auto n = static_cast<unsigned>(-iexp);
                Integer new_num(boost::multiprecision::pow(den, n));
                Integer new_den(boost::multiprecision::pow(num, n));
                if (new_den == 0) throw std::runtime_error("Division by zero");
                s.push(Rational(new_num, new_den));
            }
        } else {
            // Promote to Real
            Real rbase, rexp;
            if (std::holds_alternative<Integer>(a))
                rbase = Real(std::get<Integer>(a));
            else if (std::holds_alternative<Rational>(a))
                rbase = Real(std::get<Rational>(a));
            else if (std::holds_alternative<Real>(a))
                rbase = std::get<Real>(a);
            else { s.push(a); s.push(b); throw std::runtime_error("Bad argument type"); }

            if (std::holds_alternative<Integer>(b))
                rexp = Real(std::get<Integer>(b));
            else if (std::holds_alternative<Rational>(b))
                rexp = Real(std::get<Rational>(b));
            else if (std::holds_alternative<Real>(b))
                rexp = std::get<Real>(b);
            else { s.push(a); s.push(b); throw std::runtime_error("Bad argument type"); }

            if (rbase < 0) throw std::runtime_error("Bad argument value");
            s.push(Real(boost::multiprecision::pow(rbase, rexp)));
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
        s.push(String{s.dir_path(s.current_dir())});
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

    register_command("CD", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object name_obj = s.pop();
        if (!std::holds_alternative<Name>(name_obj)) {
            s.push(name_obj);
            throw std::runtime_error("Expected a name");
        }
        auto& name = std::get<Name>(name_obj).value;
        int dir_id = s.find_directory(s.current_dir(), name);
        if (dir_id < 0) {
            s.push(name_obj);
            throw std::runtime_error("Directory not found");
        }
        s.set_current_dir(dir_id);
    });

    register_command("UPDIR", [](Store& s, Context&) {
        int parent = s.parent_dir_id(s.current_dir());
        if (parent >= 0) {
            s.set_current_dir(parent);
        }
    });

    register_command("PGDIR", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object name_obj = s.pop();
        if (!std::holds_alternative<Name>(name_obj)) {
            s.push(name_obj);
            throw std::runtime_error("Expected a name");
        }
        auto& name = std::get<Name>(name_obj).value;
        int dir_id = s.find_directory(s.current_dir(), name);
        if (dir_id < 0) {
            s.push(name_obj);
            throw std::runtime_error("Directory not found");
        }
        s.purge_directory_tree(dir_id);
    });

    register_command("VARS", [](Store& s, Context&) {
        List result;
        auto vars = s.list_variables(s.current_dir());
        for (auto& v : vars) {
            result.items.push_back(Name{v});
        }
        auto dirs = s.list_subdirectories(s.current_dir());
        for (auto& d : dirs) {
            result.items.push_back(Name{d + "/"});
        }
        s.push(std::move(result));
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
                s.push(Name{name});
                return;
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
        if (is_symbolic(cond) || is_symbolic(then_prog)) {
            s.push(symbolic_func("IFT", {then_prog, cond}));
            return;
        }
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
        if (is_symbolic(cond) || is_symbolic(then_prog) || is_symbolic(else_prog)) {
            s.push(symbolic_func("IFTE", {else_prog, then_prog, cond}));
            return;
        }
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
            }, a.as_variant());
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
        if (is_symbolic(a)) { s.push(symbolic_func("SIN", {a})); return; }
        s.push(Real(std::sin(to_rad(to_real_value(a), s))));
    });

    register_command("COS", [to_rad](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (is_symbolic(a)) { s.push(symbolic_func("COS", {a})); return; }
        s.push(Real(std::cos(to_rad(to_real_value(a), s))));
    });

    register_command("TAN", [to_rad](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (is_symbolic(a)) { s.push(symbolic_func("TAN", {a})); return; }
        s.push(Real(std::tan(to_rad(to_real_value(a), s))));
    });

    register_command("ASIN", [from_rad](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (is_symbolic(a)) { s.push(symbolic_func("ASIN", {a})); return; }
        s.push(from_rad(std::asin(to_double_value(a)), s));
    });

    register_command("ACOS", [from_rad](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (is_symbolic(a)) { s.push(symbolic_func("ACOS", {a})); return; }
        s.push(from_rad(std::acos(to_double_value(a)), s));
    });

    register_command("ATAN", [from_rad](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (is_symbolic(a)) { s.push(symbolic_func("ATAN", {a})); return; }
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
        if (is_symbolic(a)) { s.push(symbolic_func("EXP", {a})); return; }
        s.push(Real(std::exp(to_double_value(a))));
    });

    register_command("LN", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (is_symbolic(a)) { s.push(symbolic_func("LN", {a})); return; }
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
        if (is_symbolic(a)) { s.push(symbolic_func("SQRT", {a})); return; }
        if (std::holds_alternative<Integer>(a)) {
            auto& v = std::get<Integer>(a);
            if (v < 0) throw std::runtime_error("Bad argument value");
            Integer isqrt = boost::multiprecision::sqrt(v);
            if (isqrt * isqrt == v) {
                s.push(isqrt);
            } else {
                s.push(Real(boost::multiprecision::sqrt(Real(v))));
            }
        } else if (std::holds_alternative<Real>(a)) {
            auto& v = std::get<Real>(a);
            if (v < 0) throw std::runtime_error("Bad argument value");
            s.push(Real(boost::multiprecision::sqrt(v)));
        } else if (std::holds_alternative<Rational>(a)) {
            auto& v = std::get<Rational>(a);
            if (v < 0) throw std::runtime_error("Bad argument value");
            Integer num = boost::multiprecision::numerator(v);
            Integer den = boost::multiprecision::denominator(v);
            Integer isqrt_num = boost::multiprecision::sqrt(num);
            Integer isqrt_den = boost::multiprecision::sqrt(den);
            if (isqrt_num * isqrt_num == num && isqrt_den * isqrt_den == den) {
                s.push(Rational(isqrt_num, isqrt_den));
            } else {
                s.push(Real(boost::multiprecision::sqrt(Real(v))));
            }
        } else {
            throw std::runtime_error("Bad argument type");
        }
    });

    register_command("SQ", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (is_symbolic(a)) { s.push(symbolic_binary(a, Integer(2), "^")); return; }
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

// ---- Symbolic Manipulation Commands ----

void CommandRegistry::register_symbolic_commands() {
    // SUBST: ( 'expr' 'var' 'replacement' -- 'result' )
    // Tokenize expr, replace matching Name tokens with replacement, reconstruct string.
    register_command("SUBST", [](Store& s, Context&) {
        if (s.depth() < 3) throw std::runtime_error("Too few arguments");
        Object repl_obj = s.pop();  // level 1: replacement
        Object var_obj = s.pop();   // level 2: variable name
        Object expr_obj = s.pop();  // level 3: expression

        if (!std::holds_alternative<Symbol>(expr_obj) && !std::holds_alternative<Name>(expr_obj))
            throw std::runtime_error("Bad argument type");
        if (!std::holds_alternative<Name>(var_obj))
            throw std::runtime_error("Bad argument type");

        std::string expr_str = to_expr_string(expr_obj);
        std::string var_name = std::get<Name>(var_obj).value;
        std::string repl_str = to_expr_string(repl_obj);

        auto tokens = tokenize_expression(expr_str);

        // Reconstruct, replacing matching Name tokens
        std::string result;
        for (size_t i = 0; i < tokens.size(); ++i) {
            auto& tok = tokens[i];
            if (tok.type == ExprTokenType::Name && tok.value == var_name) {
                // Check if replacement needs parentheses:
                // Determine the context precedence by looking at surrounding operators
                int ctx_prec = 0;
                // Look backwards for an operator
                for (int j = static_cast<int>(i) - 1; j >= 0; --j) {
                    if (tokens[j].type == ExprTokenType::Op) {
                        ctx_prec = precedence(tokens[j].value);
                        break;
                    }
                    if (tokens[j].type != ExprTokenType::RParen) break;
                }
                // Look forward for an operator
                for (size_t j = i + 1; j < tokens.size(); ++j) {
                    if (tokens[j].type == ExprTokenType::Op) {
                        int fwd_prec = precedence(tokens[j].value);
                        if (fwd_prec > ctx_prec) ctx_prec = fwd_prec;
                        break;
                    }
                    if (tokens[j].type != ExprTokenType::LParen) break;
                }

                if (needs_parens(repl_str, ctx_prec)) {
                    result += "(" + repl_str + ")";
                } else {
                    result += repl_str;
                }
            } else if (tok.type == ExprTokenType::Comma) {
                result += ", ";
            } else {
                result += tok.value;
            }
        }

        s.push(Symbol{result});
    });

    // STASH: pop 1 item from stack, store as single-item group on stash
    register_command("STASH", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object item = s.pop();
        s.stash_push({item});
    });

    // STASHN: pop count from level 1, pop N items, store as group on stash
    register_command("STASHN", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object n_obj = s.pop();
        if (!std::holds_alternative<Integer>(n_obj))
            throw std::runtime_error("Bad argument type");
        int n = static_cast<int>(std::get<Integer>(n_obj));
        if (n < 0 || s.depth() < n) throw std::runtime_error("Too few arguments");

        // Pop N items (level 1 first = last in group)
        std::vector<Object> group(n);
        for (int i = n - 1; i >= 0; --i) {
            group[i] = s.pop();
        }
        s.stash_push(group);
    });

    // UNSTASH: pop most recent group from stash, push items back to stack in original order
    register_command("UNSTASH", [](Store& s, Context&) {
        auto group = s.stash_pop();
        for (auto& item : group) {
            s.push(item);
        }
    });

    // EXPLODE: decompose a Symbol's top-level operation into operands + operator
    // ( 'expr' -- operand1 [operand2 ...] « operator » )
    register_command("EXPLODE", [](Store& s, Context& ctx) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object obj = s.pop();
        if (!std::holds_alternative<Symbol>(obj))
            throw std::runtime_error("Bad argument type");
        std::string expr = std::get<Symbol>(obj).value;
        auto tokens = tokenize_expression(expr);

        if (tokens.empty()) throw std::runtime_error("Cannot EXPLODE empty expression");

        // Check for function-call pattern: NAME(args)
        if (tokens.size() >= 3 &&
            tokens[0].type == ExprTokenType::Name &&
            tokens[1].type == ExprTokenType::LParen &&
            tokens.back().type == ExprTokenType::RParen) {
            std::string func_name = tokens[0].value;
            // Split arguments by commas at paren depth 1 (the outer parens of the call)
            std::vector<std::string> args;
            std::string current_arg;
            int depth = 0;
            for (size_t i = 2; i < tokens.size() - 1; ++i) {
                auto& tok = tokens[i];
                if (tok.type == ExprTokenType::LParen) ++depth;
                if (tok.type == ExprTokenType::RParen) --depth;
                if (tok.type == ExprTokenType::Comma && depth == 0) {
                    // Trim whitespace from arg
                    args.push_back(current_arg);
                    current_arg.clear();
                } else {
                    current_arg += tok.value;
                }
            }
            if (!current_arg.empty()) {
                args.push_back(current_arg);
            }

            // Push each argument as the appropriate type
            for (auto& arg : args) {
                // Try to parse as integer
                bool is_number = true;
                bool has_dot = false;
                for (size_t ci = 0; ci < arg.size(); ++ci) {
                    char c = arg[ci];
                    if (c == '-' && ci == 0) continue;
                    if (c == '.' && !has_dot) { has_dot = true; continue; }
                    if (!std::isdigit(static_cast<unsigned char>(c))) { is_number = false; break; }
                }
                if (is_number && !arg.empty() && arg != "-") {
                    s.push(Symbol{arg});
                } else {
                    // Check if it's a simple name (no operators)
                    auto arg_tokens = tokenize_expression(arg);
                    if (arg_tokens.size() == 1 && arg_tokens[0].type == ExprTokenType::Name) {
                        s.push(Name{arg_tokens[0].value});
                    } else {
                        s.push(Symbol{arg});
                    }
                }
            }

            // Push function as a Program
            s.push(Program{{Token::make_command(func_name)}});
            return;
        }

        // Find the lowest-precedence operator at paren depth 0
        int min_prec = 100;
        int min_idx = -1;
        int paren_depth = 0;

        for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
            auto& tok = tokens[i];
            if (tok.type == ExprTokenType::LParen) { ++paren_depth; continue; }
            if (tok.type == ExprTokenType::RParen) { --paren_depth; continue; }
            if (paren_depth > 0) continue;

            if (tok.type == ExprTokenType::Op && tok.value != "NEG") {
                int p = precedence(tok.value);
                // For left-to-right (non-right-assoc), take the LAST (rightmost) occurrence
                // at the minimum precedence level so we peel left-to-right.
                // For right-to-left, take the first occurrence.
                if (p < min_prec) {
                    min_prec = p;
                    min_idx = i;
                } else if (p == min_prec && !is_right_assoc(tok.value)) {
                    min_idx = i;  // rightmost for left-associative
                }
            }
        }

        // Handle unary NEG at depth 0
        if (min_idx < 0) {
            // Check for unary NEG
            if (tokens.size() >= 2 && tokens[0].type == ExprTokenType::Op && tokens[0].value == "NEG") {
                // Unary negation: push operand and NEG program
                std::string operand_str;
                for (size_t i = 1; i < tokens.size(); ++i) {
                    operand_str += tokens[i].value;
                }
                auto operand_tokens = tokenize_expression(operand_str);
                if (operand_tokens.size() == 1 && operand_tokens[0].type == ExprTokenType::Name) {
                    s.push(Name{operand_tokens[0].value});
                } else {
                    s.push(Symbol{operand_str});
                }
                s.push(Program{{Token::make_command("NEG")}});
                return;
            }

            // Atomic expression or fully-parenthesized — strip outer parens and re-explode
            if (tokens.size() >= 2 &&
                tokens[0].type == ExprTokenType::LParen &&
                tokens.back().type == ExprTokenType::RParen) {
                std::string inner;
                for (size_t i = 1; i + 1 < tokens.size(); ++i) {
                    inner += tokens[i].value;
                }
                s.push(Symbol{inner});
                ctx.execute_tokens({Token::make_command("EXPLODE")});
                return;
            }

            throw std::runtime_error("Cannot EXPLODE atomic expression");
        }

        // Binary operator found — extract left and right operands
        std::string left_str, right_str;
        for (int i = 0; i < min_idx; ++i) left_str += tokens[i].value;
        for (int i = min_idx + 1; i < static_cast<int>(tokens.size()); ++i) right_str += tokens[i].value;
        std::string op = tokens[min_idx].value;

        // Push left operand
        auto push_operand = [&s](const std::string& str) {
            auto toks = tokenize_expression(str);
            if (toks.size() == 1 && toks[0].type == ExprTokenType::Name) {
                s.push(Name{toks[0].value});
            } else {
                s.push(Symbol{str});
            }
        };

        push_operand(left_str);
        push_operand(right_str);

        // Push operator as a Program
        s.push(Program{{Token::make_command(op)}});
    });

    // ASSEMBLE: loop while stash non-empty — UNSTASH then EVAL level 1
    register_command("ASSEMBLE", [](Store& s, Context& ctx) {
        // Implicit stash: scoop remaining stack items into a new stash group
        // so that EXPLODE ASSEMBLE works without an explicit STASH in between.
        if (s.depth() > 0) {
            int n = s.depth();
            std::vector<Object> group(n);
            for (int i = n - 1; i >= 0; --i) {
                group[i] = s.pop();
            }
            s.stash_push(group);
        }

        while (s.stash_depth() > 0) {
            auto group = s.stash_pop();
            for (auto& item : group) {
                s.push(item);
            }
            // EVAL the top of stack (should be a Program from EXPLODE)
            Object top = s.pop();
            if (std::holds_alternative<Program>(top)) {
                ctx.execute_tokens(std::get<Program>(top).tokens);
            } else {
                s.push(top);
            }
        }
    });
}

// ---- List Commands ----

void CommandRegistry::register_list_commands() {
    // LIST-> : explode list onto stack with count
    auto list_to = [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<List>(a))
            throw std::runtime_error("Bad argument type");
        auto& items = std::get<List>(a).items;
        for (auto& item : items) s.push(item);
        s.push(Integer(static_cast<int>(items.size())));
    };
    register_command("LIST\xe2\x86\x92", list_to);
    register_command("LIST->", list_to);

    // ->LIST : collect N items from stack into list
    auto to_list = [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object n_obj = s.pop();
        if (!std::holds_alternative<Integer>(n_obj))
            throw std::runtime_error("Bad argument type");
        int n = static_cast<int>(std::get<Integer>(n_obj));
        if (n < 0 || s.depth() < n) throw std::runtime_error("Too few arguments");
        List list;
        list.items.resize(n);
        for (int i = n - 1; i >= 0; --i) list.items[i] = s.pop();
        s.push(std::move(list));
    };
    register_command("\xe2\x86\x92LIST", to_list);
    register_command("->LIST", to_list);

    // GET : 1-based element access
    register_command("GET", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object idx_obj = s.pop();
        Object obj = s.pop();

        if (std::holds_alternative<List>(obj)) {
            if (!std::holds_alternative<Integer>(idx_obj))
                throw std::runtime_error("Bad argument type");
            int idx = static_cast<int>(std::get<Integer>(idx_obj));
            auto& items = std::get<List>(obj).items;
            if (idx < 1 || idx > static_cast<int>(items.size())) {
                s.push(obj); s.push(idx_obj);
                throw std::runtime_error("Index out of range");
            }
            s.push(items[idx - 1]);
        } else if (std::holds_alternative<Matrix>(obj)) {
            // Matrix GET expects { row col } list as index
            if (!std::holds_alternative<List>(idx_obj))
                throw std::runtime_error("Bad argument type");
            auto& idx_list = std::get<List>(idx_obj).items;
            if (idx_list.size() != 2 ||
                !std::holds_alternative<Integer>(idx_list[0]) ||
                !std::holds_alternative<Integer>(idx_list[1]))
                throw std::runtime_error("Bad argument type");
            int r = static_cast<int>(std::get<Integer>(idx_list[0]));
            int c = static_cast<int>(std::get<Integer>(idx_list[1]));
            auto& rows = std::get<Matrix>(obj).rows;
            if (r < 1 || r > static_cast<int>(rows.size()) ||
                c < 1 || c > static_cast<int>(rows[0].size())) {
                s.push(obj); s.push(idx_obj);
                throw std::runtime_error("Index out of range");
            }
            s.push(rows[r - 1][c - 1]);
        } else {
            s.push(obj); s.push(idx_obj);
            throw std::runtime_error("Bad argument type");
        }
    });

    // PUT : 1-based element replacement
    register_command("PUT", [](Store& s, Context&) {
        if (s.depth() < 3) throw std::runtime_error("Too few arguments");
        Object val = s.pop();
        Object idx_obj = s.pop();
        Object obj = s.pop();

        if (std::holds_alternative<List>(obj)) {
            if (!std::holds_alternative<Integer>(idx_obj))
                throw std::runtime_error("Bad argument type");
            int idx = static_cast<int>(std::get<Integer>(idx_obj));
            auto list = std::get<List>(obj);
            if (idx < 1 || idx > static_cast<int>(list.items.size())) {
                s.push(obj); s.push(idx_obj); s.push(val);
                throw std::runtime_error("Index out of range");
            }
            list.items[idx - 1] = val;
            s.push(std::move(list));
        } else if (std::holds_alternative<Matrix>(obj)) {
            if (!std::holds_alternative<List>(idx_obj))
                throw std::runtime_error("Bad argument type");
            auto& idx_list = std::get<List>(idx_obj).items;
            if (idx_list.size() != 2 ||
                !std::holds_alternative<Integer>(idx_list[0]) ||
                !std::holds_alternative<Integer>(idx_list[1]))
                throw std::runtime_error("Bad argument type");
            // Validate element type for matrix
            if (!is_numeric(val) && !is_symbolic(val)) {
                s.push(obj); s.push(idx_obj); s.push(val);
                throw std::runtime_error("Invalid matrix element type");
            }
            int r = static_cast<int>(std::get<Integer>(idx_list[0]));
            int c = static_cast<int>(std::get<Integer>(idx_list[1]));
            auto mat = std::get<Matrix>(obj);
            if (r < 1 || r > static_cast<int>(mat.rows.size()) ||
                c < 1 || c > static_cast<int>(mat.rows[0].size())) {
                s.push(obj); s.push(idx_obj); s.push(val);
                throw std::runtime_error("Index out of range");
            }
            mat.rows[r - 1][c - 1] = val;
            s.push(std::move(mat));
        } else {
            s.push(obj); s.push(idx_obj); s.push(val);
            throw std::runtime_error("Bad argument type");
        }
    });

    // GETI : GET with auto-incremented index
    register_command("GETI", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object idx_obj = s.pop();
        Object obj = s.pop();

        if (std::holds_alternative<List>(obj)) {
            if (!std::holds_alternative<Integer>(idx_obj))
                throw std::runtime_error("Bad argument type");
            int idx = static_cast<int>(std::get<Integer>(idx_obj));
            auto& items = std::get<List>(obj).items;
            if (idx < 1 || idx > static_cast<int>(items.size())) {
                s.push(obj); s.push(idx_obj);
                throw std::runtime_error("Index out of range");
            }
            s.push(obj);
            int next_idx = (idx < static_cast<int>(items.size())) ? idx + 1 : 1;
            s.push(Integer(next_idx));
            s.push(items[idx - 1]);
        } else {
            s.push(obj); s.push(idx_obj);
            throw std::runtime_error("Bad argument type");
        }
    });

    // PUTI : PUT with auto-incremented index
    register_command("PUTI", [](Store& s, Context&) {
        if (s.depth() < 3) throw std::runtime_error("Too few arguments");
        Object val = s.pop();
        Object idx_obj = s.pop();
        Object obj = s.pop();

        if (std::holds_alternative<List>(obj)) {
            if (!std::holds_alternative<Integer>(idx_obj))
                throw std::runtime_error("Bad argument type");
            int idx = static_cast<int>(std::get<Integer>(idx_obj));
            auto list = std::get<List>(obj);
            if (idx < 1 || idx > static_cast<int>(list.items.size())) {
                s.push(obj); s.push(idx_obj); s.push(val);
                throw std::runtime_error("Index out of range");
            }
            list.items[idx - 1] = val;
            int next_idx = (idx < static_cast<int>(list.items.size())) ? idx + 1 : 1;
            s.push(std::move(list));
            s.push(Integer(next_idx));
        } else {
            s.push(obj); s.push(idx_obj); s.push(val);
            throw std::runtime_error("Bad argument type");
        }
    });

    // HEAD : first element (list) or first char (string)
    register_command("HEAD", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (std::holds_alternative<List>(a)) {
            auto& items = std::get<List>(a).items;
            if (items.empty()) throw std::runtime_error("Empty list");
            s.push(items[0]);
        } else if (std::holds_alternative<String>(a)) {
            auto& str = std::get<String>(a).value;
            if (str.empty()) throw std::runtime_error("Bad argument value");
            s.push(String{str.substr(0, 1)});
        } else {
            s.push(a);
            throw std::runtime_error("Bad argument type");
        }
    });

    // TAIL : all but first element (list) or all but first char (string)
    register_command("TAIL", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (std::holds_alternative<List>(a)) {
            auto& items = std::get<List>(a).items;
            if (items.empty()) throw std::runtime_error("Empty list");
            List result;
            result.items.assign(items.begin() + 1, items.end());
            s.push(std::move(result));
        } else if (std::holds_alternative<String>(a)) {
            auto& str = std::get<String>(a).value;
            if (str.empty()) throw std::runtime_error("Bad argument value");
            s.push(String{str.substr(1)});
        } else {
            s.push(a);
            throw std::runtime_error("Bad argument type");
        }
    });

    // SIZE : element count (list), {rows, cols} (matrix), or char count (string)
    register_command("SIZE", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (std::holds_alternative<List>(a)) {
            s.push(Integer(static_cast<int>(std::get<List>(a).items.size())));
        } else if (std::holds_alternative<Matrix>(a)) {
            auto& rows = std::get<Matrix>(a).rows;
            List dims;
            dims.items.push_back(Integer(static_cast<int>(rows.size())));
            dims.items.push_back(Integer(rows.empty() ? 0 : static_cast<int>(rows[0].size())));
            s.push(std::move(dims));
        } else if (std::holds_alternative<String>(a)) {
            s.push(Integer(static_cast<int>(std::get<String>(a).value.size())));
        } else {
            s.push(a);
            throw std::runtime_error("Bad argument type");
        }
    });

    // POS : find element in list (or substring in string), return 1-based index or 0
    register_command("POS", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object needle = s.pop();
        Object a = s.pop();
        if (std::holds_alternative<List>(a)) {
            auto& items = std::get<List>(a).items;
            for (size_t i = 0; i < items.size(); ++i) {
                if (repr(items[i]) == repr(needle)) {
                    s.push(Integer(static_cast<int>(i + 1)));
                    return;
                }
            }
            s.push(Integer(0));
        } else if (std::holds_alternative<String>(a) && std::holds_alternative<String>(needle)) {
            auto& str = std::get<String>(a).value;
            auto& search = std::get<String>(needle).value;
            auto pos = str.find(search);
            s.push(Integer(pos == std::string::npos ? 0 : static_cast<int>(pos) + 1));
        } else {
            s.push(a); s.push(needle);
            throw std::runtime_error("Bad argument type");
        }
    });

    // SUB : sub-list (or substring) by 1-based start/end indices
    register_command("SUB", [](Store& s, Context&) {
        if (s.depth() < 3) throw std::runtime_error("Too few arguments");
        Object end_obj = s.pop();
        Object start_obj = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<Integer>(start_obj) ||
            !std::holds_alternative<Integer>(end_obj)) {
            s.push(a); s.push(start_obj); s.push(end_obj);
            throw std::runtime_error("Bad argument type");
        }
        int start = static_cast<int>(std::get<Integer>(start_obj));
        int end = static_cast<int>(std::get<Integer>(end_obj));
        if (std::holds_alternative<List>(a)) {
            auto& items = std::get<List>(a).items;
            int sz = static_cast<int>(items.size());
            if (start < 1) start = 1;
            if (end > sz) end = sz;
            List result;
            if (start <= end) {
                result.items.assign(items.begin() + start - 1, items.begin() + end);
            }
            s.push(std::move(result));
        } else if (std::holds_alternative<String>(a)) {
            auto& str = std::get<String>(a).value;
            if (start < 1) start = 1;
            if (end > static_cast<int>(str.size())) end = static_cast<int>(str.size());
            if (start > end) {
                s.push(String{""});
            } else {
                s.push(String{str.substr(start - 1, end - start + 1)});
            }
        } else {
            s.push(a); s.push(start_obj); s.push(end_obj);
            throw std::runtime_error("Bad argument type");
        }
    });

    // REVLIST : reverse list
    register_command("REVLIST", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<List>(a))
            throw std::runtime_error("Bad argument type");
        auto list = std::get<List>(a);
        std::reverse(list.items.begin(), list.items.end());
        s.push(std::move(list));
    });

    // SORT : sort list of homogeneous numeric/string elements
    register_command("SORT", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<List>(a))
            throw std::runtime_error("Bad argument type");
        auto list = std::get<List>(a);
        if (list.items.empty()) { s.push(std::move(list)); return; }
        // Check if all items are the same numeric or string type
        bool all_string = true;
        bool all_numeric = true;
        for (auto& item : list.items) {
            if (!std::holds_alternative<String>(item)) all_string = false;
            if (!is_numeric(item)) all_numeric = false;
        }
        if (!all_string && !all_numeric)
            throw std::runtime_error("SORT requires homogeneous numeric or string list");
        if (all_string) {
            std::sort(list.items.begin(), list.items.end(),
                [](const Object& x, const Object& y) {
                    return std::get<String>(x).value < std::get<String>(y).value;
                });
        } else {
            std::sort(list.items.begin(), list.items.end(),
                [](const Object& x, const Object& y) {
                    return to_real_value(x) < to_real_value(y);
                });
        }
        s.push(std::move(list));
    });

    // ADD : append element to list
    register_command("ADD", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object elem = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<List>(a))
            throw std::runtime_error("Bad argument type");
        auto list = std::get<List>(a);
        list.items.push_back(std::move(elem));
        s.push(std::move(list));
    });

    // --- Higher-order ---

    // DOLIST : apply program to corresponding elements of N lists
    register_command("DOLIST", [](Store& s, Context& ctx) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object prog_obj = s.pop();
        if (!std::holds_alternative<Program>(prog_obj))
            throw std::runtime_error("Bad argument type");
        Object n_obj = s.pop();
        if (!std::holds_alternative<Integer>(n_obj))
            throw std::runtime_error("Bad argument type");
        int n = static_cast<int>(std::get<Integer>(n_obj));
        if (n < 1 || s.depth() < n) throw std::runtime_error("Too few arguments");

        std::vector<List> lists(n);
        for (int i = n - 1; i >= 0; --i) {
            Object lobj = s.pop();
            if (!std::holds_alternative<List>(lobj))
                throw std::runtime_error("Bad argument type");
            lists[i] = std::get<List>(std::move(lobj));
        }
        size_t len = lists[0].items.size();
        for (int i = 1; i < n; ++i) {
            if (lists[i].items.size() != len)
                throw std::runtime_error("Lists must have same length");
        }
        auto& prog = std::get<Program>(prog_obj);
        List result;
        for (size_t j = 0; j < len; ++j) {
            for (int i = 0; i < n; ++i) s.push(lists[i].items[j]);
            ctx.execute_tokens(prog.tokens);
            result.items.push_back(s.pop());
        }
        s.push(std::move(result));
    });

    // MAP : apply program to each element, collect results into list
    register_command("MAP", [](Store& s, Context& ctx) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object prog_obj = s.pop();
        if (!std::holds_alternative<Program>(prog_obj))
            throw std::runtime_error("Bad argument type");
        Object lobj = s.pop();
        if (!std::holds_alternative<List>(lobj))
            throw std::runtime_error("Bad argument type");
        auto& input_list = std::get<List>(lobj);
        auto& prog = std::get<Program>(prog_obj);
        List result;
        for (auto& item : input_list.items) {
            s.push(item);
            ctx.execute_tokens(prog.tokens);
            result.items.push_back(s.pop());
        }
        s.push(std::move(result));
    });

    // STREAM : reduce list with binary program
    register_command("STREAM", [](Store& s, Context& ctx) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object prog_obj = s.pop();
        if (!std::holds_alternative<Program>(prog_obj))
            throw std::runtime_error("Bad argument type");
        Object lobj = s.pop();
        if (!std::holds_alternative<List>(lobj))
            throw std::runtime_error("Bad argument type");
        auto& items = std::get<List>(lobj).items;
        if (items.empty()) throw std::runtime_error("Empty list");
        auto& prog = std::get<Program>(prog_obj);
        s.push(items[0]);
        for (size_t i = 1; i < items.size(); ++i) {
            s.push(items[i]);
            ctx.execute_tokens(prog.tokens);
        }
    });

    // SEQ : generate list from start, step, count using program
    register_command("SEQ", [](Store& s, Context& ctx) {
        if (s.depth() < 4) throw std::runtime_error("Too few arguments");
        Object prog_obj = s.pop();
        Object count_obj = s.pop();
        Object step_obj = s.pop();
        Object start_obj = s.pop();
        if (!std::holds_alternative<Program>(prog_obj) ||
            !std::holds_alternative<Integer>(count_obj))
            throw std::runtime_error("Bad argument type");
        int count = static_cast<int>(std::get<Integer>(count_obj));
        auto& prog = std::get<Program>(prog_obj);
        List result;
        Object current = start_obj;
        for (int i = 0; i < count; ++i) {
            s.push(current);
            ctx.execute_tokens(prog.tokens);
            result.items.push_back(s.pop());
            // Advance: current = current + step
            s.push(current);
            s.push(step_obj);
            ctx.execute_tokens({Token::make_command("+")});
            current = s.pop();
        }
        s.push(std::move(result));
    });

    // FILTER : keep elements where program returns truthy
    register_command("FILTER", [](Store& s, Context& ctx) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object prog_obj = s.pop();
        if (!std::holds_alternative<Program>(prog_obj))
            throw std::runtime_error("Bad argument type");
        Object lobj = s.pop();
        if (!std::holds_alternative<List>(lobj))
            throw std::runtime_error("Bad argument type");
        auto& items = std::get<List>(lobj).items;
        auto& prog = std::get<Program>(prog_obj);
        List result;
        for (auto& item : items) {
            s.push(item);
            ctx.execute_tokens(prog.tokens);
            Object test = s.pop();
            if (is_truthy(test)) {
                result.items.push_back(item);
            }
        }
        s.push(std::move(result));
    });

    // DOSUBS : apply program to sliding windows of N elements
    register_command("DOSUBS", [](Store& s, Context& ctx) {
        if (s.depth() < 3) throw std::runtime_error("Too few arguments");
        Object prog_obj = s.pop();
        Object n_obj = s.pop();
        Object lobj = s.pop();
        if (!std::holds_alternative<Program>(prog_obj) ||
            !std::holds_alternative<Integer>(n_obj) ||
            !std::holds_alternative<List>(lobj))
            throw std::runtime_error("Bad argument type");
        int n = static_cast<int>(std::get<Integer>(n_obj));
        auto& items = std::get<List>(lobj).items;
        int sz = static_cast<int>(items.size());
        if (n < 1 || n > sz) throw std::runtime_error("Bad argument value");
        auto& prog = std::get<Program>(prog_obj);
        List result;
        for (int i = 0; i <= sz - n; ++i) {
            for (int j = 0; j < n; ++j) s.push(items[i + j]);
            ctx.execute_tokens(prog.tokens);
            result.items.push_back(s.pop());
        }
        s.push(std::move(result));
    });

    // ZIP : transpose N lists into list of lists
    register_command("ZIP", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object n_obj = s.pop();
        if (!std::holds_alternative<Integer>(n_obj))
            throw std::runtime_error("Bad argument type");
        int n = static_cast<int>(std::get<Integer>(n_obj));
        if (n < 1 || s.depth() < n) throw std::runtime_error("Too few arguments");
        std::vector<List> lists(n);
        for (int i = n - 1; i >= 0; --i) {
            Object lobj = s.pop();
            if (!std::holds_alternative<List>(lobj))
                throw std::runtime_error("Bad argument type");
            lists[i] = std::get<List>(std::move(lobj));
        }
        size_t len = lists[0].items.size();
        for (int i = 1; i < n; ++i) {
            if (lists[i].items.size() != len)
                throw std::runtime_error("Lists must have same length");
        }
        List result;
        for (size_t j = 0; j < len; ++j) {
            List row;
            for (int i = 0; i < n; ++i) row.items.push_back(lists[i].items[j]);
            result.items.push_back(std::move(row));
        }
        s.push(std::move(result));
    });

    // --- Set operations ---

    // UNION
    register_command("UNION", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<List>(a) || !std::holds_alternative<List>(b))
            throw std::runtime_error("Bad argument type");
        auto result = std::get<List>(a);
        auto& b_items = std::get<List>(b).items;
        for (auto& item : b_items) {
            bool found = false;
            for (auto& existing : result.items) {
                if (repr(existing) == repr(item)) { found = true; break; }
            }
            if (!found) result.items.push_back(item);
        }
        s.push(std::move(result));
    });

    // INTERSECT
    register_command("INTERSECT", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<List>(a) || !std::holds_alternative<List>(b))
            throw std::runtime_error("Bad argument type");
        auto& a_items = std::get<List>(a).items;
        auto& b_items = std::get<List>(b).items;
        List result;
        for (auto& item : a_items) {
            for (auto& bi : b_items) {
                if (repr(item) == repr(bi)) {
                    result.items.push_back(item);
                    break;
                }
            }
        }
        s.push(std::move(result));
    });

    // DIFFERENCE
    register_command("DIFFERENCE", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<List>(a) || !std::holds_alternative<List>(b))
            throw std::runtime_error("Bad argument type");
        auto& a_items = std::get<List>(a).items;
        auto& b_items = std::get<List>(b).items;
        List result;
        for (auto& item : a_items) {
            bool found = false;
            for (auto& bi : b_items) {
                if (repr(item) == repr(bi)) { found = true; break; }
            }
            if (!found) result.items.push_back(item);
        }
        s.push(std::move(result));
    });
}

// ---- Matrix Commands ----

void CommandRegistry::register_matrix_commands() {
    // ->V2 : construct 2D vector
    auto to_v2 = [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object y = s.pop();
        Object x = s.pop();
        Matrix m;
        m.rows.push_back({std::move(x), std::move(y)});
        s.push(std::move(m));
    };
    register_command("\xe2\x86\x92V2", to_v2);
    register_command("->V2", to_v2);

    // ->V3 : construct 3D vector
    auto to_v3 = [](Store& s, Context&) {
        if (s.depth() < 3) throw std::runtime_error("Too few arguments");
        Object z = s.pop();
        Object y = s.pop();
        Object x = s.pop();
        Matrix m;
        m.rows.push_back({std::move(x), std::move(y), std::move(z)});
        s.push(std::move(m));
    };
    register_command("\xe2\x86\x92V3", to_v3);
    register_command("->V3", to_v3);

    // V-> : explode vector onto stack
    auto v_to = [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<Matrix>(a))
            throw std::runtime_error("Bad argument type");
        auto& rows = std::get<Matrix>(a).rows;
        if (rows.size() != 1)
            throw std::runtime_error("V-> requires a vector (1-row matrix)");
        for (auto& elem : rows[0]) s.push(elem);
    };
    register_command("V\xe2\x86\x92", v_to);
    register_command("V->", v_to);

    // CON : constant matrix/vector of given dimensions
    register_command("CON", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object val = s.pop();
        Object dims_obj = s.pop();
        if (!std::holds_alternative<List>(dims_obj))
            throw std::runtime_error("Bad argument type");
        auto& dims = std::get<List>(dims_obj).items;
        Matrix m;
        if (dims.size() == 1) {
            int cols = static_cast<int>(std::get<Integer>(dims[0]));
            m.rows.push_back(std::vector<Object>(cols, val));
        } else if (dims.size() == 2) {
            int rows = static_cast<int>(std::get<Integer>(dims[0]));
            int cols = static_cast<int>(std::get<Integer>(dims[1]));
            for (int r = 0; r < rows; ++r)
                m.rows.push_back(std::vector<Object>(cols, val));
        } else {
            throw std::runtime_error("Bad argument type");
        }
        s.push(std::move(m));
    });

    // IDN : identity matrix
    register_command("IDN", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object n_obj = s.pop();
        if (!std::holds_alternative<Integer>(n_obj))
            throw std::runtime_error("Bad argument type");
        int n = static_cast<int>(std::get<Integer>(n_obj));
        if (n < 1) throw std::runtime_error("Bad argument value");
        Matrix m;
        for (int r = 0; r < n; ++r) {
            std::vector<Object> row(n, Integer(0));
            row[r] = Integer(1);
            m.rows.push_back(std::move(row));
        }
        s.push(std::move(m));
    });

    // RDM : redimension matrix
    register_command("RDM", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object dims_obj = s.pop();
        Object mat_obj = s.pop();
        if (!std::holds_alternative<Matrix>(mat_obj) || !std::holds_alternative<List>(dims_obj))
            throw std::runtime_error("Bad argument type");
        auto& old_mat = std::get<Matrix>(mat_obj);
        auto& dims = std::get<List>(dims_obj).items;
        // Flatten old matrix
        std::vector<Object> flat;
        for (auto& row : old_mat.rows)
            for (auto& elem : row)
                flat.push_back(elem);
        Matrix m;
        if (dims.size() == 1) {
            int cols = static_cast<int>(std::get<Integer>(dims[0]));
            std::vector<Object> row;
            for (int c = 0; c < cols; ++c)
                row.push_back(c < static_cast<int>(flat.size()) ? flat[c] : Integer(0));
            m.rows.push_back(std::move(row));
        } else if (dims.size() == 2) {
            int rows = static_cast<int>(std::get<Integer>(dims[0]));
            int cols = static_cast<int>(std::get<Integer>(dims[1]));
            size_t idx = 0;
            for (int r = 0; r < rows; ++r) {
                std::vector<Object> row;
                for (int c = 0; c < cols; ++c) {
                    row.push_back(idx < flat.size() ? flat[idx] : Integer(0));
                    ++idx;
                }
                m.rows.push_back(std::move(row));
            }
        } else {
            throw std::runtime_error("Bad argument type");
        }
        s.push(std::move(m));
    });

    // TRN : transpose
    register_command("TRN", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<Matrix>(a))
            throw std::runtime_error("Bad argument type");
        auto& rows = std::get<Matrix>(a).rows;
        if (rows.empty()) { s.push(a); return; }
        int nr = static_cast<int>(rows.size());
        int nc = static_cast<int>(rows[0].size());
        Matrix result;
        for (int c = 0; c < nc; ++c) {
            std::vector<Object> row;
            for (int r = 0; r < nr; ++r) row.push_back(rows[r][c]);
            result.rows.push_back(std::move(row));
        }
        s.push(std::move(result));
    });

    // DET : determinant (numeric or symbolic)
    register_command("DET", [this](Store& s, Context& ctx) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object a = s.pop();
        if (!std::holds_alternative<Matrix>(a))
            throw std::runtime_error("Bad argument type");
        auto& rows = std::get<Matrix>(a).rows;
        int n = static_cast<int>(rows.size());
        if (n == 0 || static_cast<int>(rows[0].size()) != n)
            throw std::runtime_error("DET requires a square matrix");

        // Recursive determinant via cofactor expansion
        std::function<Object(const std::vector<std::vector<Object>>&, int)> det;
        det = [&](const std::vector<std::vector<Object>>& m, int sz) -> Object {
            if (sz == 1) return m[0][0];
            if (sz == 2) {
                // a*d - b*c
                s.push(m[0][0]); s.push(m[1][1]);
                this->execute("*", s, ctx);
                s.push(m[0][1]); s.push(m[1][0]);
                this->execute("*", s, ctx);
                this->execute("-", s, ctx);
                return s.pop();
            }
            // Cofactor expansion along first row
            Object result = Integer(0);
            bool first = true;
            for (int j = 0; j < sz; ++j) {
                // Build minor matrix
                std::vector<std::vector<Object>> minor;
                for (int r = 1; r < sz; ++r) {
                    std::vector<Object> row;
                    for (int c = 0; c < sz; ++c) {
                        if (c != j) row.push_back(m[r][c]);
                    }
                    minor.push_back(std::move(row));
                }
                Object cofactor = det(minor, sz - 1);
                // Multiply by element and sign
                s.push(m[0][j]); s.push(cofactor);
                this->execute("*", s, ctx);
                Object term = s.pop();
                if (first) {
                    result = (j % 2 == 0) ? term : term; // first term
                    if (j % 2 != 0) {
                        s.push(term); this->execute("NEG", s, ctx);
                        result = s.pop();
                    } else {
                        result = term;
                    }
                    first = false;
                } else {
                    s.push(result); s.push(term);
                    if (j % 2 == 0) {
                        this->execute("+", s, ctx);
                    } else {
                        this->execute("-", s, ctx);
                    }
                    result = s.pop();
                }
            }
            return result;
        };
        s.push(det(rows, n));
    });

    // CROSS : cross product (3D vectors)
    register_command("CROSS", [this](Store& s, Context& ctx) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<Matrix>(a) || !std::holds_alternative<Matrix>(b))
            throw std::runtime_error("Bad argument type");
        auto& ar = std::get<Matrix>(a).rows;
        auto& br = std::get<Matrix>(b).rows;
        if (ar.size() != 1 || ar[0].size() != 3 || br.size() != 1 || br[0].size() != 3)
            throw std::runtime_error("CROSS requires 3D vectors");
        // a x b = [a2*b3-a3*b2, a3*b1-a1*b3, a1*b2-a2*b1]
        auto cross_elem = [&](int i1, int j1, int i2, int j2) -> Object {
            s.push(ar[0][i1]); s.push(br[0][j1]);
            this->execute("*", s, ctx);
            s.push(ar[0][i2]); s.push(br[0][j2]);
            this->execute("*", s, ctx);
            this->execute("-", s, ctx);
            return s.pop();
        };
        Object x = cross_elem(1, 2, 2, 1);
        Object y = cross_elem(2, 0, 0, 2);
        Object z = cross_elem(0, 1, 1, 0);
        Matrix result;
        result.rows.push_back({std::move(x), std::move(y), std::move(z)});
        s.push(std::move(result));
    });

    // DOT : dot product
    register_command("DOT", [this](Store& s, Context& ctx) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object b = s.pop();
        Object a = s.pop();
        if (!std::holds_alternative<Matrix>(a) || !std::holds_alternative<Matrix>(b))
            throw std::runtime_error("Bad argument type");
        auto& ar = std::get<Matrix>(a).rows;
        auto& br = std::get<Matrix>(b).rows;
        if (ar.size() != 1 || br.size() != 1 || ar[0].size() != br[0].size())
            throw std::runtime_error("DOT requires vectors of equal length");
        size_t n = ar[0].size();
        // First product
        s.push(ar[0][0]); s.push(br[0][0]);
        this->execute("*", s, ctx);
        // Accumulate remaining products
        for (size_t i = 1; i < n; ++i) {
            s.push(ar[0][i]); s.push(br[0][i]);
            this->execute("*", s, ctx);
            this->execute("+", s, ctx);
        }
    });

    // ABS on vectors (Euclidean norm)
    // This is handled in the arithmetic overload section
}

// ---- Display Commands ----

void CommandRegistry::register_display_commands() {
    register_command("STD", [](Store& s, Context&) {
        s.set_meta("number_format", "STD");
        s.set_meta("format_digits", "0");
    });

    register_command("FIX", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object obj = s.pop();
        if (!std::holds_alternative<Integer>(obj))
            throw std::runtime_error("FIX: expected Integer");
        int n = std::get<Integer>(obj).convert_to<int>();
        if (n < 0 || n > 11) throw std::runtime_error("FIX: digits must be 0-11");
        s.set_meta("number_format", "FIX");
        s.set_meta("format_digits", std::to_string(n));
    });

    register_command("SCI", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object obj = s.pop();
        if (!std::holds_alternative<Integer>(obj))
            throw std::runtime_error("SCI: expected Integer");
        int n = std::get<Integer>(obj).convert_to<int>();
        if (n < 0 || n > 11) throw std::runtime_error("SCI: digits must be 0-11");
        s.set_meta("number_format", "SCI");
        s.set_meta("format_digits", std::to_string(n));
    });

    register_command("ENG", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object obj = s.pop();
        if (!std::holds_alternative<Integer>(obj))
            throw std::runtime_error("ENG: expected Integer");
        int n = std::get<Integer>(obj).convert_to<int>();
        if (n < 0 || n > 11) throw std::runtime_error("ENG: digits must be 0-11");
        s.set_meta("number_format", "ENG");
        s.set_meta("format_digits", std::to_string(n));
    });

    register_command("RECT", [](Store& s, Context&) {
        s.set_meta("coordinate_mode", "RECT");
    });

    register_command("POLAR", [](Store& s, Context&) {
        s.set_meta("coordinate_mode", "POLAR");
    });

    register_command("SPHERICAL", [](Store& s, Context&) {
        s.set_meta("coordinate_mode", "SPHERICAL");
    });
}

// ---- Flag Commands ----

void CommandRegistry::register_flag_commands() {
    register_command("SF", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object obj = s.pop();
        std::string name;
        if (std::holds_alternative<String>(obj)) name = std::get<String>(obj).value;
        else if (std::holds_alternative<Name>(obj)) name = std::get<Name>(obj).value;
        else throw std::runtime_error("SF: expected String or Name");
        s.set_flag(name, 0, "1");
    });

    register_command("CF", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object obj = s.pop();
        std::string name;
        if (std::holds_alternative<String>(obj)) name = std::get<String>(obj).value;
        else if (std::holds_alternative<Name>(obj)) name = std::get<Name>(obj).value;
        else throw std::runtime_error("CF: expected String or Name");
        s.set_flag(name, 0, "0");
    });

    register_command("FS?", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object obj = s.pop();
        std::string name;
        if (std::holds_alternative<String>(obj)) name = std::get<String>(obj).value;
        else if (std::holds_alternative<Name>(obj)) name = std::get<Name>(obj).value;
        else throw std::runtime_error("FS?: expected String or Name");
        auto flag = s.get_flag(name);
        if (flag && std::get<1>(*flag) == "1")
            s.push(Integer(1));
        else
            s.push(Integer(0));
    });

    register_command("FC?", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object obj = s.pop();
        std::string name;
        if (std::holds_alternative<String>(obj)) name = std::get<String>(obj).value;
        else if (std::holds_alternative<Name>(obj)) name = std::get<Name>(obj).value;
        else throw std::runtime_error("FC?: expected String or Name");
        auto flag = s.get_flag(name);
        if (!flag || std::get<1>(*flag) == "0")
            s.push(Integer(1));
        else
            s.push(Integer(0));
    });

    register_command("SFLAG", [](Store& s, Context&) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object name_obj = s.pop();
        Object val_obj = s.pop();
        std::string name;
        if (std::holds_alternative<String>(name_obj)) name = std::get<String>(name_obj).value;
        else if (std::holds_alternative<Name>(name_obj)) name = std::get<Name>(name_obj).value;
        else throw std::runtime_error("SFLAG: expected String or Name for flag name");

        if (std::holds_alternative<Integer>(val_obj)) {
            s.set_flag(name, 1, std::get<Integer>(val_obj).str());
        } else if (std::holds_alternative<Real>(val_obj)) {
            s.set_flag(name, 2, std::get<Real>(val_obj).str());
        } else if (std::holds_alternative<String>(val_obj)) {
            s.set_flag(name, 3, std::get<String>(val_obj).value);
        } else {
            throw std::runtime_error("SFLAG: unsupported value type");
        }
    });

    register_command("RFLAG", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object obj = s.pop();
        std::string name;
        if (std::holds_alternative<String>(obj)) name = std::get<String>(obj).value;
        else if (std::holds_alternative<Name>(obj)) name = std::get<Name>(obj).value;
        else throw std::runtime_error("RFLAG: expected String or Name");

        auto flag = s.get_flag(name);
        if (!flag) {
            throw std::runtime_error("RFLAG: undefined flag '" + name + "'");
        }
        int type_tag = std::get<0>(*flag);
        const std::string& value = std::get<1>(*flag);
        switch (type_tag) {
            case 0: s.push(Integer(value == "1" ? 1 : 0)); break;
            case 1: s.push(Integer(value)); break;
            case 2: s.push(Real(value)); break;
            case 3: s.push(String{value}); break;
            default: throw std::runtime_error("RFLAG: unknown type tag");
        }
    });

    register_command("STOF", [](Store& s, Context&) {
        auto flags = s.all_flags();
        List result;
        for (auto& [name, type_tag, value] : flags) {
            List pair;
            pair.items.push_back(String{name});
            switch (type_tag) {
                case 0: pair.items.push_back(Integer(value == "1" ? 1 : 0)); break;
                case 1: pair.items.push_back(Integer(value)); break;
                case 2: pair.items.push_back(Real(value)); break;
                case 3: pair.items.push_back(String{value}); break;
                default: pair.items.push_back(String{value}); break;
            }
            result.items.push_back(std::move(pair));
        }
        s.push(std::move(result));
    });

    register_command("RCLF", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object obj = s.pop();
        if (!std::holds_alternative<List>(obj))
            throw std::runtime_error("RCLF: expected List");
        const auto& list = std::get<List>(obj);

        s.clear_all_flags();
        for (const auto& item : list.items) {
            if (!std::holds_alternative<List>(item))
                throw std::runtime_error("RCLF: each element must be a { name value } list");
            const auto& pair = std::get<List>(item);
            if (pair.items.size() != 2)
                throw std::runtime_error("RCLF: each element must have exactly 2 items");
            if (!std::holds_alternative<String>(pair.items[0]))
                throw std::runtime_error("RCLF: flag name must be a String");
            const std::string& name = std::get<String>(pair.items[0]).value;
            const auto& val = pair.items[1];

            if (std::holds_alternative<Integer>(val)) {
                // Could be bool (0/1) or integer — store as integer type
                int iv = std::get<Integer>(val).convert_to<int>();
                if (iv == 0 || iv == 1) {
                    s.set_flag(name, 0, std::to_string(iv));
                } else {
                    s.set_flag(name, 1, std::get<Integer>(val).str());
                }
            } else if (std::holds_alternative<Real>(val)) {
                s.set_flag(name, 2, std::get<Real>(val).str());
            } else if (std::holds_alternative<String>(val)) {
                s.set_flag(name, 3, std::get<String>(val).value);
            } else {
                throw std::runtime_error("RCLF: unsupported value type");
            }
        }
    });
}

// ---- Conversion Commands ----

void CommandRegistry::register_conversion_commands() {
    register_command("->Q", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object obj = s.pop();
        double dv;
        if (std::holds_alternative<Real>(obj)) {
            dv = std::get<Real>(obj).convert_to<double>();
        } else if (std::holds_alternative<Integer>(obj)) {
            // Integer is already exact — wrap as rational
            s.push(Rational(std::get<Integer>(obj), Integer(1)));
            return;
        } else {
            throw std::runtime_error("->Q: expected Real or Integer");
        }

        // Continued fraction approximation
        double x = dv;
        bool neg = x < 0;
        if (neg) x = -x;
        Integer h0(0), h1(1), k0(1), k1(0);
        double rem = x;
        for (int i = 0; i < 30; ++i) {
            Integer a(static_cast<long long>(std::floor(rem)));
            Integer h2 = a * h1 + h0;
            Integer k2 = a * k1 + k0;
            h0 = h1; h1 = h2;
            k0 = k1; k1 = k2;
            double frac = rem - std::floor(rem);
            if (frac < 1e-12) break;
            rem = 1.0 / frac;
            if (rem > 1e12) break;
        }
        if (neg) h1 = -h1;
        s.push(Rational(h1, k1));
    });

    register_command("HMS->", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object obj = s.pop();
        Real hms;
        if (std::holds_alternative<Real>(obj))
            hms = std::get<Real>(obj);
        else if (std::holds_alternative<Integer>(obj))
            hms = Real(std::get<Integer>(obj));
        else
            throw std::runtime_error("HMS->: expected Real or Integer");

        bool neg = hms < 0;
        if (neg) hms = -hms;
        Real h = floor(hms);
        Real frac = (hms - h) * 100;
        Real m = floor(frac);
        Real sec = (frac - m) * 100;
        Real result = h + m / 60 + sec / 3600;
        if (neg) result = -result;
        s.push(result);
    });

    register_command("->HMS", [](Store& s, Context&) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object obj = s.pop();
        Real dec;
        if (std::holds_alternative<Real>(obj))
            dec = std::get<Real>(obj);
        else if (std::holds_alternative<Integer>(obj))
            dec = Real(std::get<Integer>(obj));
        else
            throw std::runtime_error("->HMS: expected Real or Integer");

        bool neg = dec < 0;
        if (neg) dec = -dec;
        Real h = floor(dec);
        Real rem = (dec - h) * 60;
        Real m = floor(rem);
        Real sec = (rem - m) * 60;
        Real result = h + m / 100 + sec / 10000;
        if (neg) result = -result;
        s.push(result);
    });
}

// ---- CAS commands ----

void CommandRegistry::register_cas_commands() {
    // DIFF: (level 2: Symbol expr, level 1: Name var) → Symbol derivative
    register_command("DIFF", [](Store& s, Context& ctx) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object var_obj = s.pop();
        Object expr_obj = s.pop();
        if (!std::holds_alternative<Name>(var_obj)) {
            s.push(expr_obj);
            s.push(var_obj);
            throw std::runtime_error("DIFF: variable must be a name");
        }
        auto result = ctx.cas().differentiate(expr_obj, std::get<Name>(var_obj).value);
        s.push(result);
    });

    // INTEGRATE: (level 2: Symbol expr, level 1: Name var) → Symbol antiderivative
    register_command("INTEGRATE", [](Store& s, Context& ctx) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object var_obj = s.pop();
        Object expr_obj = s.pop();
        if (!std::holds_alternative<Name>(var_obj)) {
            s.push(expr_obj);
            s.push(var_obj);
            throw std::runtime_error("INTEGRATE: variable must be a name");
        }
        auto result = ctx.cas().integrate(expr_obj, std::get<Name>(var_obj).value);
        s.push(result);
    });

    // SOLVE: (level 2: Symbol expr, level 1: Name var) → List of solutions
    register_command("SOLVE", [](Store& s, Context& ctx) {
        if (s.depth() < 2) throw std::runtime_error("Too few arguments");
        Object var_obj = s.pop();
        Object expr_obj = s.pop();
        if (!std::holds_alternative<Name>(var_obj)) {
            s.push(expr_obj);
            s.push(var_obj);
            throw std::runtime_error("SOLVE: variable must be a name");
        }
        auto result = ctx.cas().solve(expr_obj, std::get<Name>(var_obj).value);
        s.push(result);
    });

    // SIMPLIFY: (level 1: Symbol expr) → Symbol simplified
    register_command("SIMPLIFY", [](Store& s, Context& ctx) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object expr_obj = s.pop();
        auto result = ctx.cas().simplify(expr_obj);
        s.push(result);
    });

    // EXPAND: (level 1: Symbol expr) → Symbol expanded
    register_command("EXPAND", [](Store& s, Context& ctx) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object expr_obj = s.pop();
        auto result = ctx.cas().expand(expr_obj);
        s.push(result);
    });

    // FACTOR: (level 1: Symbol expr) → Symbol factored
    register_command("FACTOR", [](Store& s, Context& ctx) {
        if (s.depth() < 1) throw std::runtime_error("Too few arguments");
        Object expr_obj = s.pop();
        auto result = ctx.cas().factor(expr_obj);
        s.push(result);
    });
}

} // namespace lpr
