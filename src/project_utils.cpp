#include "project_utils.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include "cmdutils.hpp"
#include "formatted_out.hpp"
#include "parsing/lex.hpp"
#include "parsing/par.hpp"
#include "runtime_config.hpp"
#include "util.hpp"

extern bool verbose;
dependency_info deps;
std::string pyruvic_path;

const value_list &get_val_list_by_platform(const subcategory &subcat, const std::string &name) {
	static value_list empty_val_list;
	for (const auto &vp : subcat) {
		if (std::find_if(std::begin(platform_idents), std::end(platform_idents),
			[&vp](const char *pi){ return pi == vp.first; })) {
			auto it = vp.second.find(name);
			if (it != vp.second.end()) {
				return it->second;
			}
		}
	}
	const value_pack &pck = subcat.at("");
	auto it = pck.find(name);
	if (it == pck.end())
		return empty_val_list;
	return it->second;
}
void replace_vars(const project_info &info, std::string &str) {
	std::map<std::string, std::string> vars{
		{ "${src}", "./src" },
		{ "${build}", "./build" },
		{ "${name}", info.name },
		{ "${macroname}", info.macroname },
		{ "${version_major}", std::to_string(info.ver_maj) },
		{ "${version_minor}", std::to_string(info.ver_min) },
		{ "${version_patch}", std::to_string(info.ver_pat) },
		{ "${version_tweak}", std::to_string(info.ver_twe) },
		{ "${version_name}", info.ver_name },
	};
	for (size_t i = str.rfind('$'); i != std::string::npos; i = str.rfind('$', i-1)) {
		size_t j = str.find('}', i + 2);
		if (j != std::string::npos && str[i+1] == '{') {
			std::string v = str.substr(i, j-i+1);
			auto it = vars.find(v);
			if (it != vars.end()) {
				str.replace(i, j-i+1, it->second);
			}
		}
		if (i == 0)
			break;
	}
}
std::string proj_fileext(project_t t) {
	switch (t) {
#if defined(__linux__)
	case project_t::EXECUTABLE: return "";
	case project_t::STATIC_LIBRARY: return ".a";
	case project_t::DYNAMIC_LIBRARY: return ".so";
#elif defined(_WIN32)
	case project_t::EXECUTABLE: return ".exe";
	case project_t::STATIC_LIBRARY: return ".a";
	case project_t::DYNAMIC_LIBRARY: return ".dll";
#endif
	};
	return "";
}

void load_cfg() {
	pyruvic_path = get_exe_path();
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
	const value_list &c_compilers = get_val_list_by_platform(cfg["[compilation]"][""], "c-compiler:");
	const value_list &cpp_compilers =get_val_list_by_platform(cfg["[compilation]"][""], "c++-compiler:");
	const value_list &linkers = get_val_list_by_platform(cfg["[compilation]"][""], "linker:");
	for (const auto &c_comp : c_compilers) { if (command_exists(c_comp)) { c_compiler = c_comp; break; } }
	for (const auto &cpp_comp : cpp_compilers) { if (command_exists(cpp_comp)) { cpp_compiler = cpp_comp; break; } }
	for (const auto &link : linkers) { if (command_exists(link)) { linker = link; break; } }
	if (c_compiler.empty()) { std::cout << prettyErrorGeneral("Could not find C compiler.", severity::FATAL) << std::endl; exit(-1); }
	if (cpp_compiler.empty()) { std::cout << prettyErrorGeneral("Could not find C++ compiler.", severity::FATAL) << std::endl; exit(-1); }
	if (linker.empty()) { std::cout << prettyErrorGeneral("Could not find linker.", severity::FATAL) << std::endl; exit(-1); }
	linker = cpp_compiler + " -fuse-ld=" + linker; // TODO: change this

	deps.load(cfg["[auogen-libraries]"]);
	deps.load(cfg["[pkg-config-auogen-libraries]"]);
	deps.load(cfg["[known-libraries]"]);
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
			"[dependencies]" << std::endl;
	}
}
