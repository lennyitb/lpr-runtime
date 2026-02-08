#include "lpr/lpr.h"
#include <iostream>
#include <string>

static void display_stack(lpr_ctx* ctx) {
    int d = lpr_depth(ctx);
    if (d == 0) return;
    for (int level = d; level >= 1; --level) {
        char* s = lpr_repr(ctx, level);
        std::cout << level << ": " << (s ? s : "?") << "\n";
        lpr_free(s);
    }
}

int main(int argc, char* argv[]) {
    const char* db_path = nullptr;
    if (argc > 1) {
        db_path = argv[1];
    }

    lpr_ctx* ctx = lpr_open(db_path);
    if (!ctx) {
        std::cerr << "Failed to open database\n";
        return 1;
    }

    std::string line;
    std::cout << "LPR Runtime v0.1.0\n";
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;

        if (line == "q" || line == "quit") break;

        lpr_result r = lpr_exec(ctx, line.c_str());
        if (!r.ok) {
            // The error is on the stack — display it distinctly
            int d = lpr_depth(ctx);
            if (d > 0) {
                char* s = lpr_repr(ctx, 1);
                if (s) {
                    std::cerr << "** " << s << "\n";
                    lpr_free(s);
                }
            }
        }

        display_stack(ctx);
    }

    lpr_close(ctx);
    return 0;
}
