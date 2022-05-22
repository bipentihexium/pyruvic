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

#if defined(__linux__)
constexpr const char *platform_idents[] = { "unix" };
#elif defined(_WIN32)
constexpr const char *platform_idents[] = { "win" };
#endif

extern bool verbose;
constexpr const char *filehist_file = "./.pyr/filehist";
constexpr const char *filedeps_file = "./.pyr/filedeps";
constexpr const char *last_build_file = "./.pyr/last_build";
constexpr const char *objfile_ext = ".o";
file_history hist;
file_dependencies fdeps;
bool rebuild;
std::string pyruvic_path;
std::vector<std::string> prebuild_commands;
std::vector<std::string> prebuild_parallel_commands;
std::vector<std::string> postbuild_commands;
std::vector<std::string> postbuild_parallel_commands;

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
void replace_vars(std::string &str) {
	std::map<std::string, std::string> vars{
		{ "${src}", "./src" },
		{ "${build}", "./build" },
		{ "${name}", project::name },
		{ "${macroname}", project::macroname },
		{ "${version_major}", std::to_string(project::ver_maj) },
		{ "${version_minor}", std::to_string(project::ver_min) },
		{ "${version_patch}", std::to_string(project::ver_pat) },
		{ "${version_tweak}", std::to_string(project::ver_twe) },
		{ "${version_name}", project::ver_name },
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
std::string proj_fileext() {
	switch (project::type) {
#if defined(__linux__)
	case project::project_t::EXECUTABLE: return "";
	case project::project_t::STATIC_LIBRARY: return ".a";
	case project::project_t::DYNAMIC_LIBRARY: return ".so";
#elif defined(_WIN32)
	case project::project_t::EXECUTABLE: return ".exe";
	case project::project_t::STATIC_LIBRARY: return ".a";
	case project::project_t::DYNAMIC_LIBRARY: return ".dll";
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

	// TODO: load library info / just join the categories and make the list global :)
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
void load_project(bool release, bool obfuscate) {
	std::string proj_file("./pyruvic.projinfo");
	if (!std::filesystem::exists(proj_file)) {
		std::cout << prettyErrorGeneral("could not find project file - " + proj_file, severity::FATAL) << std::endl;
		exit(-1);
	}
	std::cout << prettyErrorGeneral("loading project file - " + proj_file, severity::INFO) << std::endl;
	std::ifstream f(proj_file);
	std::stringstream projfile_ss;
	projfile_ss << f.rdbuf();
	tokenstream ts = lex(proj_file, projfile_ss);
	pyruvic_file proj(parse(ts));
	bool errors = false;
	if (proj["[target]"][""][""]["name:"].empty()) { std::cout << prettyErrorGeneral("[target] must have name", severity::ERROR) << std::endl; errors = true; }
	if (proj["[target]"][""][""]["type:"].empty()) { std::cout << prettyErrorGeneral("[target] must have type", severity::ERROR) << std::endl; errors = true; }
	if (proj["[target]"][""][""]["macroname:"].empty()) { std::cout << prettyErrorGeneral("[target] must have macroname", severity::ERROR) << std::endl; errors = true; }
	if (proj["[target]"][""][""]["version:"].empty()) { std::cout << prettyErrorGeneral("[target] must have version", severity::ERROR) << std::endl; errors = true; }
	if (errors) { exit(-1); }

	project::name = proj["[target]"][""][""]["name:"][0];
	const std::string &t = proj["[target]"][""][""]["type:"][0];
	if (t == "executable") project::type = project::project_t::EXECUTABLE;
	else if (t == "static library") project::type = project::project_t::STATIC_LIBRARY;
	else if (t == "dynamic library") project::type = project::project_t::DYNAMIC_LIBRARY;
	else { std::cout << prettyErrorGeneral("Unkown target type \"" + t + "\"", severity::ERROR) << std::endl; errors = true; }
	project::macroname = proj["[target]"][""][""]["macroname:"][0];
	const std::string &v = proj["[target]"][""][""]["version:"][0];
	project::ver_maj = project::ver_min = project::ver_pat = project::ver_twe = 0;
	int *ver_vals_ptrs[] = { &project::ver_maj, &project::ver_min, &project::ver_pat, &project::ver_twe };
	constexpr int ver_vals_size = 4;
	int ver_step = 0;
	std::string buf;
	for (size_t i = 0; i < v.size(); ++i) {
		switch (v[i]) {
		case ' ': project::ver_name = v.substr(i + 1); i = v.size(); break;
		case '.':
			if (ver_step >= ver_vals_size - 1) {
				std::cout << prettyErrorGeneral("Too much sub-versions (version: " + v + ")", severity::ERROR) << std::endl;
				errors = true;
				goto break_version_loop;
			}
			ver_vals_ptrs[ver_step++][0] = std::stoi(buf);
			buf.clear();
			break;
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			buf.push_back(v[i]);
			break;
		default:
			std::cout << prettyErrorGeneral("Unexpected character in version (version: " + v + ")", severity::ERROR) << std::endl;
			errors = true;
			break;
		};
	}
break_version_loop:
	if (!proj["[target]"][""][""]["cfg-file:"].empty()) {
		project::cfg_file = proj["[target]"][""][""]["cfg-file:"][0];
		replace_vars(project::cfg_file);
	}
	if (!proj["[requirements]"][""][""]["c-standard:"].empty()) {
		project::c_standard = proj["[requirements]"][""][""]["c-standard:"][0];
	}
	if (!proj["[requirements]"][""][""]["c++-standard:"].empty()) {
		project::cpp_standard = proj["[requirements]"][""][""]["c++-standard:"][0];
	}
	for (const auto &stdlib : get_val_list_by_platform(proj["[requirements]"][""], "libs:"))
		project::stdlibs.push_back(stdlib);

	if (errors) { exit(-1); }

	if (std::filesystem::exists(filehist_file))
		hist.load_saved(filehist_file);
	if (std::filesystem::exists(filedeps_file))
		fdeps.load_saved(filedeps_file);
	if (std::filesystem::exists(last_build_file)) {
		std::ifstream flb(last_build_file);
		uint16_t build_data;
		flb >> build_data;
		if ((build_data & 1) != release || ((build_data >> 1) & 1) != obfuscate) {
			hist.clear();
		}
	} else {
		hist.clear(); // rebuild when unknown
	}

	auto fillcmds = [](std::vector<std::string> &cmds, subcategory &subc) {
		for (const auto &vp : subc) {
			if (vp.first.empty() || std::find_if(std::begin(platform_idents), std::end(platform_idents),
				[&vp](const char *plid) { return vp.first == plid; }) != std::end(platform_idents)) {
				for (const auto &vl : vp.second) {
					std::string file(vl.first.substr(0, vl.first.size() - 1));
					replace_vars(file);
					if (file == "__always__" || hist.was_updated(file, fdeps)) {
						for (const auto &v : vl.second) {
							std::string v_copy(v);
							replace_vars(v_copy);
							cmds.push_back(v_copy);
							if (verbose)
								std::cout << prettyErrorGeneral(v_copy, severity::DEBUG) << std::endl;
						}
						hist.update(file);
					}
				}
			}
		}
	};
	category &commands = proj["[commands]"];
	fillcmds(prebuild_commands, commands["pre-build"]);
	fillcmds(prebuild_parallel_commands, commands["pre-build-parallel"]);
	fillcmds(postbuild_commands, commands["post-build"]);
	fillcmds(postbuild_parallel_commands, commands["post-build-parallel"]);

	// TODO: load dependencies
}
void clean_build_files() {
	std::filesystem::remove_all("./.pyr/");
}
void pre_build() {
	if (!prebuild_commands.empty())
		run_commands(prebuild_commands, project::name + " pre-build commands");
	if (!prebuild_parallel_commands.empty())
		run_commands_parallel(prebuild_parallel_commands, project::name + " pre-build commands (parallel)");

	if (!project::cfg_file.empty() && hist.was_updated("./pyruvic.projinfo")) {
		std::string template_file(pyruvic_path + "/pyruvic-default-cfg-format.cfg");
		std::cout << prettyErrorGeneral("configuring " + project::cfg_file, severity::INFO) << std::endl;
		std::stringstream template_ss;
		{
			std::ifstream f(template_file);
			template_ss << f.rdbuf();
		}
		std::string templ(template_ss.str());
		replace_vars(templ);
		std::ofstream fw(project::cfg_file);
		fw << templ;
	}
}
void build_project(bool release, bool obfuscate) {
	// TODO: libraries

	std::string compile_options("-Wall ");
	std::string c_compile_options("");
	std::string cpp_compile_options("");
	std::string link_options("");
	if (release) {
		compile_options += "-O3 ";
		if (obfuscate) {
			compile_options += "-static -s -fvisibility=hidden -fvisibility-inlines-hidden ";
		}
	} else {
		compile_options += "-Wextra -Wpedantic -g ";
	}
	if (!project::c_standard.empty()) {
		c_compile_options += "-std=" + project::c_standard + " ";
	}
	if (!project::cpp_standard.empty()) {
		cpp_compile_options += "-std=" + project::cpp_standard + " ";
	}
	for (const auto &stdlib : project::stdlibs) {
		link_options += " -l" + stdlib;
	}
	std::vector<std::string> obj_files;
	std::vector<std::string> build_cmds;
	for (const auto &dir_entry : std::filesystem::recursive_directory_iterator("./src/")) {
		if (!dir_entry.is_directory()) {
			std::string file(dir_entry.path().string());
			bool c_file = file.ends_with(".c");
			bool cpp_file = file.ends_with(".cpp");
			bool c_cpp_header_file = c_file || cpp_file || file.ends_with(".h") || file.ends_with(".hpp");
			bool updated = false;
			if (hist.was_updated(file)) {
				if (c_cpp_header_file)
					fdeps.save_c_cpp_deps(file);
				updated = true;
			} else if (hist.was_updated(file, fdeps)) {
				updated = true;
			}
			if (c_file || cpp_file) {
				std::string objfile("./.pyr/objfiles/" + dir_entry.path().filename().replace_extension(objfile_ext).string());
				if (updated) {
					if (c_file) {
						std::string cmd(c_compiler + " " + compile_options + c_compile_options + "-c -o \"" + objfile + "\" \"" + file + "\"");
						if (verbose)
							std::cout << prettyErrorGeneral(cmd, severity::DEBUG) << std::endl;
						build_cmds.push_back(cmd);
					} else {
						std::string cmd(cpp_compiler + " " + compile_options + cpp_compile_options + "-c -o \"" + objfile + "\" \"" + file + "\"");
						if (verbose)
							std::cout << prettyErrorGeneral(cmd, severity::DEBUG) << std::endl;
						build_cmds.push_back(cmd);
					}
				}
				obj_files.push_back(objfile);
			}
		}
	}
	for (const auto &dir_entry : std::filesystem::recursive_directory_iterator("./src/")) {
		if (!dir_entry.is_directory()) {
			std::string file(dir_entry.path().string());
			bool c_cpp_header_file = file.ends_with(".c") || file.ends_with(".cpp") || file.ends_with(".h") || file.ends_with(".hpp");
			if (c_cpp_header_file) {
				hist.update(file);
			}
		}
	}
	std::stringstream linkcmd;
	if (project::type != project::project_t::STATIC_LIBRARY) {
		linkcmd << linker << " -o \"./build/" << project::name << proj_fileext() << "\"";
		for (const auto &of : obj_files) {
			linkcmd << " \"" << of << "\"";
		}
		linkcmd << link_options;
	}
	if (verbose)
		std::cout << prettyErrorGeneral(linkcmd.str(), severity::DEBUG) << std::endl;
	if (!std::filesystem::exists("./.pyr/objfiles/")) {
		std::filesystem::create_directories("./.pyr/objfiles/");
	}
	if (build_using(build_cmds, linkcmd.str(), "building " + project::name)) {
		std::cout << prettyErrorGeneral("failed building " + project::name, severity::ERROR) << std::endl;
		exit(-1);
	}
	std::ofstream flb(last_build_file);
	uint16_t build_data = static_cast<uint16_t>(obfuscate) << 1 | release;
	flb << build_data;
	std::cout << prettyErrorGeneral("\x1b[92mbuilt " + project::name + colReset, severity::INFO) << std::endl;
}
void post_build() {
	hist.update("./pyruvic.projinfo");
	if (hist.save(filehist_file)) {
		std::cout << prettyErrorGeneral("failed saving file history", severity::ERROR) << std::endl;
	}
	if (fdeps.save(filedeps_file)) {
		std::cout << prettyErrorGeneral("failed saving file dependencies", severity::ERROR) << std::endl;
		std::cout << prettyErrorGeneral("if file history was saved, project might not build correctly next time", severity::WARN) << std::endl;
		std::cout << prettyErrorGeneral("deleting file history (./.pyr/filehist) recommended", severity::NOTE) << std::endl;
	}

	if (!postbuild_commands.empty())
		run_commands(prebuild_commands, project::name + " post-build commands");
	if (!postbuild_parallel_commands.empty())
		run_commands_parallel(postbuild_parallel_commands, project::name + " post-build commands (parallel)");
}
void run_project() {
	if (project::type != project::project_t::STATIC_LIBRARY)
		system(("./build/" + project::name + proj_fileext()).c_str());
}
