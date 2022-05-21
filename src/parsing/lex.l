%{
#include "lex.hpp"
#include <stdio.h>
#include <iostream>
#include <string>
#include "../formatted_out.hpp"

unsigned int prevline;
unsigned int prevcol;
unsigned int line;
unsigned int col;
void reset() {
	prevline = 1;
	prevcol = 1;
	line = 1;
	col = 1;
}
void move(const char *txt) {
	prevline = line;
	prevcol = col;
	const char *i = txt-1;
	while (*++i) {
		if (*i == '\n') {
			++line;
			col = 1;
		} else if (*i == '\r') {
			col = 1;
		} else {
			++col;
		}
	}
}
std::string lexfile;
tokenstream ts;
#define CURR_LOC program_location(lexfile, prevline, prevcol, line, col)
#define tsempl(t) ts.emplace(CURR_LOC, t, yytext)
%}

%%

[ \t]+ { move(yytext); }
\r?\n { move(yytext); }
, { move(yytext); tsempl(token_t::COMMA); }
#.* { move(yytext); }
\[[a-z0-9\-]+\] { move(yytext); tsempl(token_t::CATEGORY); }
>\ ?[a-z0-9\-]+ { move(yytext); tsempl(token_t::SUBCATEGORY); }
[^ \r\n\t]+: { move(yytext); tsempl(token_t::VALUE_NAME); }
[^ \r\n\t,]+ { move(yytext); tsempl(token_t::VALUE); }
. { move(yytext); std::cout << prettyError(std::string("Unexpected character \"") + yytext + "\"", severity::ERROR, CURR_LOC,
		{ highlight("unexpected character here", severity::ERROR, CURR_LOC) }); }

%%

int yyFlexLexer::yywrap() { return 1; }

tokenstream lex(const std::string &file, std::istream &s) {
	FlexLexer *lexer = new yyFlexLexer();
	lexer->yyrestart(s);
	ts.pos = 0;
	ts.toks.clear();
	ts.toks.shrink_to_fit();
	reset();
	lexfile = file;
	lexer->yylex();
	ts.emplace(program_location(lexfile, prevline, prevcol, line, col), token_t::EOFTOK, "");
	return ts;
}

program_location::program_location(const std::string &file) : program_location(file, 1, 1, 1, 1) { }
program_location::program_location(const std::string &file, unsigned int line, unsigned int column,
	unsigned int endcolumn) : program_location(file, line, column, line, endcolumn) { }
program_location::program_location(const std::string &file, unsigned int line, unsigned int column,
	unsigned int endline, unsigned int endcolumn) : file(file), line(line), column(column),
	endline(endline), endcolumn(endcolumn) { }
program_location::program_location(const program_location &start, const program_location &end) :
	program_location(start.file, start.line, start.column, end.endline, end.endcolumn) { }
program_location &program_location::operator=(const program_location &o) {
	file = o.file;
	line = o.line;
	column = o.column;
	endline = o.endline;
	endcolumn = o.endcolumn;
	return *this;
}
program_location::program_location(const program_location &o) : program_location(o, o) { }
token::token() : token(program_location(""), token_t::ERROR) { }
token::token(const program_location &loc, token_t type, std::string val) : type(type), val(val),
	loc(loc) { }
token::token(const token &t) : token(t.loc, t.type, t.val) { }
token &token::operator=(const token &o) {
	type = o.type;
	val = o.val;
	loc = o.loc;
	return *this;
}

void tokenstream::push(const token &t) { toks.push_back(t); }
void tokenstream::emplace(const program_location &loc, token_t type, std::string val) {
	toks.emplace_back(loc, type, val);
}
const token &tokenstream::look() const { return toks[pos]; }
const token &tokenstream::next() { return toks[++pos]; }
