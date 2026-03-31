#include "core/parser.hpp"
#include <cctype>

namespace lpr {

namespace {

bool is_whitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

// Check if string looks like an integer: optional '-' followed by digits
bool is_integer(const std::string& s) {
    if (s.empty()) return false;
    size_t start = (s[0] == '-') ? 1 : 0;
    if (start >= s.size()) return false;
    for (size_t i = start; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}

// Check if string looks like a real: digits with '.' and/or 'E'/'e'
bool is_real(const std::string& s) {
    if (s.empty()) return false;
    bool has_dot = false;
    bool has_e = false;
    bool has_digit = false;
    size_t start = (s[0] == '-') ? 1 : 0;
    if (start >= s.size()) return false;
    for (size_t i = start; i < s.size(); ++i) {
        char c = s[i];
        if (c == '.') {
            if (has_dot || has_e) return false;
            has_dot = true;
        } else if (c == 'E' || c == 'e') {
            if (has_e || !has_digit) return false; // require digits before E
            has_e = true;
            // Allow optional sign after E
            if (i + 1 < s.size() && (s[i+1] == '+' || s[i+1] == '-')) ++i;
        } else if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        } else {
            has_digit = true;
        }
    }
    return has_digit && (has_dot || has_e);
}

// UTF-8 helpers for « (0xC2 0xAB) and » (0xC2 0xBB)
bool starts_with_laquo(const std::string& s, size_t pos) {
    return pos + 1 < s.size() &&
           static_cast<unsigned char>(s[pos]) == 0xC2 &&
           static_cast<unsigned char>(s[pos+1]) == 0xAB;
}

bool starts_with_raquo(const std::string& s, size_t pos) {
    return pos + 1 < s.size() &&
           static_cast<unsigned char>(s[pos]) == 0xC2 &&
           static_cast<unsigned char>(s[pos+1]) == 0xBB;
}

// ASCII helpers for << and >>
bool starts_with_ascii_open(const std::string& s, size_t pos) {
    return pos + 1 < s.size() && s[pos] == '<' && s[pos+1] == '<';
}

bool starts_with_ascii_close(const std::string& s, size_t pos) {
    return pos + 1 < s.size() && s[pos] == '>' && s[pos+1] == '>';
}

bool starts_with_prog_open(const std::string& s, size_t pos) {
    return starts_with_laquo(s, pos) || starts_with_ascii_open(s, pos);
}

bool starts_with_prog_close(const std::string& s, size_t pos) {
    return starts_with_raquo(s, pos) || starts_with_ascii_close(s, pos);
}

} // anonymous namespace

std::vector<Token> parse(const std::string& input) {
    std::vector<Token> tokens;
    size_t i = 0;
    size_t len = input.size();

    while (i < len) {
        // Skip whitespace
        while (i < len && is_whitespace(input[i])) ++i;
        if (i >= len) break;

        // Program literal: « ... » or << ... >>
        if (starts_with_prog_open(input, i)) {
            i += 2; // skip « or <<
            int nesting = 1;
            std::string body;
            while (i < len && nesting > 0) {
                if (starts_with_prog_open(input, i)) {
                    body += input[i];
                    body += input[i+1];
                    i += 2;
                    nesting++;
                } else if (starts_with_prog_close(input, i)) {
                    nesting--;
                    if (nesting > 0) {
                        body += input[i];
                        body += input[i+1];
                    }
                    i += 2;
                } else {
                    body += input[i];
                    ++i;
                }
            }
            // Trim leading/trailing whitespace from body
            size_t bs = body.find_first_not_of(" \t\n\r");
            size_t be = body.find_last_not_of(" \t\n\r");
            if (bs != std::string::npos) {
                body = body.substr(bs, be - bs + 1);
            } else {
                body.clear();
            }
            // Recursively parse body
            Program p;
            p.tokens = parse(body);
            tokens.push_back(Token::make_literal(std::move(p)));
            continue;
        }

        // String literal: "..."
        if (input[i] == '"') {
            ++i;
            std::string value;
            while (i < len && input[i] != '"') {
                if (input[i] == '\\' && i + 1 < len) {
                    ++i;
                    switch (input[i]) {
                        case 'n': value += '\n'; break;
                        case 't': value += '\t'; break;
                        case '"': value += '"'; break;
                        case '\\': value += '\\'; break;
                        default: value += input[i]; break;
                    }
                } else {
                    value += input[i];
                }
                ++i;
            }
            if (i < len) ++i; // skip closing "
            tokens.push_back(Token::make_literal(String{value}));
            continue;
        }

        // Quoted name or symbol: '...'
        if (input[i] == '\'') {
            ++i;
            std::string value;
            while (i < len && input[i] != '\'') {
                value += input[i];
                ++i;
            }
            if (i < len) ++i; // skip closing '
            // If it contains operators/spaces/parens/commas, it's a Symbol; otherwise a Name
            bool has_ops = false;
            for (char c : value) {
                if (c == '+' || c == '-' || c == '*' || c == '/' ||
                    c == '^' || c == '=' || c == ' ' || c == '(' ||
                    c == ')' || c == ',') {
                    has_ops = true;
                    break;
                }
            }
            if (has_ops) {
                tokens.push_back(Token::make_literal(Symbol{value}));
            } else {
                tokens.push_back(Token::make_literal(Name{value}));
            }
            continue;
        }

        // List literal: { ... }
        if (input[i] == '{') {
            ++i; // skip {
            int nesting = 1;
            std::string body;
            while (i < len && nesting > 0) {
                if (input[i] == '{') {
                    nesting++;
                    body += input[i];
                    ++i;
                } else if (input[i] == '}') {
                    nesting--;
                    if (nesting > 0) {
                        body += input[i];
                    }
                    ++i;
                } else {
                    body += input[i];
                    ++i;
                }
            }
            // Trim whitespace from body
            size_t bs = body.find_first_not_of(" \t\n\r");
            size_t be = body.find_last_not_of(" \t\n\r");
            if (bs != std::string::npos) {
                body = body.substr(bs, be - bs + 1);
            } else {
                body.clear();
            }
            // Parse body tokens and evaluate them into list items
            List list;
            if (!body.empty()) {
                auto inner_tokens = parse(body);
                for (auto& t : inner_tokens) {
                    if (t.kind == Token::Literal) {
                        list.items.push_back(std::move(t.literal));
                    } else {
                        // Bare word in list — try as number, otherwise treat as command name
                        // (commands in list literals are pushed as-is for later evaluation)
                        list.items.push_back(Name{t.command});
                    }
                }
            }
            tokens.push_back(Token::make_literal(std::move(list)));
            continue;
        }

        // Matrix literal: [[ ... ][ ... ] ... ]]
        if (input[i] == '[' && i + 1 < len && input[i + 1] == '[') {
            i += 2; // skip [[
            Matrix mat;
            std::vector<Object> current_row;
            std::string elem;

            auto flush_elem = [&]() {
                // Trim whitespace
                size_t a = elem.find_first_not_of(" \t\n\r");
                size_t b = elem.find_last_not_of(" \t\n\r");
                if (a != std::string::npos) {
                    std::string trimmed = elem.substr(a, b - a + 1);
                    auto elem_tokens = parse(trimmed);
                    if (elem_tokens.size() == 1 && elem_tokens[0].kind == Token::Literal) {
                        Object obj = std::move(elem_tokens[0].literal);
                        // Validate: only numeric and symbolic types allowed
                        if (std::holds_alternative<Integer>(obj) ||
                            std::holds_alternative<Real>(obj) ||
                            std::holds_alternative<Rational>(obj) ||
                            std::holds_alternative<Complex>(obj) ||
                            std::holds_alternative<Symbol>(obj) ||
                            std::holds_alternative<Name>(obj)) {
                            current_row.push_back(std::move(obj));
                        } else {
                            throw std::runtime_error("Invalid matrix element type");
                        }
                    } else if (elem_tokens.size() == 1 && elem_tokens[0].kind == Token::Command) {
                        // Bare name in matrix -> Name
                        current_row.push_back(Name{elem_tokens[0].command});
                    } else if (!elem_tokens.empty()) {
                        throw std::runtime_error("Invalid matrix element");
                    }
                }
                elem.clear();
            };

            while (i < len) {
                // Check for ][ (row separator)
                if (input[i] == ']' && i + 1 < len && input[i + 1] == '[') {
                    flush_elem();
                    if (!current_row.empty()) {
                        mat.rows.push_back(std::move(current_row));
                        current_row.clear();
                    }
                    i += 2;
                    continue;
                }
                // Check for ]] (matrix close)
                if (input[i] == ']' && i + 1 < len && input[i + 1] == ']') {
                    flush_elem();
                    if (!current_row.empty()) {
                        mat.rows.push_back(std::move(current_row));
                    }
                    i += 2;
                    break;
                }
                // Whitespace separates elements
                if (is_whitespace(input[i])) {
                    flush_elem();
                    ++i;
                    continue;
                }
                elem += input[i];
                ++i;
            }
            // Validate uniform row lengths
            if (!mat.rows.empty()) {
                size_t cols = mat.rows[0].size();
                for (size_t r = 1; r < mat.rows.size(); ++r) {
                    if (mat.rows[r].size() != cols) {
                        throw std::runtime_error("Matrix rows must have uniform length");
                    }
                }
            }
            tokens.push_back(Token::make_literal(std::move(mat)));
            continue;
        }

        // Complex literal: (re, im)
        if (input[i] == '(') {
            size_t start = i;
            // Find matching )
            size_t close = input.find(')', i);
            if (close != std::string::npos) {
                std::string inner = input.substr(i + 1, close - i - 1);
                auto comma = inner.find(',');
                if (comma != std::string::npos) {
                    std::string re_str = inner.substr(0, comma);
                    std::string im_str = inner.substr(comma + 1);
                    // Trim whitespace
                    auto trim = [](std::string& s) {
                        size_t a = s.find_first_not_of(" \t");
                        size_t b = s.find_last_not_of(" \t");
                        if (a != std::string::npos) s = s.substr(a, b - a + 1);
                    };
                    trim(re_str);
                    trim(im_str);
                    try {
                        Real re(re_str);
                        Real im(im_str);
                        tokens.push_back(Token::make_literal(Complex{re, im}));
                        i = close + 1;
                        continue;
                    } catch (...) {
                        // Not a valid complex literal, fall through to bare word
                    }
                }
            }
            // Fall through — treat as bare word
        }

        // Bare word or number
        size_t start = i;
        while (i < len && !is_whitespace(input[i]) &&
               !starts_with_prog_open(input, i) && !starts_with_prog_close(input, i) &&
               input[i] != '{' && input[i] != '}') {
            // Stop at [[ or ]] (matrix delimiters)
            if (input[i] == '[' && i + 1 < len && input[i + 1] == '[') break;
            if (input[i] == ']' && i + 1 < len && input[i + 1] == ']') break;
            if (input[i] == ']' && i + 1 < len && input[i + 1] == '[') break;
            ++i;
        }
        std::string word = input.substr(start, i - start);

        if (is_integer(word)) {
            tokens.push_back(Token::make_literal(Integer(word)));
        } else if (is_real(word)) {
            tokens.push_back(Token::make_literal(Real(word)));
        } else {
            tokens.push_back(Token::make_command(word));
        }
    }

    return tokens;
}

} // namespace lpr
