#pragma once
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

struct FileMetaData {
	int id;
	std::string name;
	std::string path;
    bool index_content;  // true for .txt, .md, .log, .csv
};

class FileRegistry {
private:
    std::unordered_map<int, FileMetaData> registry;
    int nextId = 0;
public:
    void register_file(const std::string& name, const std::string& path, bool index_content);
    const FileMetaData& get_file(int id) const;
    void list_all() const;
    void index_directory(const std::string& path);
    std::vector<std::pair<int, std::string>> get_filenames() const;
};