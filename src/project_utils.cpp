#include "project_utils.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

void new_project(const std::string &name) {
	std::string path("./" + name + "/");
	std::filesystem::create_directory(path);
	std::filesystem::create_directory(path + ".pyr/");
	std::filesystem::create_directory(path + "build/");
	std::filesystem::create_directory(path + "src/");
	{
		std::ofstream gitignore_file(path + ".gitignore");
		gitignore_file << ".pyr/*\nbuild/*\nsrc/cfg.hpp" << std::endl;
	}
	{
		std::ofstream main_file(path + "src/main.cpp");
		main_file << "#include <iostream>\n\n"
			"int main(int argc, char **argv) {\n"
			"\t(void)argc; (void)argv;\n"
			"\tstd::cout << \"Hello Pyruvic world!\" << std::endl;\n"
			"\treturn 0;\n"
			"}" << std::endl;
	}
	{
		std::ofstream projinfo_file(path + "pyruvic.projinfo");
		std::stringstream macroname;
		for (const char &c : name) {
			if (isalpha(c)) {
				macroname << static_cast<char>(toupper(c));
			} else if (c == '_') {
				macroname << '_';
			}
		}
		projinfo_file << "[target]\n"
			"\tname: " << name << "\n"
			"\ttype: executable\n"
			"\tmacroname: " << macroname.str() << "\n"
			"\tversion: 1.0.0.0\n"
			"\tcfg-file: src/cfg.hpp\n"
			"\n"
			"[requirements]\n"
			"\tc++ standard: c++14\n"
			"\n"
			"[dependencies]\n"
			"> project\n"
			"> local" << std::endl;
	}
}
void load_project() {
	;
}
void clean_build_files() {
	;
}
void build_project() {
	;
}
void run_project() {
	;
}
