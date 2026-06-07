#include "../include/inverted_index.h"
#include "../include/tokenizer.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <cerrno>

#include <fstream> // std::ifstream std::ofstream

template<typename T>
void write_binary(std::ofstream& file, const T& value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value)); // write(value, sizeof value type)
}

template<typename T>
void read_binary(std::ifstream& file, T& value) {
    file.read(reinterpret_cast<char*>(&value), sizeof(value)); // read(value, sizeof type)
}

void InvertedIndex::index_file_content(int id, const std::string& content) {
    auto tokens = tokenize(content, true);
    for (std::string token : tokens) {
        index[token][id]++;
    }
}

std::unordered_map<int, int> InvertedIndex::search(const std::string& token) const {
    auto it = index.find(token);
    if (it != index.end()) return it->second; // unordered_map<file_id, freq>
    return {};
}

void InvertedIndex::list_all() const {
    for (const auto&  [token, idx] : index) {
        bool first = true;
        std::cout << token << ": { ";
        for (const auto& [id, freq] : idx) {
            if (!first) std::cout << ", ";
            std::cout << id << " : " << freq;
            first = false;
        }
        std::cout << " }\n";
    }
}

/*
    [token_count]
    For each token:
        [token_string_length]
        [token_string_bytes]
        
        [file_count]

        For each file:
            [file_id]
            [frequency]
*/
/*
    Example
        2                    <-- token_count

        5                    <-- length("hello")
        hello                <-- token bytes
        2                    <-- file_count

        1                    <-- file_id
        3                    <-- frequency

        7                    <-- file_id
        2                    <-- frequency   

        5                    <-- length("world")
        world
        1

        1
        5
*/
void InvertedIndex::serialize(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary); // open file in binary mode
    if(!file) throw std::runtime_error("Could not open file.");

    int token_count = static_cast<int>(index.size());
    write_binary(file, token_count);

    for(const auto& [token, postings] : index) {
        int token_length = static_cast<int>(token.size());
        write_binary(file, token_length);
        file.write(token.data(), token_length);

        write_binary(file, static_cast<int>(postings.size()));
        for(const auto& [f_id, freq] : postings) {
            write_binary(file, f_id);
            write_binary(file, freq);
        }
    }
}

void InvertedIndex::deserialize(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if(!file) throw std::runtime_error("Could not open file.");

    index.clear();

    int token_count;
    read_binary(file, token_count);

    for(int i = 0; i < token_count; i++) {
        int token_length;
        read_binary(file, token_length);
        std::string token(token_length, '\0'); // create token string of legth token_length
        file.read(token.data(), token_length); // read data(token_length) into token.data()

        int postings_count;
        read_binary(file, postings_count);
        for(int i = 0; i < postings_count; i++) {
            int file_id;
            int freq;

            read_binary(file, file_id);
            read_binary(file, freq);

            index[token][file_id] = freq;
        }
    }
}