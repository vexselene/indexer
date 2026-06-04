#pragma once
#include "../include/file_registery.h"
#include "../include/file_name_index.h"
#include "../include/inverted_index.h"
#include "../include/prefix_tree.h"
#include "../include/tokenizer.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

class SearchEngine {
private:
    FileRegistry fr;
    FilenameIndex fx;
    InvertedIndex IdX;
    PrefixTree pt;
public:
    void build(const std::string& dir_path);
    std::vector<std::pair<int, int>> search(const std::string& query) const;
    void display(const std::vector<std::pair<int, int>>& results) const;
};