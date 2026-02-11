#include "core/context.hpp"
#include "core/parser.hpp"
#include <stdexcept>

namespace lpr {

// Helper: collect tokens from position i until a keyword at nesting depth 0.
// Properly tracks nesting: FOR/START close with NEXT/STEP, others close with END.
static std::vector<Token> collect_until(
    const std::vector<Token>& tokens, size_t& i,
    const std::vector<std::string>& stop_keywords)
{
    std::vector<Token> collected;
    // Stack of expected closers: 'E' = END, 'N' = NEXT/STEP
    std::vector<char> nest;
    while (i < tokens.size()) {
        const auto& t = tokens[i];
        if (t.kind == Token::Command) {
            if (nest.empty()) {
                for (const auto& kw : stop_keywords) {
                    if (t.command == kw) return collected;
                }
            }
            // Track nesting opens
            if (t.command == "IF" || t.command == "CASE" || t.command == "WHILE" || t.command == "DO") {
                nest.push_back('E'); // closed by END
            } else if (t.command == "FOR" || t.command == "START") {
                nest.push_back('N'); // closed by NEXT or STEP
            }
            // Track nesting closes
            if (!nest.empty()) {
                if (nest.back() == 'E' && t.command == "END") {
                    nest.pop_back();
                } else if (nest.back() == 'N' && (t.command == "NEXT" || t.command == "STEP")) {
                    nest.pop_back();
                }
            }
        }
        collected.push_back(t);
        ++i;
    }
    throw std::runtime_error("Unexpected end of tokens in control structure");
}

Context::Context(const char* db_path)
    : store_(db_path)
{}

bool Context::exec(const std::string& input) {
    store_.begin();
    try {
        // Snapshot BEFORE mutation (so we can undo back to this state)
        int pre_seq = store_.snapshot_stack();

        auto tokens = parse(input);
        execute_tokens(tokens);

        // Snapshot AFTER mutation (the result state)
        int post_seq = store_.snapshot_stack();

        store_.commit();
        return true;
    } catch (const std::exception& e) {
        store_.rollback();

        // Push error onto the pre-exec state
        store_.begin();
        store_.push(Error{100, e.what()});
        store_.commit();
        return false;
    }
}

static bool is_arrow_command(const std::string& cmd) {
    return cmd == "->" || cmd == "\xe2\x86\x92";
}

void Context::execute_tokens(const std::vector<Token>& tokens) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& tok = tokens[i];
        if (tok.kind == Token::Literal) {
            store_.push(tok.literal);
        } else if (is_arrow_command(tok.command)) {
            // Runstream-consuming: collect parameter names until we hit
            // a Symbol or Program token (the body).
            std::vector<std::string> names;
            ++i;
            while (i < tokens.size()) {
                const auto& t = tokens[i];
                if (t.kind == Token::Literal) {
                    // Body found (Symbol or Program)
                    break;
                }
                // Command token = parameter name (already uppercased)
                names.push_back(t.command);
                ++i;
            }
            if (i >= tokens.size()) {
                throw std::runtime_error("-> missing body");
            }
            if (names.empty()) {
                throw std::runtime_error("-> requires at least one variable name");
            }

            // Pop stack values and bind to names (first name binds deepest)
            int n = static_cast<int>(names.size());
            if (store_.depth() < n) {
                throw std::runtime_error("Too few arguments for ->");
            }
            std::unordered_map<std::string, Object> frame;
            // Pop in reverse: last name gets level 1, first name gets level N
            std::vector<Object> vals(n);
            for (int j = n - 1; j >= 0; --j) {
                vals[j] = store_.pop();
            }
            for (int j = 0; j < n; ++j) {
                frame[names[j]] = std::move(vals[j]);
            }

            push_locals(frame);

            const auto& body_tok = tokens[i];
            if (body_tok.kind == Token::Literal &&
                std::holds_alternative<Program>(body_tok.literal)) {
                execute_tokens(std::get<Program>(body_tok.literal).tokens);
            } else if (body_tok.kind == Token::Literal &&
                       std::holds_alternative<Symbol>(body_tok.literal)) {
                // Evaluate the symbol expression
                store_.push(body_tok.literal);
                commands_.execute("EVAL", store_, *this);
            } else {
                pop_locals();
                throw std::runtime_error("-> body must be a Symbol or Program");
            }

            pop_locals();
        } else if (tok.command == "IF") {
            // IF ... THEN ... [ELSE ...] END
            // Collect condition tokens up to THEN
            ++i;
            auto cond_tokens = collect_until(tokens, i, {"THEN"});
            ++i; // skip THEN
            // Collect then-body up to ELSE or END
            auto then_tokens = collect_until(tokens, i, {"ELSE", "END"});
            std::vector<Token> else_tokens;
            if (i < tokens.size() && tokens[i].kind == Token::Command && tokens[i].command == "ELSE") {
                ++i; // skip ELSE
                else_tokens = collect_until(tokens, i, {"END"});
            }
            // i now points to END, loop increment will advance past it

            // Execute condition
            execute_tokens(cond_tokens);
            if (store_.depth() < 1) throw std::runtime_error("IF: missing condition result");
            Object cond_val = store_.pop();
            bool cond_true = false;
            if (std::holds_alternative<Integer>(cond_val)) cond_true = std::get<Integer>(cond_val) != 0;
            else if (std::holds_alternative<Real>(cond_val)) cond_true = std::get<Real>(cond_val) != 0;
            else throw std::runtime_error("IF: condition must be numeric");

            if (cond_true) {
                execute_tokens(then_tokens);
            } else if (!else_tokens.empty()) {
                execute_tokens(else_tokens);
            }

        } else if (tok.command == "CASE") {
            // CASE val1 THEN body1 END val2 THEN body2 END ... [default] END
            ++i;
            bool matched = false;
            while (i < tokens.size()) {
                const auto& ct = tokens[i];
                // Check for final END (closes CASE)
                if (ct.kind == Token::Command && ct.command == "END") {
                    break; // end of CASE
                }
                // Collect tokens up to THEN or END
                auto test_tokens = collect_until(tokens, i, {"THEN", "END"});
                if (i < tokens.size() && tokens[i].kind == Token::Command && tokens[i].command == "END") {
                    // This is the default clause (no THEN found) — test_tokens is the default body
                    if (!matched) {
                        execute_tokens(test_tokens);
                    }
                    break; // END closes CASE
                }
                // We hit THEN
                ++i; // skip THEN
                auto body_tokens = collect_until(tokens, i, {"END"});
                ++i; // skip END (the one closing this THEN clause)

                if (!matched) {
                    execute_tokens(test_tokens);
                    if (store_.depth() < 1) throw std::runtime_error("CASE: missing test result");
                    Object test_val = store_.pop();
                    bool test_true = false;
                    if (std::holds_alternative<Integer>(test_val)) test_true = std::get<Integer>(test_val) != 0;
                    else if (std::holds_alternative<Real>(test_val)) test_true = std::get<Real>(test_val) != 0;
                    if (test_true) {
                        execute_tokens(body_tokens);
                        matched = true;
                        // Skip remaining clauses until final END
                    }
                }
            }
            // i points to the final END of CASE

        } else if (tok.command == "FOR") {
            // FOR varname body NEXT  or  FOR varname body STEP
            ++i;
            if (i >= tokens.size() || tokens[i].kind != Token::Command) {
                throw std::runtime_error("FOR: expected variable name");
            }
            std::string var_name = tokens[i].command;
            ++i;
            // Collect body up to NEXT or STEP
            auto body_tokens = collect_until(tokens, i, {"NEXT", "STEP"});
            bool has_step = (i < tokens.size() && tokens[i].kind == Token::Command && tokens[i].command == "STEP");
            // i points to NEXT or STEP

            // Pop start and end from stack
            if (store_.depth() < 2) throw std::runtime_error("FOR: Too few arguments");
            Object end_obj = store_.pop();
            Object start_obj = store_.pop();

            auto to_real = [](const Object& o) -> Real {
                if (std::holds_alternative<Integer>(o)) return Real(std::get<Integer>(o));
                if (std::holds_alternative<Real>(o)) return std::get<Real>(o);
                throw std::runtime_error("FOR: arguments must be numeric");
            };
            Real start_r = to_real(start_obj);
            Real end_r = to_real(end_obj);
            Real step_r = 1;
            Real counter = start_r;
            bool use_int = std::holds_alternative<Integer>(start_obj);
            bool first = true;

            for (;;) {
                // Termination check (skip on first iteration for STEP since step is unknown)
                if (!first || !has_step) {
                    if (step_r > 0 && counter > end_r) break;
                    if (step_r < 0 && counter < end_r) break;
                }
                first = false;

                // Bind loop variable
                std::unordered_map<std::string, Object> frame;
                if (use_int) {
                    Integer ic(counter);
                    frame[var_name] = ic;
                } else {
                    frame[var_name] = counter;
                }
                push_locals(frame);
                execute_tokens(body_tokens);
                pop_locals();

                if (has_step) {
                    if (store_.depth() < 1) throw std::runtime_error("STEP: missing step value");
                    Object step_obj = store_.pop();
                    step_r = to_real(step_obj);
                }

                counter += step_r;
            }

        } else if (tok.command == "START") {
            // START body NEXT  or  START body STEP
            ++i;
            auto body_tokens = collect_until(tokens, i, {"NEXT", "STEP"});
            bool has_step = (i < tokens.size() && tokens[i].kind == Token::Command && tokens[i].command == "STEP");
            // i points to NEXT or STEP

            if (store_.depth() < 2) throw std::runtime_error("START: Too few arguments");
            Object end_obj = store_.pop();
            Object start_obj = store_.pop();

            auto to_real = [](const Object& o) -> Real {
                if (std::holds_alternative<Integer>(o)) return Real(std::get<Integer>(o));
                if (std::holds_alternative<Real>(o)) return std::get<Real>(o);
                throw std::runtime_error("START: arguments must be numeric");
            };
            Real start_r = to_real(start_obj);
            Real end_r = to_real(end_obj);
            Real step_r = 1;
            Real counter = start_r;
            bool first = true;

            for (;;) {
                if (!first || !has_step) {
                    if (step_r > 0 && counter > end_r) break;
                    if (step_r < 0 && counter < end_r) break;
                }
                first = false;

                execute_tokens(body_tokens);

                if (has_step) {
                    if (store_.depth() < 1) throw std::runtime_error("STEP: missing step value");
                    Object step_obj = store_.pop();
                    step_r = to_real(step_obj);
                }

                counter += step_r;
            }

        } else if (tok.command == "WHILE") {
            // WHILE condition REPEAT body END
            ++i;
            auto cond_tokens = collect_until(tokens, i, {"REPEAT"});
            ++i; // skip REPEAT
            auto body_tokens = collect_until(tokens, i, {"END"});
            // i points to END

            for (;;) {
                execute_tokens(cond_tokens);
                if (store_.depth() < 1) throw std::runtime_error("WHILE: missing condition result");
                Object cond_val = store_.pop();
                bool cond_true = false;
                if (std::holds_alternative<Integer>(cond_val)) cond_true = std::get<Integer>(cond_val) != 0;
                else if (std::holds_alternative<Real>(cond_val)) cond_true = std::get<Real>(cond_val) != 0;
                if (!cond_true) break;
                execute_tokens(body_tokens);
            }

        } else if (tok.command == "DO") {
            // DO body UNTIL condition END
            ++i;
            auto body_tokens = collect_until(tokens, i, {"UNTIL"});
            ++i; // skip UNTIL
            auto cond_tokens = collect_until(tokens, i, {"END"});
            // i points to END

            for (;;) {
                execute_tokens(body_tokens);
                execute_tokens(cond_tokens);
                if (store_.depth() < 1) throw std::runtime_error("UNTIL: missing condition result");
                Object cond_val = store_.pop();
                bool cond_true = false;
                if (std::holds_alternative<Integer>(cond_val)) cond_true = std::get<Integer>(cond_val) != 0;
                else if (std::holds_alternative<Real>(cond_val)) cond_true = std::get<Real>(cond_val) != 0;
                if (cond_true) break;
            }

        } else {
            // Check if it's a known command first
            if (commands_.has(tok.command)) {
                commands_.execute(tok.command, store_, *this);
            } else {
                // Try local variable resolution first
                auto local = resolve_local(tok.command);
                if (local.has_value()) {
                    store_.push(*local);
                } else {
                    // Try recalling as a variable in current directory
                    Object val = store_.recall_variable(store_.current_dir(), tok.command);
                    if (!std::holds_alternative<Error>(val)) {
                        // Found a variable — evaluate programs, push everything else
                        if (std::holds_alternative<Program>(val)) {
                            execute_tokens(std::get<Program>(val).tokens);
                        } else {
                            store_.push(val);
                        }
                    } else {
                        // Not a local, not a variable — unknown command
                        commands_.execute(tok.command, store_, *this);
                    }
                }
            }
        }
    }
}

int Context::depth() {
    return store_.depth();
}

std::string Context::repr_at(int level) {
    Object obj = store_.peek(level);
    return repr(obj);
}

bool Context::undo() {
    int cur = store_.current_undo_seq();
    // Each exec creates 2 snapshots: pre (odd) and post (even).
    // The current undo_seq points to the post snapshot.
    // To undo, we go back to the pre snapshot (cur - 1),
    // which is the state before the last operation.
    // Then set undo_seq to the post snapshot of the PREVIOUS exec (cur - 2),
    // so that another undo can go back further.
    if (cur <= 1) return false;

    int target = cur - 1; // the pre-state of the current exec
    store_.begin();
    bool ok = store_.restore_stack(target);
    if (ok) {
        // Set undo_seq to target-1 so redo can go to cur and further undo to target-2
        store_.set_undo_seq(target - 1);
    }
    store_.commit();
    return ok;
}

bool Context::redo() {
    int cur = store_.current_undo_seq();
    int max_seq = store_.history_max_seq();
    if (cur + 2 > max_seq) return false;

    // Redo: jump to the post-snapshot of the next exec.
    // After undo, cur points to a post-snapshot of a previous exec.
    // The next pre-snapshot is cur+1, and next post-snapshot is cur+2.
    int target = cur + 2;

    store_.begin();
    bool ok = store_.restore_stack(target);
    if (ok) {
        store_.set_undo_seq(target);
    }
    store_.commit();
    return ok;
}

void Context::push_locals(const std::unordered_map<std::string, Object>& frame) {
    local_scopes_.push_back(frame);
}

void Context::pop_locals() {
    if (!local_scopes_.empty()) {
        local_scopes_.pop_back();
    }
}

std::optional<Object> Context::resolve_local(const std::string& name) const {
    // Search innermost scope first
    for (auto it = local_scopes_.rbegin(); it != local_scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return std::nullopt;
}

} // namespace lpr
