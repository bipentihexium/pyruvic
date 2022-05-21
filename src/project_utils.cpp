#include "project_utils.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "formatted_out.hpp"
#include "parsing/lex.hpp"
#include "parsing/par.hpp"
#include "runtime_config.hpp"
#include "util.hpp"

#if defined(__linux__)
constexpr const char *platform_idents[] = { "unix" };
#elif defined(_WIN32)
constexpr const char *platform_idents[] = { "win" };
#endif

const value_list &get_val_list_by_platform(const subcategory &subcat, const std::string &name) {
	for (const auto &vp : subcat) {
		if (std::find_if(std::begin(platform_idents), std::end(platform_idents),
			[&vp](const char *pi){ return pi == vp.first; })) {
			auto it = vp.second.find(name);
			if (it != vp.second.end()) {
				return it->second;
			}
		}
	}
	return subcat.at("").at(name);
}

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
	//for (const auto &t : ts.toks) { std::cout << prettyError(to_str(t.type), severity::DEBUG, t.loc, { highlight(t.val, severity::DEBUG, t.loc) }) << std::endl; }
	pyruvic_file cfg(parse(ts));
	const value_list &c_compilers = get_val_list_by_platform(cfg["[compilation]"][""], "c-compiler:");
	const value_list &cpp_compilers =get_val_list_by_platform(cfg["[compilation]"][""], "c++-compiler:");
	const value_list &linkers = get_val_list_by_platform(cfg["[compilation]"][""], "linker:");
	for (const auto &c_comp : c_compilers) { if (command_exists(c_comp)) { c_compiler = c_comp; break; } }
	for (const auto &cpp_comp : cpp_compilers) { if (command_exists(cpp_comp)) { cpp_compiler = cpp_comp; break; } }
	for (const auto &link : linkers) { if (command_exists(link)) { linker = link; break; } }
	if (c_compiler.empty()) { std::cout << prettyErrorGeneral("Could not find C compiler.", severity::FATAL) << std::endl; }
	if (cpp_compiler.empty()) { std::cout << prettyErrorGeneral("Could not find C++ compiler.", severity::FATAL) << std::endl; }
	if (linker.empty()) { std::cout << prettyErrorGeneral("Could not find linker.", severity::FATAL) << std::endl; }
	// TODO: load libraries info / just join the categories and make the list global :)
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
			"\tc++-standard: c++14\n"
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
