#pragma once

#include "core/object.hpp"
#include <string>
#include <vector>

namespace lpr {

std::vector<Token> parse(const std::string& input);

} // namespace lpr
