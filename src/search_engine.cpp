#include "../include/search_engine.h"
#include "../include/file_registery.h"
#include "../include/file_name_index.h"
#include "../include/inverted_index.h"
#include "../include/tokenizer.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <algorithm>
#include <fstream>
#include <iostream>

void SearchEngine::build(const std::string& dir_path) {
    fr.index_directory(dir_path);
    
    std::vector<std::pair<int, std::string>> files = fr.get_filenames();

    //index file names
    for(const auto& [f_id, f_name] : files) {
        fx.index_file_name(f_id, f_name);
    }

    // create inverted index for file content
    for(const auto& [f_id, f_name] : files) {
        std::string file_path = dir_path + "/" + f_name;
        std::ifstream file(file_path);
        if(!file.is_open()) {
            std::cerr << "Warning could not open file: " << file_path << std::endl;
            continue;
        } 

        std::string line;
        while(std::getline(file, line)) {
            IdX.index_file_content(f_id, line);
        }
    }
}

std::vector<std::pair<int, int>> SearchEngine::search(const std::string& token) const {
    std::unordered_set<int> f_name_ids = fx.search(token);
    std::unordered_map<int, int> f_content_ids = IdX.search(token);

    // build score map (O(1) lookup)
    std::unordered_map<int, int> score_map;
    for(int f_id : f_name_ids) score_map[f_id] += 10;
    for(const auto& [f_id, freq] : f_content_ids) score_map[f_id] += 3*freq;

    std::vector<std::pair<int, int>> results;
    for(const auto& [f_id, score] : score_map) results.emplace_back(f_id, score); 

    std::sort(results.begin(), results.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
        if(a.second != b.second)
            return a.second > b.second;
        return a.first < b.first;
    });

    return results;
}

void SearchEngine::display(const std::vector<std::pair<int, int>>& results) const {
    std::cout << "--------Query results--------\n";
    if(results.empty()) {
        std::cout << "Nothing found!\n";
        return;
    }
    for(const std::pair<int, int>& result : results) {
        FileMetaData f_metadata = fr.get_file(result.first);
        std::cout << std::endl;
        std::cout << "File Id  : " << f_metadata.id << std::endl;
        std::cout << "File Name: " << f_metadata.name << std::endl;
        std::cout << "File Path: " << f_metadata.path << std::endl;
    }
}