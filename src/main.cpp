#include "../include/file_registery.h"
#include <iostream>
#include <cerrno>
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

	FileRegistry fr;
	fr.index_directory(dir_path);
	std::cout << "files in the directory \"" << dir_path << "\" :\n";
	fr.list_all();

	return 0;
}
