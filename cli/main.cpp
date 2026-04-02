#include "lpr/lpr.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <linenoise.h>

// --- Non-interactive helpers (used by -e mode) ---

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

// --- Full-screen display rendering (FTXUI DOM only) ---

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

static std::string get_path(lpr_ctx* ctx) {
    char* p = lpr_path(ctx);
    std::string result = p ? p : "{ HOME }";
    lpr_free(p);
    return result;
}

static std::string get_modes(lpr_ctx* ctx) {
    std::string result;

    char* angle = lpr_get_setting(ctx, "angle_mode");
    result += angle ? angle : "RAD";
    lpr_free(angle);

    result += " ";
    char* coord = lpr_get_setting(ctx, "coordinate_mode");
    std::string cm = coord ? coord : "RECT";
    lpr_free(coord);
    if (cm == "SPHERICAL") cm = "SPHER";
    result += cm;

    result += " ";
    char* fmt = lpr_get_setting(ctx, "number_format");
    std::string nf = fmt ? fmt : "STD";
    lpr_free(fmt);
    if (nf != "STD") {
        char* dig = lpr_get_setting(ctx, "format_digits");
        if (dig) {
            nf += " " + std::string(dig);
            lpr_free(dig);
        }
    }
    result += nf;

    return result;
}

static std::string get_vars(lpr_ctx* ctx) {
    char* v = lpr_dir_contents(ctx);
    std::string result = v ? v : "(empty)";
    lpr_free(v);
    return result;
}

static void render_display(lpr_ctx* ctx, bool last_error) {
    auto term = Terminal::Size();
    int width = term.dimx;
    int height = term.dimy;

    std::string path = get_path(ctx);
    std::string modes = get_modes(ctx);
    std::string vars = get_vars(ctx);
    int depth = lpr_depth(ctx);

    // Reserve lines: status(1) + sep(1) + vars(1) + sep(1) + sep(1) + prompt(1) + newline(1) = 7
    int stack_lines = std::max(4, height - 7);

    Elements stack_elems;
    for (int level = stack_lines; level >= 1; --level) {
        std::string label = std::to_string(level) + ":";
        while (label.size() < 4) label = " " + label;

        if (level <= depth) {
            char* s = lpr_repr(ctx, level);
            std::string val = s ? s : "?";
            lpr_free(s);
            auto val_elem = text(val);
            if (level == 1 && last_error) {
                val_elem = val_elem | color(Color::Red);
            }
            stack_elems.push_back(
                hbox({text(label + "  "), filler(), val_elem})
            );
        } else {
            stack_elems.push_back(text(label));
        }
    }

    auto document = vbox({
        hbox({text(path), filler(), text(modes)}),
        separator(),
        hbox({text("VARS: "), text(vars) | dim}),
        separator(),
        vbox(stack_elems),
        separator(),
    });

    auto screen = Screen::Create(Dimension::Fixed(width),
                                 Dimension::Fit(document));
    Render(screen, document);

    // Cursor to top-left, print display
    std::cout << "\033[H\033[J";
    screen.Print();
    std::cout.flush();
}

// --- SIGWINCH self-pipe ---

static int sigwinch_pipe[2] = {-1, -1};

static void sigwinch_handler(int) {
    char c = 1;
    (void)write(sigwinch_pipe[1], &c, 1);
}

static void seed_linenoise_history(lpr_ctx* ctx) {
    int count = lpr_history_count(ctx);
    for (int i = count - 1; i >= 0; --i) {
        char* entry = lpr_history_entry(ctx, i);
        if (entry) {
            linenoiseHistoryAdd(entry);
            lpr_free(entry);
        }
    }
}

static void run_tui(lpr_ctx* ctx) {
    linenoiseSetMultiLine(0);
    linenoiseHistorySetMaxLen(1000);
    seed_linenoise_history(ctx);

    // Set up SIGWINCH self-pipe (non-blocking read end)
    pipe(sigwinch_pipe);
    fcntl(sigwinch_pipe[0], F_SETFL, O_NONBLOCK);
    struct sigaction sa = {};
    sa.sa_handler = sigwinch_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, nullptr);

    // Enter alternate screen buffer
    std::cout << "\033[?1049h";
    std::cout.flush();

    bool last_error = false;

    while (true) {
        render_display(ctx, last_error);

        // Start non-blocking line edit
        struct linenoiseState ls;
        char buf[1024];
        linenoiseEditStart(&ls, -1, -1, buf, sizeof(buf), "> ");

        char* line = nullptr;
        bool got_line = false;
        bool eof = false;

        while (!got_line && !eof) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(ls.ifd, &rfds);
            FD_SET(sigwinch_pipe[0], &rfds);
            int maxfd = std::max(ls.ifd, sigwinch_pipe[0]) + 1;

            int ret = select(maxfd, &rfds, nullptr, nullptr, nullptr);
            if (ret <= 0) continue;

            if (FD_ISSET(sigwinch_pipe[0], &rfds)) {
                // Drain the pipe
                char tmp;
                while (read(sigwinch_pipe[0], &tmp, 1) == 1) {}
                // Redraw on resize
                linenoiseHide(&ls);
                render_display(ctx, last_error);
                linenoiseShow(&ls);
            }

            if (FD_ISSET(ls.ifd, &rfds)) {
                line = linenoiseEditFeed(&ls);
                if (line == linenoiseEditMore) continue;
                if (line == nullptr) eof = true;
                else got_line = true;
            }
        }

        linenoiseEditStop(&ls);

        if (eof) break;

        std::string input(line);
        linenoiseFree(line);

        if (input.empty()) continue;
        if (input == "q" || input == "quit") break;

        if (input == "UNDO" || input == "undo") {
            lpr_undo(ctx);
            last_error = false;
            linenoiseHistoryAdd(input.c_str());
            continue;
        }
        if (input == "REDO" || input == "redo") {
            lpr_redo(ctx);
            last_error = false;
            linenoiseHistoryAdd(input.c_str());
            continue;
        }

        lpr_result r = lpr_exec(ctx, input.c_str());
        last_error = !r.ok;
        linenoiseHistoryAdd(input.c_str());
    }

    // Leave alternate screen buffer (restores original terminal content)
    std::cout << "\033[?1049l";
    std::cout.flush();

    // Clean up
    close(sigwinch_pipe[0]);
    close(sigwinch_pipe[1]);
    signal(SIGWINCH, SIG_DFL);
}

// --- Main ---

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

    // Interactive TUI mode
    run_tui(ctx);

    lpr_close(ctx);
    return 0;
}
