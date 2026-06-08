#include "../include/file_registery.h"
#include "../include/file_name_index.h"
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <filesystem>

template<typename T>
void write_binary(std::ostream& file, const T& value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

template<typename T>
void read_binary(std::istream& file, T& value) {
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
}

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
            if (entry.path().filename().string().starts_with(".indxr_")) continue; // skip files for serialization and deserialization
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
/*
[file_count]
For each file:
    [id]
    [name_length]
    [name_bytes]
    [path_length]
    [path_bytes]
    [index_content]        ← 1 byte (bool)
*/
void FileRegistry::serialize(std::ostream& out) const {
    int file_count = static_cast<int>(registry.size());
    write_binary(out, file_count);

    for(const auto& [f_id, meta] : registry) {
        write_binary(out, meta.id);

        int name_length = static_cast<int>(meta.name.size());
        write_binary(out, name_length);
        out.write(meta.name.data(), name_length);
        
        int path_length = static_cast<int>(meta.path.size());
        write_binary(out, path_length);
        out.write(meta.path.data(), path_length);

        uint8_t ic = meta.index_content ? 1 : 0;  // write it as a fixed-size type for portability
        write_binary(out, ic);

    }
}

void FileRegistry::deserialize(std::istream& in) {
    registry.clear();

    int file_count;
    read_binary(in, file_count);

    for(int i = 0; i < file_count; i++) {
        int id;
        read_binary(in, id);
        
        int name_length;
        read_binary(in, name_length);
        std::string name(name_length, '\0');
        in.read(name.data(), name_length);
        
        int path_length;
        read_binary(in, path_length);
        std::string path(path_length, '\0');
        in.read(path.data(), path_length);

        uint8_t ic;
        read_binary(in, ic);

        registry[id] = {id, name, path, ic != 0};
        if(id >= nextId) nextId = id + 1;
    }
}