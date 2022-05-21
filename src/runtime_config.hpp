#ifndef __RUNTIME_CONFIG_HPP__
#define __RUNTIME_CONFIG_HPP__

#include <string>

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

#endif
