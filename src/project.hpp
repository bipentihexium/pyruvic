#ifndef __PROJECT_HPP__
#define __PROJECT_HPP__

#include <string>
#include <vector>
#include "project_utils.hpp"
#include "runtime_config.hpp"

class project {
public:
	project_info info;

	void load(bool release, bool obfuscate);
	void clean_build_files() const;
	void pre_build();
	void build(bool release, bool obfuscate);
	void post_build();
	void run() const;
private:
	file_history hist;
	file_dependencies fdeps;
	std::vector<std::string> prebuild_commands;
	std::vector<std::string> prebuild_parallel_commands;
	std::vector<std::string> postbuild_commands;
	std::vector<std::string> postbuild_parallel_commands;
	std::vector<std::pair<std::string, std::string>> depends;
};

#endif
