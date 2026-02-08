#pragma once

#include "core/object.hpp"
#include "core/store.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace lpr {

class Context; // forward

using CommandFn = std::function<void(Store&, Context&)>;

class CommandRegistry {
public:
    CommandRegistry();

    void register_command(const std::string& name, CommandFn fn);
    bool has(const std::string& name) const;
    void execute(const std::string& name, Store& store, Context& ctx) const;

private:
    std::unordered_map<std::string, CommandFn> commands_;

    void register_stack_commands();
    void register_arithmetic_commands();
    void register_comparison_commands();
    void register_type_commands();
    void register_filesystem_commands();
    void register_program_commands();
};

} // namespace lpr
