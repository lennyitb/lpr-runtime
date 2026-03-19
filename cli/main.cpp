#include "lpr/lpr.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

static void display_stack(lpr_ctx* ctx) {
    int d = lpr_depth(ctx);
    if (d == 0) return;
    for (int level = d; level >= 1; --level) {
        char* s = lpr_repr(ctx, level);
        std::cout << level << ": " << (s ? s : "?") << "\n";
        lpr_free(s);
    }
}

static void display_error(lpr_ctx* ctx) {
    int d = lpr_depth(ctx);
    if (d > 0) {
        char* s = lpr_repr(ctx, 1);
        if (s) {
            std::cerr << "** " << s << "\n";
            lpr_free(s);
        }
    }
}

static void print_usage() {
    std::cout << "Usage: lpr-cli [-e expression]... [database.lpr]\n"
              << "  -e expr   Execute expression and exit\n"
              << "  -h        Show this help\n";
}

int main(int argc, char* argv[]) {
    const char* db_path = nullptr;
    std::vector<const char*> expressions;

    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-e") == 0) {
            if (i + 1 < argc) {
                expressions.push_back(argv[++i]);
            } else {
                std::cerr << "Option -e requires an argument\n";
                print_usage();
                return 1;
            }
        } else if (std::strcmp(argv[i], "-h") == 0) {
            print_usage();
            return 0;
        } else if (argv[i][0] == '-') {
            std::cerr << "Unknown option: " << argv[i] << "\n";
            print_usage();
            return 1;
        } else {
            db_path = argv[i];
        }
    }

    lpr_ctx* ctx = lpr_open(db_path);
    if (!ctx) {
        std::cerr << "Failed to open database\n";
        return 1;
    }

    // Non-interactive mode: execute -e expressions and exit
    if (!expressions.empty()) {
        int exit_code = 0;
        for (const char* expr : expressions) {
            lpr_result r = lpr_exec(ctx, expr);
            if (!r.ok) {
                display_error(ctx);
                exit_code = 1;
                break;
            }
        }
        if (exit_code == 0) {
            display_stack(ctx);
        }
        lpr_close(ctx);
        return exit_code;
    }

    // Interactive REPL mode
    std::string line;
    std::cout << "LPR Runtime v0.1.0\n";
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;

        if (line == "q" || line == "quit") break;

        if (line == "UNDO" || line == "undo") {
            if (!lpr_undo(ctx)) std::cerr << "** Nothing to undo\n";
            display_stack(ctx);
            continue;
        }
        if (line == "REDO" || line == "redo") {
            if (!lpr_redo(ctx)) std::cerr << "** Nothing to redo\n";
            display_stack(ctx);
            continue;
        }

        lpr_result r = lpr_exec(ctx, line.c_str());
        if (!r.ok) {
            display_error(ctx);
        }

        display_stack(ctx);
    }

    lpr_close(ctx);
    return 0;
}
