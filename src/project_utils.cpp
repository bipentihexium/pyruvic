#include "project_utils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "formatted_out.hpp"
#include "parsing/lex.hpp"
#include "parsing/par.hpp"
#include "util.hpp"

void load_cfg() {
	std::string pyruvic_path = get_exe_path();
	if (pyruvic_path.empty()) {
		std::cout << prettyErrorGeneral("Couldn't get executable path :c", severity::FATAL) << std::endl;
		exit(-1);
	}
	std::string cfg_file(pyruvic_path + "/pyruvic.cfg");
	std::cout << prettyErrorGeneral("loading config file - " + cfg_file, severity::INFO) << std::endl;
	std::ifstream f(cfg_file);
	std::stringstream cfg_ss;
	cfg_ss << f.rdbuf();
	tokenstream ts = lex(cfg_file, cfg_ss);
	pyruvic_file cfg(parse(ts));
	for (const auto &c : cfg) {
		std::cout << c.first << std::endl;
		for (const auto &s : c.second) {
			std::cout << '\t' << (s.first.empty() ? "> ____" : s.first) << std::endl;
			for (const auto &vl : s.second) {
				std::cout << "\t\t" << vl.first;
				for (const auto &v : vl.second) {
					std::cout << " " << v << ",";
				}
				std::cout << std::endl;
			}
		}
	}
}
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
void build_project(bool release, bool obfuscate) {
	(void)release; (void)obfuscate;
}
void run_project() {
	;
}
