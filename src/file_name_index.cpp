#include "../include/tokenizer.h"
#include "../include/file_registery.h"
#include "../include/file_name_index.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <unordered_set>
#include <iostream>

void FilenameIndex::add_file(int id, const std::string& file_name) {
    std::vector<std::string> tokens = tokenize(file_name, true);
    for(const auto& token : tokens) 
        index[token].insert(id);
}

const std::unordered_set<int>& FilenameIndex::search(const std::string& token) const {
    static const std::unordered_set<int> empty_set;
    auto it = index.find(token);
    if(it != index.end()) return it->second;
    return empty_set;
}

void FilenameIndex::list_all() const {
    for (const auto& [name, id_set] : index) {
        std::cout << name << ": { ";
        bool first = true;
        for(int id : id_set) {
            if (!first) std::cout << ", ";
            std::cout << id;
            first = false;
        }
        std::cout << " }" << std::endl;
    }
}

