#ifndef __RUNTIME_CONFIG_HPP__
#define __RUNTIME_CONFIG_HPP__

#include <inttypes.h>
#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include "parsing/par.hpp"
#include "project_utils.hpp"

extern std::string c_compiler;
extern std::string cpp_compiler;
extern std::string linker;

struct project_info {
public:
	std::string name;
	project_t type;
	std::string macroname;
	int ver_maj, ver_min, ver_pat, ver_twe;
	std::string ver_name;
	std::string cfg_file;

	std::string c_standard;
	std::string cpp_standard;
	std::vector<std::string> stdlibs;
};

class file_dependencies : public std::map<std::string, std::vector<std::string>> {
public:
	void save_c_cpp_deps(const std::string &file);
	bool load_saved(const std::string &file);
	bool save(const std::string &file) const;
};
class file_history : public std::map<std::string, uint64_t> {
public:
	bool was_updated(const std::string &file) const;
	bool was_updated(const std::string &file, const file_dependencies &deps, unsigned int depth=0) const;
	void update(const std::string &file);
	bool load_saved(const std::string &file);
	bool save(const std::string &file) const;
};
class dependency {
public:
	enum build_system_t { PYRUVIC, CMAKE, HEADERONLY };

	std::vector<std::string> names;
	std::string download_location;
	build_system_t build_sys;
	std::string include_dir;
	std::vector<std::string> syslibs;
	std::vector<std::string> depends;
};
class dependency_info {
public:
	void load(category &cat);
	const dependency *operator[](const std::string &dep) const;
private:
	std::vector<dependency> deps;
	std::unordered_map<std::string, dependency *> mapped_deps;
};

#endif
