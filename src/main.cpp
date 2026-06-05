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
	if(dir_path.empty()) dir_path = "data";

	SearchEngine engine;
	engine.build(dir_path);
	while(true) {
		std::string query;
		std::cout << "Enter text to search: ";
		std::getline(std::cin, query);
		
		if(query.empty()) continue;
		if(query == ":q" || query == ":Q") break;
		if(query == ":help") {
			std::cout << "\nCommands:\n";
			std::cout << "  :q, :Q    - Quit\n";
			std::cout << "  :help     - Show this message\n";
			std::cout << "  <query>   - Search for files\n\n";
			continue;
		}
		
		auto results = engine.search(query);
		engine.display(results);
	}

	return 0;
}
