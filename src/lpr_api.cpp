#include "lpr/lpr.h"
#include "core/context.hpp"
#include <cstring>
#include <cstdlib>

struct lpr_ctx {
    lpr::Context context;
    explicit lpr_ctx(const char* db_path) : context(db_path) {}
};

extern "C" {

lpr_ctx* lpr_open(const char* db_path) {
    try {
        return new lpr_ctx(db_path);
    } catch (...) {
        return nullptr;
    }
}

void lpr_close(lpr_ctx* ctx) {
    delete ctx;
}

lpr_result lpr_exec(lpr_ctx* ctx, const char* input) {
    if (!ctx || !input) return {0};
    bool ok = ctx->context.exec(input);
    if (ok) {
        ctx->context.store().record_input(input);
    }
    return {ok ? 1 : 0};
}

int lpr_depth(lpr_ctx* ctx) {
    if (!ctx) return 0;
    return ctx->context.depth();
}

char* lpr_repr(lpr_ctx* ctx, int level) {
    if (!ctx) return nullptr;
    std::string s = ctx->context.repr_at(level);
    char* result = static_cast<char*>(std::malloc(s.size() + 1));
    if (result) {
        std::memcpy(result, s.c_str(), s.size() + 1);
    }
    return result;
}

int lpr_undo(lpr_ctx* ctx) {
    if (!ctx) return 0;
    return ctx->context.undo() ? 1 : 0;
}

int lpr_redo(lpr_ctx* ctx) {
    if (!ctx) return 0;
    return ctx->context.redo() ? 1 : 0;
}

lpr_state lpr_get_state(lpr_ctx* ctx) {
    if (!ctx) return {0, 0};
    auto& store = ctx->context.store();
    int cur = store.current_undo_seq();
    int max = store.history_max_seq();
    return {cur / 2, (max - cur) / 2};
}

char* lpr_get_setting(lpr_ctx* ctx, const char* key) {
    if (!ctx || !key) return nullptr;
    std::string val = ctx->context.store().get_meta(key, "");
    if (val.empty()) return nullptr;
    char* result = static_cast<char*>(std::malloc(val.size() + 1));
    if (result) {
        std::memcpy(result, val.c_str(), val.size() + 1);
    }
    return result;
}

char* lpr_path(lpr_ctx* ctx) {
    if (!ctx) return nullptr;
    auto& store = ctx->context.store();
    std::string path = store.dir_path(store.current_dir());
    // Format as HP 50g style: "HOME/DIR" → "{ HOME DIR }"
    std::string result = "{";
    std::string segment;
    for (char c : path) {
        if (c == '/') {
            if (!segment.empty()) {
                result += " " + segment;
                segment.clear();
            }
        } else {
            segment += c;
        }
    }
    if (!segment.empty()) {
        result += " " + segment;
    }
    result += " }";
    char* buf = static_cast<char*>(std::malloc(result.size() + 1));
    if (buf) std::memcpy(buf, result.c_str(), result.size() + 1);
    return buf;
}

char* lpr_dir_contents(lpr_ctx* ctx) {
    if (!ctx) return nullptr;
    auto& store = ctx->context.store();
    int dir = store.current_dir();
    auto vars = store.list_variables(dir);
    auto dirs = store.list_subdirectories(dir);
    std::string result;
    for (auto& v : vars) {
        if (!result.empty()) result += " ";
        result += v;
    }
    for (auto& d : dirs) {
        if (!result.empty()) result += " ";
        result += d + "/";
    }
    if (result.empty()) return nullptr;
    char* buf = static_cast<char*>(std::malloc(result.size() + 1));
    if (buf) std::memcpy(buf, result.c_str(), result.size() + 1);
    return buf;
}

int lpr_history_count(lpr_ctx* ctx) {
    if (!ctx) return 0;
    return ctx->context.store().input_history_count();
}

char* lpr_history_entry(lpr_ctx* ctx, int index) {
    if (!ctx || index < 0) return nullptr;
    std::string entry = ctx->context.store().input_history_entry(index);
    if (entry.empty()) return nullptr;
    char* result = static_cast<char*>(std::malloc(entry.size() + 1));
    if (result) {
        std::memcpy(result, entry.c_str(), entry.size() + 1);
    }
    return result;
}

void lpr_free(void* ptr) {
    std::free(ptr);
}

} // extern "C"
