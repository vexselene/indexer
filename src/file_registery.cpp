#include "../include/file_registery.h"
#include "../include/file_name_index.h"
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <filesystem>

void FileRegistry::register_file(const std::string&  name, const std::string& path, bool index_content) {
    registry[nextId] = {nextId, name, path, index_content};
    nextId++;
}

// operator [] can INSERT a new element if key doesn't exist - .at() throws std::out_of_range
const FileMetaData& FileRegistry::get_file(int id) const { return registry.at(id); }

void FileRegistry::list_all() const {
    for (const auto& [id, meta] : registry) {
        std::cout << "[" << id << "] " << meta.name << "  →  " << meta.path << std::endl;
    }
}

void FileRegistry::index_directory(const std::string& path) {
    const std::unordered_set<std::string> content_exts = {".txt", ".md", ".csv", ".log"};
    for(auto& entry : std::filesystem::recursive_directory_iterator(path,
                      std::filesystem::directory_options::skip_permission_denied)) {
        // Each entry is of type fs::directory_entry. It knows the file's full path, whether it's a file or folder, its extension, etc.
        if(entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            bool index_content = content_exts.count(ext) > 0;
            FileRegistry::register_file(entry.path().filename().string(), 
                                                entry.path().string(), index_content);
        }
    }
}

std::vector<std::pair<int, std::string>> FileRegistry::get_filenames() const {
    std::vector<std::pair<int, std::string>> file_names;
    for (const auto& [id, meta] : registry) {
        file_names.emplace_back(id, meta.name);
    }
    return file_names;
}