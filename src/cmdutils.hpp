#ifndef __CMDUTILS_HPP__
#define __CMDUTILS_HPP__

#include <string>
#include <vector>

bool build_using(const std::vector<std::string> &compile_cmds, const std::string &link_cmd, const std::string &note);

#endif
