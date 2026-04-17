#include "../include/file_registery.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <filesystem>

void FileRegistry::register_file(const std::string&  name, const std::string& path) {
    registry[nextId] = {nextId, name, path};
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
    for(auto& entry : std::filesystem::directory_iterator(path)) {
        // Each entry is of type fs::directory_entry. It knows the file's full path, whether it's a file or folder, its extension, etc.
        if(entry.is_regular_file()) 
            FileRegistry::register_file(entry.path().filename().string(), entry.path().string());

    }
}