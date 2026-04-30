#pragma once
#include <unordered_map>
#include <string>

class InvertedIndex {
private:
    std::unordered_map<std::string, std::unordered_map<int, int>> index;
public:
    void index_file_content(int id, const std::string& content); // tokenizes content, increments frequency for each token
    const std::unordered_map<int, int>& search(const std::string& token) const;
    void list_all() const;
};