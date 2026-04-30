#include "../include/file_registery.h"
#include "../include/file_name_index.h"
#include "../include/inverted_index.h"
#include <iostream>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <utility>

void inverted_search(const InvertedIndex& IdX) {
	bool flag = true;
	while(flag) {
		std::cout << "Enter text to search: ";
		std::string s; std::cin >> s;

		std::unordered_map<int, int> results = IdX.search(s);
		if (results.empty()) std::cout << s << " : not found\n";
		else {
			std::cout << "file_id \t freq\n";
			for(const auto& [f_id, freq] : results) {
				std::cout << f_id << " \t\t " << freq << std::endl;
			}
		}

		char choice = 'n';
		std::cout << "Do you want to continue? (y/n) : ";
		std::cin >> choice;
		if(choice != 'y') flag = false;
	}
}

int main(int argc, char* argv[]) {
	namespace fs = std::filesystem;
	std::string dir_path;
	if(argc >= 2) {
		if(fs::is_directory(argv[1])) {
			dir_path = argv[1];
		} else {
			std::cerr << "Invalid directory \"" << argv[1] << "\" fallback to defaults\n";
		}
	}
	if(dir_path.empty()) dir_path = "./data";

	// debug
	FileRegistry fr;
	fr.index_directory(dir_path);
	fr.list_all();
	std::vector<std::pair<int, std::string>> files = fr.get_filenames();
	
	FilenameIndex fx;
	for(auto& index : files)
		fx.index_file_name(index.first, index.second);
	
	fx.list_all();

	// create inverted index
	InvertedIndex IdX;
	std::vector<std::string> file_content;

	for(const auto& [f_id, f_name] : files) {
		std::string file_path = dir_path + "/" + f_name;
		std::ifstream file(file_path);
		if(!file.is_open()) {
			std::cerr << "Warning could not open file: " << file_path << std::endl;
			return -1;
		} 

		std::string line;
		while(std::getline(file, line)) {
			IdX.index_file_content(f_id, line);
		}
	}

	inverted_search(IdX);

	return 0;
}
