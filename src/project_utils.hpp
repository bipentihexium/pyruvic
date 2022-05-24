#ifndef __PROJECT_UTILS_HPP__
#define __PROJECT_UTILS_HPP__

#include <string>
#include "parsing/par.hpp"

#if defined(__linux__)
constexpr const char *platform_idents[] = { "unix" };
#elif defined(_WIN32)
constexpr const char *platform_idents[] = { "win" };
#endif

constexpr const char *filehist_file = "./.pyr/filehist";
constexpr const char *filedeps_file = "./.pyr/filedeps";
constexpr const char *last_build_file = "./.pyr/last_build";
constexpr const char *objfile_ext = ".o";
enum class project_t { EXECUTABLE, STATIC_LIBRARY, DYNAMIC_LIBRARY };

extern std::string pyruvic_path;

class value_list;
struct project_info;

const value_list &get_val_list_by_platform(const subcategory &subcat, const std::string &name);
void replace_vars(const project_info &info, std::string &str);
std::string proj_fileext(project_t t);

void load_cfg();
void new_project(const std::string &);

#endif
