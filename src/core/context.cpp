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

void Context::execute_tokens(const std::vector<Token>& tokens) {
    for (const auto& tok : tokens) {
        if (tok.kind == Token::Literal) {
            store_.push(tok.literal);
        } else {
            commands_.execute(tok.command, store_, *this);
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

} // namespace lpr
