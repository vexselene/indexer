#include "../include/inverted_index.h"
#include "../include/tokenizer.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <cerrno>

template<typename T>
void write_binary(std::ostream& file, const T& value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value)); // write(value, sizeof value type)
}

template<typename T>
void read_binary(std::istream& file, T& value) {
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
void InvertedIndex::serialize(std::ostream& out) const {
    int token_count = static_cast<int>(index.size());
    write_binary(out, token_count);

    for(const auto& [token, postings] : index) {
        int token_length = static_cast<int>(token.size());
        write_binary(out, token_length);
        out.write(token.data(), token_length);

        write_binary(out, static_cast<int>(postings.size()));
        for(const auto& [f_id, freq] : postings) {
            write_binary(out, f_id);
            write_binary(out, freq);
        }
    }
}

void InvertedIndex::deserialize(std::istream& in) {
    index.clear();

    int token_count;
    read_binary(in, token_count);

    for(int i = 0; i < token_count; i++) {
        int token_length;
        read_binary(in, token_length);
        std::string token(token_length, '\0'); // create token string of legth token_length
        in.read(token.data(), token_length); // read data(token_length) into token.data()

        int postings_count;
        read_binary(in, postings_count);
        for(int i = 0; i < postings_count; i++) {
            int file_id;
            int freq;

            read_binary(in, file_id);
            read_binary(in, freq);

            index[token][file_id] = freq;
        }
    }
}

void InvertedIndex::remove_file_tokens(int f_id) { 
    for(auto it = index.begin(); it != index.end(); ) {
        it->second.erase(f_id); // (map.erase(key)) | index = <token, <f_id, freq> 
        if(it->second.empty()) 
            it = index.erase(it); // erase returns the NEXT valid iterator
        else 
            ++it; // advance normally
    }
}

// how many files contain this token
int InvertedIndex::document_frequency(const std::string& token) const {
    auto it = index.find(token);
    if (it != index.end()) return it->second.size();
    return 0;
}