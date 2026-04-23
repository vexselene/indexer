#include "../include/file_registery.h"
#include "../include/file_name_index.h"
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

	// debug
	FileRegistry fr;
	fr.index_directory(dir_path);
	fr.list_all();
	std::vector<std::pair<int, std::string>> files = fr.get_filenames();
	
	FilenameIndex fx;
	for(auto& index : files)
		fx.add_file(index.first, index.second);
	
	fx.list_all();

	return 0;
}
