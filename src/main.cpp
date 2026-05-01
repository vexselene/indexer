#include "../include/search_engine.h"
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <filesystem>

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

	SearchEngine engine;
	engine.build(dir_path);
	bool flag = true;
	while(flag) {
		std::string s; 
		std::cout << "Enter text to search: "; std::cin >> s;
		std::vector<std::pair<int, int>> results = engine.search(s);
		engine.display(results);

		char choice = 'n';
		std::cout << "\nDo you want to continue? (y/n): ";
		std::cin >> choice;
		if(choice != 'y') flag = false;
	}

	return 0;
}
