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

void lpr_free(void* ptr) {
    std::free(ptr);
}

} // extern "C"
