#ifndef __CMDUTILS_HPP__
#define __CMDUTILS_HPP__

#include <string>
#include <vector>

bool run_commands(const std::vector<std::string> &cmds, const std::string &note);
bool run_commands_parallel(const std::vector<std::string> &cmds, const std::string &note);
bool build_using(const std::vector<std::string> &compile_cmds, const std::string &link_cmd, const std::string &note);

#endif
