#pragma once

#include "../core/object.hpp"
#include <string>

namespace lpr {

class CASBridge {
public:
    virtual ~CASBridge() = default;

    virtual Object differentiate(const Object& expr, const std::string& var) = 0;
    virtual Object integrate(const Object& expr, const std::string& var) = 0;
    virtual Object solve(const Object& expr, const std::string& var) = 0;
    virtual Object simplify(const Object& expr) = 0;
    virtual Object expand(const Object& expr) = 0;
    virtual Object factor(const Object& expr) = 0;
};

} // namespace lpr
