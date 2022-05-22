#ifndef __RUNTIME_CONFIG_HPP__
#define __RUNTIME_CONFIG_HPP__

#include <inttypes.h>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

extern std::string c_compiler;
extern std::string cpp_compiler;
extern std::string linker;

namespace project {
	extern std::string name;
	enum class project_t { EXECUTABLE, STATIC_LIBRARY, DYNAMIC_LIBRARY };
	extern project_t type;
	extern std::string macroname;
	extern int ver_maj, ver_min, ver_pat, ver_twe;
	extern std::string ver_name;
	extern std::string cfg_file;

	extern std::string c_standard;
	extern std::string cpp_standard;
}

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

#endif
