#include "../include/search_engine.h"
#include "../include/file_registery.h"
#include "../include/inverted_index.h"
#include "../include/tokenizer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>  // std::log
#include <utility>
#include <algorithm>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>

#include <mutex>
#include <shared_mutex>

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
    std::unique_lock lock(mtx);
    // resolve dir_path -> absolute path
    namespace fs =  std::filesystem;
    std::string abs_path = fs::absolute(dir_path).string();
    indexed_path = abs_path;
    if(load(abs_path)) {
        std::cout << "Loaded existing Index\n";
        FileChanges changes = has_changed(abs_path);
        for(const auto& path : changes.added) {
            std::cout << "Added: " << path << std::endl;
        }
        for(const auto& path : changes.modified) {
            std::cout << "Modified: " << path << std::endl;
        }
        for(const auto& path : changes.deleted) {
            std::cout << "Deleted: " << path << std::endl;
        }

        update_changes(changes);
        return;
    }

    // build if saved index not loaded
    fr.index_directory(abs_path);
    
    std::vector<std::pair<int, std::string>> files = fr.get_filenames();

    //index file names
    for(const auto& [f_id, f_name] : files) {
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
    std::shared_lock lock(mtx);
    std::vector<std::string> tokens = tokenize(query, false);

    // build score map (O(1) lookup)
    std::unordered_map<int, int> score_map;
    int total_files = fr.get_registry().size(); // for TF-IDF
    for(const auto& token : tokens) {
        std::unordered_set<int> f_name_ids = pt.search(token); //exact search
        std::unordered_set<int> pref_f_name_ids = pt.search_matching(token); // prefix search
        std::unordered_map<int, int> f_content_ids = IdX.search(token);
        int df = IdX.document_frequency(token);
        double idf = (df > 0) ? std::log(static_cast<double>(total_files) / df) : 0.0;
        for(int f_id : f_name_ids) {
            score_map[f_id] += 10;
            pref_f_name_ids.erase(f_id); // avoid double count
        }
        for(int f_id : pref_f_name_ids) score_map[f_id] += 5;
        // Content match: TF-IDF weighted (scaled ×10 to match filename scores)
        /*
            TF-IDF = TF × IDF

            TF  (Term Frequency)     = how often word appears in THIS file
            IDF (Inverse Document Frequency) = how RARE the word is across ALL files

            IDF = log(total_files / files_containing_word)
        */
        for(const auto& [f_id, freq] : f_content_ids) {
            score_map[f_id] += static_cast<int>(freq * idf * 10);
        }
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
    std::shared_lock lock(mtx);
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

FileChanges SearchEngine::has_changed(const std::string& abs_path) const {
    FileChanges changes;
    std::unordered_set<std::string> seen_paths;
    namespace fs = std::filesystem;
    for(const auto& entry : fs::recursive_directory_iterator(abs_path)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().filename().string().starts_with(".indxr_")) continue;

        std::string path = entry.path().string();
        seen_paths.insert(path);

        //new file
        if(!fr.contains_path(path)) {
            changes.added.push_back(path);
            continue;
        }

        // existing files
        int id = fr.get_id_from_path(path);

        const FileMetaData& meta = fr.get_file(id);
        uintmax_t current_file_size = fs::file_size(entry.path());
        int64_t current_ticks = fs::last_write_time(entry.path()).time_since_epoch().count();
        if(current_file_size != meta.file_size || current_ticks != meta.last_modified_ticks) {
            changes.modified.push_back(path);
        }

    }
    // deleted files
    for(const auto& [id, meta] : fr.get_registry()) {
        if(!seen_paths.contains(meta.path)) {
            changes.deleted.push_back(meta.path);
        }
    }
    return changes;
}

void SearchEngine::update_changes(const FileChanges& changes) {
    if(changes.added.empty() && changes.modified.empty()
                        && changes.deleted.empty()) return;
    std::cout << "Updating Changes\n";

    // handle deleted files
    for(const auto& path : changes.deleted) {
        int f_id = fr.get_id_from_path(path);
        FileMetaData old_meta = fr.get_file(f_id);

        auto name_tokens = tokenize(old_meta.name, false);
        for(const auto& token : name_tokens) pt.remove_token(f_id, token);

        IdX.remove_file_tokens(f_id);

        fr.remove_file(f_id);
    }
    
    // handle modified files
    for(const auto& path : changes.modified) {
        int f_id = fr.get_id_from_path(path);
        IdX.remove_file_tokens(f_id);

        // create inverted index for new file content
        FileMetaData meta = fr.get_file(f_id);
        if(!meta.index_content) continue; // skip file for inverted index if not a text file 
        std::ifstream file(meta.path);
        if(!file.is_open()) {
            std::cerr << "Warning could not open file: " << meta.path << std::endl;
            continue;
        } 
    
        std::string line;
        while(std::getline(file, line)) {
            IdX.index_file_content(f_id, line);
        }
        // Update stored metadata so it won't re-trigger next time
        namespace fs = std::filesystem;
        fr.update_file_metadata(f_id, fs::file_size(path), 
                                fs::last_write_time(path).time_since_epoch().count());
    }

    // handle added files
    //index file names
    for(const auto& path : changes.added) {
        namespace fs = std::filesystem;
        std::string name = fs::path(path).filename().string();
        std::string ext = fs::path(path).extension().string();
        std::unordered_set<std::string> content_ext = {".txt", ".csv", ".md", ".log"};
        bool index_content = content_ext.count(ext) > 0;
        uintmax_t file_size = fs::file_size(path);
        int64_t last_modified_ticks = fs::last_write_time(path).time_since_epoch().count();

        fr.register_file(name, path, index_content, file_size, last_modified_ticks);
        int f_id = fr.get_id_from_path(path);

        pt.index_file_name(f_id, name);
        if(index_content) {
            std::ifstream file(path);
            std::string line, content;
            while(std::getline(file, line)) content += line;
            IdX.index_file_content(f_id, content);
        }
    }

    save();
    std::cout << "Changes Updated!\n";
}