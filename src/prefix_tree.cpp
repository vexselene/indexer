#include "../include/tokenizer.h"
#include "../include/prefix_tree.h"
#include "../include/file_registery.h"
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

TrieNode::~TrieNode() {
    for(auto& [ch, child] : links) {
        delete child; // avoid mem leaks
    }
}

bool TrieNode::contains_key(char c) const {
    return links.find(c) != links.end();
}

void TrieNode::put_key(char c, TrieNode* newNode) {
    links[c] = newNode;
}

TrieNode* TrieNode::get_key(char c) const {
    return links.at(c); // [] operator can modify map -> at() safely returns key
}

void TrieNode::set_terminal(int id) {
    file_ids.insert(id);
    is_terminal_flag = true;
}

bool TrieNode::is_terminal() const {
    return is_terminal_flag;
}

PrefixTree::PrefixTree() {root = new TrieNode();}

PrefixTree::~PrefixTree() {delete root;}

void PrefixTree::insert(int id, const std::string& token) {
    TrieNode* node = root;
    for(char c : token) {
        if(!node->contains_key(c)) node->put_key(c, new TrieNode());
        node = node->get_key(c);
    }
    node->set_terminal(id);
}

void PrefixTree::index_file_name(int id, const std::string& file_name) {
    std::vector<std::string> tokens = tokenize(file_name, false);
    for(auto& token : tokens) {
        insert(id, token);
    }
}

std::unordered_set<int> PrefixTree::search(const std::string& token) const {
    TrieNode* node = root;
    for(char c : token) {
        if(!node->contains_key(c)) {
            return {}; // return empty if word not found
        }
        node = node->get_key(c);
    }
    return node->file_ids;
}

std::unordered_set<int> PrefixTree::search_matching(const std::string& prefix) const {
    TrieNode* node = root;
    for(char c : prefix) {
        if(!node->contains_key(c)) {
            return {}; // return empty set if prefix not found
        }
        node = node->get_key(c);
    }

    std::unordered_set<int> f_ids;
    get_file_ids(node, f_ids);
    return f_ids;
}

void PrefixTree::get_file_ids(TrieNode* node, std::unordered_set<int>& f_ids) const {
    if(node->is_terminal()) {
        f_ids.insert(node->file_ids.begin(), node->file_ids.end());
    }
    for(auto& link : node->links) {
        get_file_ids(node->get_key(link.first), f_ids);
    }
}

void PrefixTree::get_words(TrieNode* node, std::vector<std::string>& words, std::string word = "") const {
    if(node->is_terminal()) {
        words.push_back(word);
    }
    for(auto& link : node->links) {
        word.push_back(link.first);
        get_words(node->get_key(link.first), words, word);
        word.pop_back(); // backtrack
    }
}

void PrefixTree::list_all() const {
    std::vector<std::string> words;
    get_words(root, words);
    for(auto& word : words) {
        std::cout << word << std::endl;
    }
}