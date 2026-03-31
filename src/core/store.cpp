#include "core/store.hpp"
#include <sqlite3.h>
#include <stdexcept>
#include <cstring>

namespace lpr {

Store::Store(const char* db_path) {
    const char* path = db_path ? db_path : ":memory:";
    int rc = sqlite3_open(path, &db_);
    if (rc != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("Failed to open database: " + err);
    }
    exec_sql("PRAGMA journal_mode=WAL;");
    exec_sql("PRAGMA foreign_keys=ON;");
    create_schema();
    ensure_home();
}

Store::~Store() {
    if (db_) sqlite3_close(db_);
}

void Store::exec_sql(const char* sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw std::runtime_error("SQL error: " + msg);
    }
}

void Store::create_schema() {
    exec_sql(R"(
        CREATE TABLE IF NOT EXISTS objects (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            type_tag INTEGER NOT NULL,
            data TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS stack (
            pos INTEGER PRIMARY KEY,
            object_id INTEGER NOT NULL REFERENCES objects(id)
        );
        CREATE TABLE IF NOT EXISTS directories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            parent_id INTEGER REFERENCES directories(id),
            name TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS variables (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            dir_id INTEGER NOT NULL REFERENCES directories(id),
            name TEXT NOT NULL,
            object_id INTEGER NOT NULL REFERENCES objects(id),
            UNIQUE(dir_id, name)
        );
        CREATE TABLE IF NOT EXISTS history (
            seq INTEGER NOT NULL,
            pos INTEGER NOT NULL,
            object_id INTEGER NOT NULL REFERENCES objects(id),
            PRIMARY KEY(seq, pos)
        );
        CREATE TABLE IF NOT EXISTS history_seqs (
            seq INTEGER PRIMARY KEY
        );
        CREATE TABLE IF NOT EXISTS meta (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS stash (
            group_id INTEGER NOT NULL,
            pos INTEGER NOT NULL,
            object_id INTEGER NOT NULL REFERENCES objects(id),
            PRIMARY KEY(group_id, pos)
        );
        CREATE TABLE IF NOT EXISTS stash_history (
            seq INTEGER NOT NULL,
            group_id INTEGER NOT NULL,
            pos INTEGER NOT NULL,
            object_id INTEGER NOT NULL REFERENCES objects(id),
            PRIMARY KEY(seq, group_id, pos)
        );
        CREATE TABLE IF NOT EXISTS flags (
            name TEXT PRIMARY KEY,
            type_tag INTEGER NOT NULL,
            value TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS input_history (
            seq INTEGER PRIMARY KEY AUTOINCREMENT,
            input TEXT NOT NULL
        );
    )");
}

void Store::ensure_home() {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT id FROM directories WHERE parent_id IS NULL AND name='HOME'", -1, &stmt, nullptr);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        exec_sql("INSERT INTO directories (parent_id, name) VALUES (NULL, 'HOME')");
        // Set current dir to HOME
        int home = static_cast<int>(sqlite3_last_insert_rowid(db_));
        sqlite3_prepare_v2(db_, "INSERT OR REPLACE INTO meta (key, value) VALUES ('current_dir', ?)", -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, std::to_string(home).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        // Initialize undo_seq
        sqlite3_prepare_v2(db_, "INSERT OR REPLACE INTO meta (key, value) VALUES ('undo_seq', '0')", -1, &stmt, nullptr);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    } else {
        sqlite3_finalize(stmt);
    }
}

// --- Stack operations ---

void Store::push(const Object& obj) {
    // Insert the object
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "INSERT INTO objects (type_tag, data) VALUES (?, ?)", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, static_cast<int>(type_tag(obj)));
    std::string data = serialize(obj);
    sqlite3_bind_text(stmt, 2, data.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    int obj_id = static_cast<int>(sqlite3_last_insert_rowid(db_));

    // Push onto stack at next position
    int d = depth();
    sqlite3_prepare_v2(db_, "INSERT INTO stack (pos, object_id) VALUES (?, ?)", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, d + 1);
    sqlite3_bind_int(stmt, 2, obj_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

Object Store::pop() {
    int d = depth();
    if (d == 0) {
        return Error{1, "Stack underflow"};
    }

    // Get the top item
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_,
        "SELECT o.type_tag, o.data FROM stack s JOIN objects o ON s.object_id = o.id WHERE s.pos = ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, d);
    Object result = Error{2, "Stack read error"};
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auto tag = static_cast<TypeTag>(sqlite3_column_int(stmt, 0));
        const char* data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result = deserialize(tag, data ? data : "");
    }
    sqlite3_finalize(stmt);

    // Remove from stack
    sqlite3_prepare_v2(db_, "DELETE FROM stack WHERE pos = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, d);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return result;
}

Object Store::peek(int level) {
    int d = depth();
    if (level < 1 || level > d) {
        return Error{1, "Invalid stack level"};
    }
    // level 1 = top = pos d, level 2 = pos d-1, etc.
    int pos = d - level + 1;

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_,
        "SELECT o.type_tag, o.data FROM stack s JOIN objects o ON s.object_id = o.id WHERE s.pos = ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, pos);
    Object result = Error{2, "Stack read error"};
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auto tag = static_cast<TypeTag>(sqlite3_column_int(stmt, 0));
        const char* data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result = deserialize(tag, data ? data : "");
    }
    sqlite3_finalize(stmt);
    return result;
}

int Store::depth() {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM stack", -1, &stmt, nullptr);
    int d = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        d = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return d;
}

void Store::clear_stack() {
    exec_sql("DELETE FROM stack");
}

// --- History ---

int Store::snapshot_stack() {
    int seq = history_max_seq() + 1;

    // Record that this seq exists (even if stack is empty)
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "INSERT INTO history_seqs (seq) VALUES (?)", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, seq);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Copy stack rows into history
    sqlite3_prepare_v2(db_,
        "INSERT INTO history (seq, pos, object_id) SELECT ?, pos, object_id FROM stack",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, seq);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Copy stash rows into stash_history
    sqlite3_prepare_v2(db_,
        "INSERT INTO stash_history (seq, group_id, pos, object_id) SELECT ?, group_id, pos, object_id FROM stash",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, seq);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    set_undo_seq(seq);
    return seq;
}

bool Store::restore_stack(int seq) {
    // For seq 0 (initial empty state), we just clear the stack and stash
    if (seq == 0) {
        clear_stack();
        stash_clear();
        set_undo_seq(0);
        return true;
    }

    // Check if this snapshot exists via history_seqs
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM history_seqs WHERE seq = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, seq);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    if (count == 0) return false;

    // Clear current stack and restore from snapshot
    clear_stack();
    sqlite3_prepare_v2(db_,
        "INSERT INTO stack (pos, object_id) SELECT pos, object_id FROM history WHERE seq = ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, seq);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Clear current stash and restore from snapshot
    stash_clear();
    sqlite3_prepare_v2(db_,
        "INSERT INTO stash (group_id, pos, object_id) SELECT group_id, pos, object_id FROM stash_history WHERE seq = ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, seq);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    set_undo_seq(seq);
    return true;
}

int Store::history_max_seq() {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT COALESCE(MAX(seq), 0) FROM history_seqs", -1, &stmt, nullptr);
    int seq = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        seq = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return seq;
}

int Store::current_undo_seq() {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT value FROM meta WHERE key = 'undo_seq'", -1, &stmt, nullptr);
    int seq = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        seq = std::stoi(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    return seq;
}

void Store::set_undo_seq(int seq) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "INSERT OR REPLACE INTO meta (key, value) VALUES ('undo_seq', ?)", -1, &stmt, nullptr);
    std::string s = std::to_string(seq);
    sqlite3_bind_text(stmt, 1, s.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// --- Variables ---

void Store::store_variable(int dir_id, const std::string& name, const Object& obj) {
    // Insert object
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "INSERT INTO objects (type_tag, data) VALUES (?, ?)", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, static_cast<int>(type_tag(obj)));
    std::string data = serialize(obj);
    sqlite3_bind_text(stmt, 2, data.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    int obj_id = static_cast<int>(sqlite3_last_insert_rowid(db_));

    // Upsert variable
    sqlite3_prepare_v2(db_,
        "INSERT INTO variables (dir_id, name, object_id) VALUES (?, ?, ?) "
        "ON CONFLICT(dir_id, name) DO UPDATE SET object_id = excluded.object_id",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, dir_id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, obj_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

Object Store::recall_variable(int dir_id, const std::string& name) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_,
        "SELECT o.type_tag, o.data FROM variables v JOIN objects o ON v.object_id = o.id "
        "WHERE v.dir_id = ? AND v.name = ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, dir_id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);

    Object result = Error{3, "Undefined Name"};
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auto tag = static_cast<TypeTag>(sqlite3_column_int(stmt, 0));
        const char* data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result = deserialize(tag, data ? data : "");
    }
    sqlite3_finalize(stmt);
    return result;
}

bool Store::purge_variable(int dir_id, const std::string& name) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "DELETE FROM variables WHERE dir_id = ? AND name = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, dir_id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return sqlite3_changes(db_) > 0;
}

std::vector<std::string> Store::list_variables(int dir_id) {
    std::vector<std::string> result;
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT name FROM variables WHERE dir_id = ? ORDER BY name", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, dir_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    return result;
}

// --- Directories ---

int Store::home_dir_id() {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT id FROM directories WHERE parent_id IS NULL AND name='HOME'", -1, &stmt, nullptr);
    int id = 1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return id;
}

int Store::create_directory(int parent_id, const std::string& name) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "INSERT INTO directories (parent_id, name) VALUES (?, ?)", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, parent_id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return static_cast<int>(sqlite3_last_insert_rowid(db_));
}

int Store::find_directory(int parent_id, const std::string& name) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT id FROM directories WHERE parent_id = ? AND name = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, parent_id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return id;
}

int Store::parent_dir_id(int dir_id) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT parent_id FROM directories WHERE id = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, dir_id);
    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
        id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return id;
}

std::string Store::dir_name(int dir_id) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT name FROM directories WHERE id = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, dir_id);
    std::string name;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return name;
}

std::string Store::dir_path(int dir_id) {
    std::vector<std::string> parts;
    int cur = dir_id;
    while (cur != -1) {
        parts.push_back(dir_name(cur));
        cur = parent_dir_id(cur);
    }
    std::string path;
    for (int i = static_cast<int>(parts.size()) - 1; i >= 0; --i) {
        if (!path.empty()) path += "/";
        path += parts[i];
    }
    return path;
}

std::vector<std::string> Store::list_subdirectories(int dir_id) {
    std::vector<std::string> result;
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT name FROM directories WHERE parent_id = ? ORDER BY name", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, dir_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    return result;
}

void Store::purge_directory_tree(int dir_id) {
    // Recursively purge child directories first
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT id FROM directories WHERE parent_id = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, dir_id);
    std::vector<int> children;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        children.push_back(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);

    for (int child : children) {
        purge_directory_tree(child);
    }

    // Delete variables in this directory
    sqlite3_stmt* del_vars = nullptr;
    sqlite3_prepare_v2(db_, "DELETE FROM variables WHERE dir_id = ?", -1, &del_vars, nullptr);
    sqlite3_bind_int(del_vars, 1, dir_id);
    sqlite3_step(del_vars);
    sqlite3_finalize(del_vars);

    // Delete the directory itself
    sqlite3_stmt* del_dir = nullptr;
    sqlite3_prepare_v2(db_, "DELETE FROM directories WHERE id = ?", -1, &del_dir, nullptr);
    sqlite3_bind_int(del_dir, 1, dir_id);
    sqlite3_step(del_dir);
    sqlite3_finalize(del_dir);
}

// --- Transactions ---

void Store::begin() { exec_sql("BEGIN"); }
void Store::commit() { exec_sql("COMMIT"); }
void Store::rollback() { exec_sql("ROLLBACK"); }

// --- Current directory ---

int Store::current_dir() {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT value FROM meta WHERE key = 'current_dir'", -1, &stmt, nullptr);
    int id = home_dir_id();
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        id = std::stoi(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    return id;
}

void Store::set_current_dir(int dir_id) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "INSERT OR REPLACE INTO meta (key, value) VALUES ('current_dir', ?)", -1, &stmt, nullptr);
    std::string s = std::to_string(dir_id);
    sqlite3_bind_text(stmt, 1, s.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// --- Generic meta access ---

std::string Store::get_meta(const std::string& key, const std::string& default_val) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT value FROM meta WHERE key = ?", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    std::string result = default_val;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (val) result = val;
    }
    sqlite3_finalize(stmt);
    return result;
}

void Store::set_meta(const std::string& key, const std::string& value) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "INSERT OR REPLACE INTO meta (key, value) VALUES (?, ?)", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// --- Stash ---

void Store::stash_push(const std::vector<Object>& group) {
    // Next group_id = max existing + 1
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT COALESCE(MAX(group_id), 0) FROM stash", -1, &stmt, nullptr);
    int gid = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        gid = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    gid += 1;

    for (int i = 0; i < static_cast<int>(group.size()); ++i) {
        // Insert object
        sqlite3_prepare_v2(db_, "INSERT INTO objects (type_tag, data) VALUES (?, ?)", -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, static_cast<int>(type_tag(group[i])));
        std::string data = serialize(group[i]);
        sqlite3_bind_text(stmt, 2, data.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        int obj_id = static_cast<int>(sqlite3_last_insert_rowid(db_));

        // Insert stash row
        sqlite3_prepare_v2(db_, "INSERT INTO stash (group_id, pos, object_id) VALUES (?, ?, ?)", -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, gid);
        sqlite3_bind_int(stmt, 2, i);
        sqlite3_bind_int(stmt, 3, obj_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

std::vector<Object> Store::stash_pop() {
    // Find the highest group_id
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT COALESCE(MAX(group_id), 0) FROM stash", -1, &stmt, nullptr);
    int gid = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        gid = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (gid == 0) {
        throw std::runtime_error("Stash is empty");
    }

    // Read all items in the group, ordered by pos
    std::vector<Object> group;
    sqlite3_prepare_v2(db_,
        "SELECT o.type_tag, o.data FROM stash s JOIN objects o ON s.object_id = o.id "
        "WHERE s.group_id = ? ORDER BY s.pos",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, gid);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        auto tag = static_cast<TypeTag>(sqlite3_column_int(stmt, 0));
        const char* data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        group.push_back(deserialize(tag, data ? data : ""));
    }
    sqlite3_finalize(stmt);

    // Delete the group
    sqlite3_prepare_v2(db_, "DELETE FROM stash WHERE group_id = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, gid);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return group;
}

int Store::stash_depth() {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT COUNT(DISTINCT group_id) FROM stash", -1, &stmt, nullptr);
    int d = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        d = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return d;
}

void Store::stash_clear() {
    exec_sql("DELETE FROM stash");
}

// --- Flags ---

void Store::set_flag(const std::string& name, int type_tag, const std::string& value) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_,
        "INSERT OR REPLACE INTO flags (name, type_tag, value) VALUES (?, ?, ?)",
        -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, type_tag);
    sqlite3_bind_text(stmt, 3, value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::optional<std::tuple<int, std::string>> Store::get_flag(const std::string& name) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT type_tag, value FROM flags WHERE name = ?", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<std::tuple<int, std::string>> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int tag = sqlite3_column_int(stmt, 0);
        const char* val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result = std::make_tuple(tag, std::string(val ? val : ""));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<std::tuple<std::string, int, std::string>> Store::all_flags() {
    std::vector<std::tuple<std::string, int, std::string>> result;
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT name, type_tag, value FROM flags ORDER BY name", -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int tag = sqlite3_column_int(stmt, 1);
        const char* val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        result.emplace_back(name ? name : "", tag, val ? val : "");
    }
    sqlite3_finalize(stmt);
    return result;
}

void Store::clear_all_flags() {
    exec_sql("DELETE FROM flags");
}

// --- Input History ---

void Store::record_input(const std::string& input) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "INSERT INTO input_history (input) VALUES (?)", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, input.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

int Store::input_history_count() {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM input_history", -1, &stmt, nullptr);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

std::string Store::input_history_entry(int index) {
    // index 0 = most recent, so ORDER BY seq DESC LIMIT 1 OFFSET index
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_,
        "SELECT input FROM input_history ORDER BY seq DESC LIMIT 1 OFFSET ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, index);
    std::string result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (val) result = val;
    }
    sqlite3_finalize(stmt);
    return result;
}

} // namespace lpr
