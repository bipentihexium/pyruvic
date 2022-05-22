#ifndef __PROJECT_UTILS_HPP__
#define __PROJECT_UTILS_HPP__

#include <string>

void load_cfg();
void new_project(const std::string &);
void load_project(bool release, bool obfuscate);
void clean_build_files();
void pre_build();
void build_project(bool release, bool obfuscate);
void post_build();
void run_project();

#endif
