#pragma once
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <filesystem>

struct FileMetaData {
	int id;
	std::string name;
	std::string path;
    bool index_content;  // true for .txt, .md, .log, .csv

    uintmax_t file_size;
    int64_t last_modified_ticks;
};

class FileRegistry {
private:
    std::unordered_map<int, FileMetaData> registry;
    std::unordered_map<std::string, int> path_to_id;
    int nextId = 0;
public:
    const std::unordered_map<int, FileMetaData>& get_registry() const {return registry;}
    void register_file(const std::string&  name, const std::string& path, bool index_content,
                                 uintmax_t file_size, int64_t last_modified_ticks) ;
    const FileMetaData& get_file(int id) const;
    void remove_file(int id);
    void update_file_metadata(int id, uintmax_t size, int64_t ticks);
    void index_directory(const std::string& path);
    std::vector<std::pair<int, std::string>> get_filenames() const;
    void list_all() const;
    bool contains_path(const std::string& path) const;
    int get_id_from_path(const std::string& path) const;
    void serialize(std::ostream& out) const;
    void deserialize(std::istream& in);
};