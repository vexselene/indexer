#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

struct TrieNode {
    std::unordered_map<char, TrieNode*> links;
    std::unordered_set<int> file_ids;
    bool is_terminal_flag;

    ~TrieNode();
    bool contains_key(char c) const;
    void put_key(char c, TrieNode* newNode);
    TrieNode* get_key(char c) const;
    void set_terminal(int id);
    bool is_terminal() const;
};

class PrefixTree {
private:
    TrieNode* root;
public:
    PrefixTree();
    ~PrefixTree();
    void insert(int id, const std::string& token);
    void index_file_name(int id, const std::string& file_name);
    std::unordered_set<int> search(const std::string& token) const;
    std::unordered_set<int> search_matching(const std::string& prefix) const;
    void get_file_ids(TrieNode* node, std::unordered_set<int>& f_ids) const;
    void get_words(TrieNode* node, std::vector<std::string>& words, std::string word) const;
    void list_all() const;
};