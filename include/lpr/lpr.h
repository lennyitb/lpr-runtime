#ifndef LPR_H
#define LPR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lpr_ctx lpr_ctx;

typedef struct {
    int ok;
} lpr_result;

typedef struct {
    int undo_levels;
    int redo_levels;
} lpr_state;

lpr_ctx*   lpr_open(const char* db_path);
void       lpr_close(lpr_ctx* ctx);
lpr_result lpr_exec(lpr_ctx* ctx, const char* input);
int        lpr_depth(lpr_ctx* ctx);
char*      lpr_repr(lpr_ctx* ctx, int level);
int        lpr_undo(lpr_ctx* ctx);
int        lpr_redo(lpr_ctx* ctx);
lpr_state  lpr_get_state(lpr_ctx* ctx);
char*      lpr_get_setting(lpr_ctx* ctx, const char* key);
void       lpr_free(void* ptr);

#ifdef __cplusplus
}
#endif

#endif /* LPR_H */
