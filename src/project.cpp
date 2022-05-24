#include "project.hpp"

#include <filesystem>
#include <iostream>
#include "cmdutils.hpp"
#include "formatted_out.hpp"

extern bool verbose;

void project::load(bool release, bool obfuscate) {
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

	info.name = proj["[target]"][""][""]["name:"][0];
	const std::string &t = proj["[target]"][""][""]["type:"][0];
	if (t == "executable") info.type = project_t::EXECUTABLE;
	else if (t == "static library") info.type = project_t::STATIC_LIBRARY;
	else if (t == "dynamic library") info.type = project_t::DYNAMIC_LIBRARY;
	else { std::cout << prettyErrorGeneral("Unkown target type \"" + t + "\"", severity::ERROR) << std::endl; errors = true; }
	info.macroname = proj["[target]"][""][""]["macroname:"][0];
	const std::string &v = proj["[target]"][""][""]["version:"][0];
	info.ver_maj = info.ver_min = info.ver_pat = info.ver_twe = 0;
	int *ver_vals_ptrs[] = { &info.ver_maj, &info.ver_min, &info.ver_pat, &info.ver_twe };
	constexpr int ver_vals_size = 4;
	int ver_step = 0;
	std::string buf;
	for (size_t i = 0; i < v.size(); ++i) {
		switch (v[i]) {
		case ' ': info.ver_name = v.substr(i + 1); i = v.size(); break;
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
		info.cfg_file = proj["[target]"][""][""]["cfg-file:"][0];
		replace_vars(info, info.cfg_file);
	}
	if (!proj["[requirements]"][""][""]["c-standard:"].empty()) {
		info.c_standard = proj["[requirements]"][""][""]["c-standard:"][0];
	}
	if (!proj["[requirements]"][""][""]["c++-standard:"].empty()) {
		info.cpp_standard = proj["[requirements]"][""][""]["c++-standard:"][0];
	}
	for (const auto &stdlib : get_val_list_by_platform(proj["[requirements]"][""], "libs:"))
		info.stdlibs.push_back(stdlib);

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

	auto fillcmds = [this](std::vector<std::string> &cmds, subcategory &subc) {
		for (const auto &vp : subc) {
			if (vp.first.empty() || std::find_if(std::begin(platform_idents), std::end(platform_idents),
				[&vp](const char *plid) { return vp.first == plid; }) != std::end(platform_idents)) {
				for (const auto &vl : vp.second) {
					std::string file(vl.first.substr(0, vl.first.size() - 1));
					replace_vars(info, file);
					if (file == "__always__" || hist.was_updated(file, fdeps)) {
						for (const auto &v : vl.second) {
							std::string v_copy(v);
							replace_vars(info, v_copy);
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

	subcategory &dependencies = proj["[dependencies]"][""];
	for (const auto &vp : dependencies) {
		if (vp.first.empty() || std::find_if(std::begin(platform_idents), std::end(platform_idents),
			[&vp](const char *pi){ return pi == vp.first; })) {
			for (const auto &v : vp.second) {
				if (!v.second.empty())
					depends.push_back(std::make_pair(v.first, v.second[0]));
			}
		}
	}
}
void project::clean_build_files() const {
	std::filesystem::remove_all("./.pyr/");
}
void project::pre_build() {
	if (!prebuild_commands.empty())
		run_commands(prebuild_commands, info.name + " pre-build commands");
	if (!prebuild_parallel_commands.empty())
		run_commands_parallel(prebuild_parallel_commands, info.name + " pre-build commands (parallel)");

	if (!info.cfg_file.empty() && hist.was_updated("./pyruvic.projinfo")) {
		std::string template_file(pyruvic_path + "/pyruvic-default-cfg-format.cfg");
		std::cout << prettyErrorGeneral("configuring " + info.cfg_file, severity::INFO) << std::endl;
		std::stringstream template_ss;
		{
			std::ifstream f(template_file);
			template_ss << f.rdbuf();
		}
		std::string templ(template_ss.str());
		replace_vars(info, templ);
		std::ofstream fw(info.cfg_file);
		fw << templ;
	}
}
void project::build(bool release, bool obfuscate) {
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
	if (!info.c_standard.empty()) {
		c_compile_options += "-std=" + info.c_standard + " ";
	}
	if (!info.cpp_standard.empty()) {
		cpp_compile_options += "-std=" + info.cpp_standard + " ";
	}
	for (const auto &stdlib : info.stdlibs) {
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
	if (info.type != project_t::STATIC_LIBRARY) {
		linkcmd << linker << " -o \"./build/" << info.name << proj_fileext(info.type) << "\"";
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
	if (build_using(build_cmds, linkcmd.str(), "building " + info.name)) {
		std::cout << prettyErrorGeneral("failed building " + info.name, severity::ERROR) << std::endl;
		exit(-1);
	}
	std::ofstream flb(last_build_file);
	uint16_t build_data = static_cast<uint16_t>(obfuscate) << 1 | release;
	flb << build_data;
	std::cout << prettyErrorGeneral("\x1b[92mbuilt " + info.name + colReset, severity::INFO) << std::endl;
}
void project::post_build() {
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
		run_commands(prebuild_commands, info.name + " post-build commands");
	if (!postbuild_parallel_commands.empty())
		run_commands_parallel(postbuild_parallel_commands, info.name + " post-build commands (parallel)");
}
void project::run() const {
	if (info.type != project_t::STATIC_LIBRARY)
		system(("./build/" + info.name + proj_fileext(info.type)).c_str());
}
