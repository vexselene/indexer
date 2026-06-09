#pragma once
// #define USE_HASHMAP
#ifdef USE_HASHMAP
#include "../include/file_name_index.h"
#endif
#include "../include/file_registery.h"
#include "../include/inverted_index.h"
#include "../include/prefix_tree.h"
#include "../include/tokenizer.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

struct FileChanges {
    std::vector<std::string> added; // paths
    std::vector<std::string> modified;
    std::vector<std::string> deleted;
};

class SearchEngine {
private:
    #ifdef USE_HASHMAP
    FilenameIndex fx;
    #endif
    FileRegistry fr;
    InvertedIndex IdX;
    PrefixTree pt;

    std::string indexed_path;
public:
    void build(const std::string& dir_path);
    std::vector<std::pair<int, int>> search(const std::string& query) const;
    void display(const std::vector<std::pair<int, int>>& results) const;
    
    #ifdef USE_HASHMAP
    const FilenameIndex& get_filename_index() const { return fx; }
    #endif
    
    const PrefixTree& get_prefix_tree() const { return pt; }
    const InvertedIndex& get_inverted_index() const { return IdX; }

    void save() const;
    bool load(const std::string& abs_path); // returns true if loaded from disk
    FileChanges has_changed(const std::string& abs_path) const;
    void update_changes(const FileChanges& changes);
};