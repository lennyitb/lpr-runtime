#pragma once

#include "bridge.hpp"
#include <symengine/basic.h>
#include <string>
#include <unordered_map>

namespace lpr {

class SymEngineBridge : public CASBridge {
public:
    SymEngineBridge();

    Object differentiate(const Object& expr, const std::string& var) override;
    Object integrate(const Object& expr, const std::string& var) override;
    Object solve(const Object& expr, const std::string& var) override;
    Object simplify(const Object& expr) override;
    Object expand(const Object& expr) override;
    Object factor(const Object& expr) override;

private:
    // Bidirectional function name mapping
    std::unordered_map<std::string, std::string> rpl_to_symengine_;
    std::unordered_map<std::string, std::string> symengine_to_rpl_;

    // Convert between RPL expression strings and SymEngine expressions
    SymEngine::RCP<const SymEngine::Basic> to_symengine(const std::string& expr);
    std::string from_symengine(const SymEngine::RCP<const SymEngine::Basic>& expr);

    // Integrate a single term (recursive helper)
    SymEngine::RCP<const SymEngine::Basic> integrate_term(
        const SymEngine::RCP<const SymEngine::Basic>& term,
        const SymEngine::RCP<const SymEngine::Symbol>& var);

    // Extract the string value from a Symbol Object, or throw
    std::string extract_symbol_string(const Object& obj, const std::string& cmd);
};

} // namespace lpr
