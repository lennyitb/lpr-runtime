#pragma once

#include "core/object.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <tuple>

struct sqlite3;
struct sqlite3_stmt;

namespace lpr {

class Store {
public:
    explicit Store(const char* db_path); // nullptr for in-memory
    ~Store();

    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;

    // Stack operations
    void   push(const Object& obj);
    Object pop();
    Object peek(int level); // 1-based, 1 = top
    int    depth();
    void   clear_stack();

    // History (undo/redo)
    int  snapshot_stack();          // returns sequence number
    bool restore_stack(int seq);

    // Stash (auxiliary hidden stack, LIFO of groups)
    void stash_push(const std::vector<Object>& group);
    std::vector<Object> stash_pop();
    int  stash_depth();  // number of groups
    void stash_clear();

    // Variables
    void   store_variable(int dir_id, const std::string& name, const Object& obj);
    Object recall_variable(int dir_id, const std::string& name);
    bool   purge_variable(int dir_id, const std::string& name);
    std::vector<std::string> list_variables(int dir_id);

    // Directories
    int  home_dir_id();
    int  create_directory(int parent_id, const std::string& name);
    int  find_directory(int parent_id, const std::string& name);
    int  parent_dir_id(int dir_id);
    std::string dir_name(int dir_id);
    std::string dir_path(int dir_id);
    std::vector<std::string> list_subdirectories(int dir_id);
    void purge_directory_tree(int dir_id);

    // Transaction helpers
    void begin();
    void commit();
    void rollback();

    // Current directory tracking (stored in meta table)
    int  current_dir();
    void set_current_dir(int dir_id);

    // History navigation state
    int  history_max_seq();
    int  current_undo_seq();
    void set_undo_seq(int seq);

    // Generic meta table access
    std::string get_meta(const std::string& key, const std::string& default_val = "");
    void set_meta(const std::string& key, const std::string& value);

    // Flag registry
    void set_flag(const std::string& name, int type_tag, const std::string& value);
    std::optional<std::tuple<int, std::string>> get_flag(const std::string& name);
    std::vector<std::tuple<std::string, int, std::string>> all_flags();
    void clear_all_flags();

    // Input history
    void record_input(const std::string& input);
    int  input_history_count();
    std::string input_history_entry(int index); // 0 = most recent; empty if out of range

private:
    sqlite3* db_ = nullptr;

    void exec_sql(const char* sql);
    void create_schema();
    void ensure_home();
};

} // namespace lpr
