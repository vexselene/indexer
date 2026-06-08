#include "../include/search_engine.h"
#include "../include/file_registery.h"
#include "../include/file_name_index.h"
#include "../include/inverted_index.h"
#include "../include/tokenizer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>

// helper to read section
std::pair<std::string, std::string> read_section(std::ifstream& in) {
    std::string tag(8, '\0');
    in.read(tag.data(), 8);

    int size;
    in.read(reinterpret_cast<char*>(&size), sizeof(size));
    
    std::string data(size, '\0');
    in.read(data.data(), size);

    return {tag, data};
}

void SearchEngine::build(const std::string& dir_path) {
    // resolve dir_path -> absolute path
    namespace fs =  std::filesystem;
    std::string abs_path = fs::absolute(dir_path).string();
    indexed_path = abs_path;
    if(load(abs_path)) {
        std::cout << "Loaded existing Index\n";
        return;
    }

    // build if saved index not loaded
    fr.index_directory(abs_path);
    
    std::vector<std::pair<int, std::string>> files = fr.get_filenames();

    //index file names
    for(const auto& [f_id, f_name] : files) {
        #ifdef USE_HASHMAP
        fx.index_file_name(f_id, f_name);
        #endif
        pt.index_file_name(f_id, f_name);
    }

    // create inverted index for file content
    for(const auto& [f_id, f_name] : files) {
        FileMetaData meta = fr.get_file(f_id);
        if(!meta.index_content) continue; // skip file for inverted index if not a text file 
        std::string file_path = fr.get_file(f_id).path; // use absolute path stored in the File meta data
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

std::vector<std::pair<int, int>> SearchEngine::search(const std::string& query) const {
    std::vector<std::string> tokens = tokenize(query, false);

    // build score map (O(1) lookup)
    std::unordered_map<int, int> score_map;
    
    for(const auto& token : tokens) {
        #ifdef USE_HASHMAP
        std::unordered_set<int> f_name_ids = fx.search(token); 
        #else
        std::unordered_set<int> f_name_ids = pt.search(token); //exact search
        #endif
        std::unordered_set<int> pref_f_name_ids = pt.search_matching(token); // prefix search
        std::unordered_map<int, int> f_content_ids = IdX.search(token);
        
        for(int f_id : f_name_ids) {
            score_map[f_id] += 10;
            pref_f_name_ids.erase(f_id);
        }
        for(int f_id : pref_f_name_ids) score_map[f_id] += 5;
        for(const auto& [f_id, freq] : f_content_ids) score_map[f_id] += 3*freq;
    }
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
        std::cout << "--------x-----------x--------\n";
        return;
    }
    for(const std::pair<int, int>& result : results) {
        FileMetaData f_metadata = fr.get_file(result.first);
        std::cout << std::endl;
        std::cout << "File Id  : " << f_metadata.id << std::endl;
        std::cout << "File Name: " << f_metadata.name << std::endl;
        std::cout << "File Path: " << f_metadata.path << std::endl;
    }
    std::cout << "--------x-----------x--------\n";
}

void SearchEngine::save() const {
    std::ofstream db(indexed_path + "/.indxr_db", std::ios::binary);

    // store the absolute path of the directory the index_.bin is for
    db.write("METADATA", 8);
    int size = static_cast<int>(indexed_path.size());
    db.write(reinterpret_cast<const char*>(&size), sizeof(size));
    db.write(indexed_path.data(), size);
    
    std::stringstream registry_buf;
    std::stringstream prefix_buf;
    std::stringstream inverted_buf;

    fr.serialize(registry_buf);
    pt.serialize(prefix_buf);
    IdX.serialize(inverted_buf);

    std::string r = registry_buf.str();
    std::string p = prefix_buf.str();
    std::string i = inverted_buf.str();

    db.write("REGISTRY", 8);
    size = static_cast<int>(r.size());
    db.write(reinterpret_cast<const char*>(&size), sizeof(size));
    db.write(r.data(), size);
    db.write("PREFIX__", 8);
    
    size = static_cast<int>(p.size());
    db.write(reinterpret_cast<const char*>(&size), sizeof(size));
    db.write(p.data(), size);
    
    db.write("INVERTED", 8);
    size = static_cast<int>(i.size());
    db.write(reinterpret_cast<const char*>(&size), sizeof(size));
    db.write(i.data(), size);

    std::cout << "Index saved" << std::endl;
}

bool SearchEngine::load(const std::string& abs_path) {
    namespace fs = std::filesystem;
    if(!fs::exists(abs_path + "/.indxr_db")) return false;
    
    std::ifstream db(abs_path + "/.indxr_db", std::ios::binary); // open in binary mode
    if(!db) throw std::runtime_error("Could not open .indxr_db");
    
    std::pair<std::string, std::string> section = read_section(db);
    if(section.first != "METADATA") return false;
    else if(section.second != abs_path) {
        std::cout << "Indexed directory changed -- rebuilding Index\n";
        return false;
    }

    // registry section
    section = read_section(db);
    if(section.first != "REGISTRY") {
        std::cout << ".indxr_db is corrupted ..... rebuilding Index\n";
        return false;
    }
    std::stringstream registry_stream(section.second);
    
    // prefix section
    section = read_section(db);
    if(section.first != "PREFIX__") {
        std::cout << ".indxr_db is corrupted ..... rebuilding Index\n";
        return false;
    }
    std::stringstream prefix_stream(section.second);
    
    //inverted index section
    section = read_section(db);
    if(section.first != "INVERTED") {
        std::cout << ".indxr_db is corrupted ..... rebuilding Index\n";
        return false;
    }
    std::stringstream inverted_stream(section.second);

    fr.deserialize(registry_stream);
    pt.deserialize(prefix_stream);
    IdX.deserialize(inverted_stream);

    return true;
}