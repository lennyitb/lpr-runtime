#pragma once

#include "core/store.hpp"
#include "core/commands.hpp"
#include "core/object.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>

namespace lpr {

class Context {
public:
    explicit Context(const char* db_path);
    ~Context() = default;

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    // Execute RPL input — returns true on success
    bool exec(const std::string& input);

    // Stack inspection
    int         depth();
    std::string repr_at(int level); // 1-based

    // Undo/Redo
    bool undo();
    bool redo();

    // Execute token stream (used by EVAL, STR→, etc.)
    void execute_tokens(const std::vector<Token>& tokens);

    Store& store() { return store_; }

    // Local variable scope stack
    void push_locals(const std::unordered_map<std::string, Object>& frame);
    void pop_locals();
    std::optional<Object> resolve_local(const std::string& name) const;

private:
    Store store_;
    CommandRegistry commands_;
    std::vector<std::unordered_map<std::string, Object>> local_scopes_;
};

} // namespace lpr
