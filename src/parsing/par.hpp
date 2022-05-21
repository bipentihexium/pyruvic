#ifndef __PAR_HPP__
#define __PAR_HPP__

#include <string>
#include <unordered_map>
#include <vector>
#include "lex.hpp"

class value_list : public std::vector<std::string> { };
class subcategory : public std::unordered_map<std::string, value_list> { };
class category : public std::unordered_map<std::string, subcategory> { };
class pyruvic_file : public std::unordered_map<std::string, category> { };

pyruvic_file parse(const tokenstream &t);

#endif
