#include "runtime_config.hpp"

#include <fstream>
#include <iostream>
#include "formatted_out.hpp"
#include "project_utils.hpp"

std::string c_compiler;
std::string cpp_compiler;
std::string linker;

bool file_history::was_updated(const std::string &file) const {
	if (!std::filesystem::exists(file)) {
		return true;
	}
	auto it = find(file);
	if (it == end())
		return true;
	std::filesystem::file_time_type t = std::filesystem::last_write_time(file);
	uint64_t now = static_cast<uint64_t>(t.time_since_epoch().count());
	bool updated = now > it->second;
	return updated;
}
bool file_history::was_updated(const std::string &file, const file_dependencies &deps, unsigned int depth) const {
	if (depth > 50) {
		std::cout << prettyErrorGeneral("Exceeded maximum dependency update search depth (" + std::to_string(depth) + ") - counting as updated", severity::WARN) << std::endl;
		return true;
	}
	if (was_updated(file)) {
		return true;
	}
	auto it = deps.find(file);
	if (it == deps.end()) {
		return false;
	}
	std::filesystem::path p(file);
	p = p.parent_path();
	for (const auto &d : it->second) {
		std::filesystem::path dp(p);
		dp.append(d);
		dp = "./" / std::filesystem::relative(dp);
		if (was_updated(dp.string(), deps, depth + 1)) {
			std::cout << file << " > " << dp.string() << " upd" << std::endl;
			return true;
		}
	}
	return false;
}
void file_history::update(const std::string &file) {
	if (std::filesystem::exists(file)) {
		std::filesystem::file_time_type t = std::filesystem::last_write_time(file);
		uint64_t now = static_cast<uint64_t>(t.time_since_epoch().count());
		(*this)[file] = now;
	}
}
bool file_history::load_saved(const std::string &file) {
	std::ifstream f(file);
	if (f.bad())
		return true;
	std::string str;
	while (std::getline(f, str)) {
		size_t split = str.rfind(' ');
		insert(std::make_pair(str.substr(0, split), std::stoull(str.substr(split + 1))));
	}
	return false;
}
bool file_history::save(const std::string &file) const {
	std::ofstream f(file);
	if (f.bad())
		return true;
	for (const auto &filedata : *this) {
		f << filedata.first << ' ' << filedata.second << std::endl;
	}
	return false;
}

void file_dependencies::save_c_cpp_deps(const std::string &file) {
	std::ifstream f(file);
	if (f.good()) {
		std::string s;
		std::vector<std::string> deps;
		while (std::getline(f, s)) {
			std::string no_whitespace(s);
			no_whitespace.erase(std::remove_if(no_whitespace.begin(), no_whitespace.end(), isspace), no_whitespace.end());
			if (no_whitespace.starts_with("#include")) {
				size_t start = s.find('"');
				if (start != std::string::npos) {
					size_t end = s.find('"', start + 1);
					deps.push_back(s.substr(start+1, end-start-1));
				}
			}
		}
		(*this)[file] = deps;
	}
}
bool file_dependencies::load_saved(const std::string &file) {
	std::ifstream f(file);
	if (f.bad())
		return true;
	std::string str;
	while (std::getline(f, str)) {
		std::vector<std::string> deps;
		size_t first = str.find(':');
		for (size_t i = first, j = str.find(':', first + 1); j != std::string::npos; i = j, j = str.find(':', i + 1)) {
			deps.push_back(str.substr(i+1, j-i-1));
		}
		insert(std::make_pair(str.substr(0, first), deps));
	}
	return false;
}
bool file_dependencies::save(const std::string &file) const {
	std::ofstream f(file);
	if (f.bad())
		return true;
	for (const auto &filedata : *this) {
		f << filedata.first << ':';
		for (const auto &dep : filedata.second) {
			f << dep << ':';
		}
		f << std::endl;
	}
	return false;
}

void dependency_info::load(category &cat) {
	for (auto &depinfo : cat) {
		dependency dep;
		dep.names.push_back(depinfo.first);
		for (const auto &alias : depinfo.second[""]["aliases:"]) {
			dep.names.push_back(alias);
		}
		dep.download_location = depinfo.second[""]["repo:"].empty() ? "" : depinfo.second[""]["repo:"][0];
		dep.build_sys = dependency::build_system_t::HEADERONLY;
		if (!depinfo.second[""]["build-system:"].empty()) {
			if (depinfo.second[""]["build-system:"][0] == "cmake") {
				dep.build_sys = dependency::build_system_t::CMAKE;
			} else if (depinfo.second[""]["build-system:"][0] == "pyruvic") {
				dep.build_sys = dependency::build_system_t::PYRUVIC;
			}
		}
		const value_list &incl_dir = get_val_list_by_platform(depinfo.second, "include-dir:");
		dep.include_dir = incl_dir.empty() ? "include/" : incl_dir[0];
		const value_list &syslibs = get_val_list_by_platform(depinfo.second, "link:");
		for (const auto &sl : syslibs)
			dep.syslibs.push_back(sl);
		const value_list &depends = get_val_list_by_platform(depinfo.second, "depends:");
		for (const auto &dep_dep : depends)
			dep.depends.push_back(dep_dep);
	}
}
const dependency *dependency_info::operator[](const std::string &dep) const {
	auto it = mapped_deps.find(dep);
	return it == mapped_deps.end() ? nullptr : it->second;
}
