#pragma once
#include <unordered_map>
#include <string>
#include <unordered_set>

class FilenameIndex {
private:
    std::unordered_map<std::string, std::unordered_set<int>> index;
public:
    void add_file(int id, const std::string& file_name);
    const std::unordered_set<int>& search(const std::string& token) const;
    void list_all() const;
};
