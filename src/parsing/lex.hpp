#ifndef __LEX_HPP__
#define __LEX_HPP__

#include <fstream>
#include <string>
#include <vector>

enum class token_t {
	EOFTOK=0,
	ERROR=256,
	CATEGORY, SUBCATEGORY, VALUE_PACK, VALUE_NAME, VALUE,
	COMMA, CLOSE_BRACE
};
inline std::string to_str(token_t t) {
	switch (t) {
	case token_t::EOFTOK: return "EOF";
	case token_t::ERROR: return "ERROR";
	case token_t::CATEGORY: return "CATEGORY";
	case token_t::SUBCATEGORY: return "SUBCATEGORY";
	case token_t::VALUE_PACK: return "VALUE_PACK";
	case token_t::VALUE_NAME: return "VALUE_NAME";
	case token_t::VALUE: return "VALUE";
	case token_t::COMMA: return "COMMA";
	case token_t::CLOSE_BRACE: return "CLOSE_BRACE";
	}
	return "";
}

class program_location {
public:
	std::string file;
	unsigned int line;
	unsigned int column;
	unsigned int endline;
	unsigned int endcolumn;

	program_location(const std::string &file);
	program_location(const std::string &file, unsigned int line, unsigned int column, unsigned int endcolumn);
	program_location(const std::string &file, unsigned int line, unsigned int column, unsigned int endline, unsigned int endcolumn);
	program_location(const program_location &start, const program_location &end);
	program_location(const program_location &o);
	program_location &operator=(const program_location &o);
};
class token {
public:
	token_t type;
	std::string val;
	program_location loc;

	token();
	token(const program_location &loc, token_t type, std::string val="");
	token(const token &t);
	token &operator=(const token &o);
};
using lextoken = token;
class tokenstream {
public:
	std::vector<token> toks;
	size_t pos;

	void push(const token &t);
	void emplace(const program_location &loc, token_t type, std::string val="");
	const token &look() const;
	const token &next();
};
tokenstream lex(const std::string &file, std::istream &s);

#endif