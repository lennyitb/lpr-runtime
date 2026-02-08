#pragma once

#include "core/store.hpp"
#include "core/commands.hpp"
#include "core/object.hpp"
#include <vector>
#include <string>

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

private:
    Store store_;
    CommandRegistry commands_;
};

} // namespace lpr
