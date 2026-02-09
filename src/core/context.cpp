#include "core/context.hpp"
#include "core/parser.hpp"
#include <stdexcept>

namespace lpr {

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
