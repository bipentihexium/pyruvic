#include "runtime_config.hpp"

#include <fstream>
#include <iostream>
#include "formatted_out.hpp"

std::string c_compiler;
std::string cpp_compiler;
std::string linker;

namespace project {
	std::string name;
	project_t type;
	std::string macroname;
	int ver_maj, ver_min, ver_pat, ver_twe;
	std::string ver_name;
	std::string cfg_file;

	std::string c_standard;
	std::string cpp_standard;
	std::vector<std::string> stdlibs;
}

bool file_history::was_updated(const std::string &file) const {
	auto it = find(file);
	if (it == end())
		return true;
	std::filesystem::file_time_type t = std::filesystem::last_write_time(file);
	uint64_t now = static_cast<uint64_t>(t.time_since_epoch().count());
	return now > it->second;
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
		if (was_updated(dp, deps, depth + 1))
			return true;
	}
	return false;
}
void file_history::update(const std::string &file) {
	std::filesystem::file_time_type t = std::filesystem::last_write_time(file);
	uint64_t now = static_cast<uint64_t>(t.time_since_epoch().count());
	(*this)[file] = now;
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
