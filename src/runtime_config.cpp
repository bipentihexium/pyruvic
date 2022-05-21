#include "runtime_config.hpp"

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
}
