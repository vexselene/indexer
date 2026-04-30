#include "../include/inverted_index.h"
#include "../include/tokenizer.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include <cerrno>

void InvertedIndex::index_file_content(int id, const std::string& content) {
    auto tokens = tokenize(content, true);
    for (std::string token : tokens) {
        index[token][id]++;
    }
}

const std::unordered_map<int, int>& InvertedIndex::search(const std::string& token) const {
    static const std::unordered_map<int, int> empty_map;
    auto it = index.find(token);
    if (it != index.end()) return it->second;
    else return empty_map;
}

void InvertedIndex::list_all() const {
    for (const auto&  [token, idx] : index) {
        bool first = true;
        std::cout << token << ": { ";
        for (const auto& [id, freq] : idx) {
            if (!first) std::cout << ", ";
            std::cout << id << " : " << freq;
            first = false;
        }
        std::cout << " }\n";
    }
}