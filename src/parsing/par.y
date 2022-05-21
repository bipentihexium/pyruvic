%language "c++"
%skeleton "lalr1.cc"
%define api.token.constructor
%define api.value.type variant
%define parse.error custom

%code requires {
#include "par.hpp"

#include <algorithm>
#include "lex.hpp"
#include "../formatted_out.hpp"
}
%code {
	tokenstream parserts;
	pyruvic_file parser_out;
	namespace yy { parser::symbol_type yylex(); }
#define M std::move
}

%token EOFTOK 0 "EOF"
%token CATEGORY "[category]" SUBCATEGORY "subcategory{" VALUE_PACK ">value_pack" VALUE_NAME "value_name:" VALUE "value"
%token COMMA "," CLOSE_BRACE "}"

%type<lextoken> EOFTOK CATEGORY SUBCATEGORY VALUE_PACK VALUE_NAME VALUE COMMA CLOSE_BRACE
%type<lextoken> error
%type<category> category
%type<category> subcategories
%type<subcategory> val_packs
%type<value_pack> named_vals
%type<value_list> vals
%type<value_list> vals_nonempty

%%

parserlib:
	categories EOFTOK;
categories:
	%empty
|	categories category;
category:
	"[category]" subcategories { parser_out.insert(std::make_pair($1.val, M($2))); };
subcategories:
	val_packs { $$ = category(); $$.insert(std::make_pair("", M($1))); }
|	subcategories "subcategory{" val_packs "}" { $$ = M($1); $$.insert(std::make_pair($2.val, M($3))); };
val_packs:
	named_vals { $$ = subcategory(); $$.insert(std::make_pair("", M($1))); }
|	val_packs ">value_pack" named_vals { $$ = M($1); $$.insert(std::make_pair($2.val, M($3))); };
named_vals:
	%empty { $$ = value_pack(); }
|	named_vals "value_name:" vals { $$ = M($1); $$.insert(std::make_pair($2.val, M($3))); };
vals:
	%empty { $$ = value_list(); }
|	vals_nonempty { $$ = M($1); }
vals_nonempty:
	"value" { $$ = value_list(); $$.push_back($1.val); }
|	vals_nonempty "," "value" { $$ = M($1); $$.push_back($3.val); };

%%

const token &nexttok() {
	if (parserts.pos >= parserts.toks.size() - 1 && parserts.pos < static_cast<size_t>(-1))
		return parserts.toks.back();
	return parserts.next();
}
yy::parser::symbol_type toSymbol(const token &t) {
	switch (t.type) {
	case token_t::EOFTOK: return yy::parser::make_EOFTOK(t); break;
	case token_t::ERROR: return yy::parser::make_EOFTOK(t); break;
	case token_t::CATEGORY: return yy::parser::make_CATEGORY(t); break;
	case token_t::SUBCATEGORY: return yy::parser::make_SUBCATEGORY(t); break;
	case token_t::VALUE_PACK: return yy::parser::make_VALUE_PACK(t); break;
	case token_t::VALUE_NAME: return yy::parser::make_VALUE_NAME(t); break;
	case token_t::VALUE: return yy::parser::make_VALUE(t); break;
	case token_t::COMMA: return yy::parser::make_COMMA(t); break;
	case token_t::CLOSE_BRACE: return yy::parser::make_CLOSE_BRACE(t); break;
	default: return yy::parser::make_EOFTOK(t);
	}
}
yy::parser::symbol_type yy::yylex() {
	return toSymbol(nexttok());
}

void yy::parser::report_syntax_error(const context& yyctx) const {
	std::string msg("");
	symbol_kind_type expected[20];
	int n = yyctx.expected_tokens(expected, 20);
	if (n > 0) {
		msg += "Expected ";
		for (int i = 0; i < n; ++i) {
			if (i != 0)
				msg += ", ";
			if (i == n - 1)
				msg += "or ";
			msg += symbol_name(expected[i]);
		}
	}
	const symbol_type &la = yyctx.lookahead();
	if (!la.empty()) {
		if (n > 0)
			msg += ", but found ";
		else
			msg += ": Unexpected token: ";
		msg += symbol_name(la.kind());
	}
	msg += "!";
	const lextoken &lt = la.value.as<lextoken>();
	std::cout << prettyError(msg, severity::ERROR, lt.loc, { highlight("here", severity::ERROR, lt.loc) }) << std::endl;
}

void yy::parser::error(const std::string &msg) {
	std::cout << prettyErrorGeneral(msg, severity::ERROR);
}

pyruvic_file parse(const tokenstream &t) {
	parser_out.clear();
	parserts = t;
	parserts.pos = static_cast<size_t>(-1);
	yy::parser p;
	p.parse();
	return std::move(parser_out);
}