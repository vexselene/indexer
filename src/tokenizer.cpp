#include "../include/tokenizer.h"
#include <string>
#include <vector>
#include <cctype>

std::vector<std::string> tokenize(const std::string& text, bool allow_numbers) {
    std::vector<std::string> tokens;
    std::string token;

    for (char x : text) {
        unsigned char c = static_cast<unsigned char>(x);

        bool valid = allow_numbers ? std::isalnum(c) : std::isalpha(c);

        if (!valid) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            continue;
        }

        token += std::tolower(c);
    }

    if (!token.empty()) tokens.push_back(token);

    return tokens;
}