#ifndef __FORMATTED_OUT_HPP__
#define __FORMATTED_OUT_HPP__

#include <fstream>
#include <initializer_list>
#include <limits>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include "parsing/lex.hpp"

enum class severity {
	DEBUG, INFO, NOTE, WARN, ERROR, FATAL
};
inline std::string sev2col(severity sev) {
	static const std::string table[] = { "\x1b[95m", "\x1b[97m", "\x1b[96m", "\x1b[93m", "\x1b[91m\x1b[1m", "\x1b[31m\x1b[1m" };
	return table[static_cast<size_t>(sev)];
}
inline std::string sev2str(severity sev) {
	static const std::string table[] = { "debug", "info", "note", "warn", "error", "fatal error" };
	return table[static_cast<size_t>(sev)];
}
inline char sev2ul(severity sev) {
	static const char table[] = { '.', '=', '-', '~', '^', 'v' };
	return table[static_cast<size_t>(sev)];
}
inline const std::string colReset = "\x1b[0m";
class highlight {
public:
	std::string msg;
	severity sev;
	program_location loc;
	inline highlight(const std::string &msg, severity sev, const program_location &loc) : msg(msg), sev(sev), loc(loc) { }
};
inline std::map<std::string, std::string> cachedfiles;
class prettyErrorGeneral {
public:
	std::string msg;
	severity sev;
	inline prettyErrorGeneral(const std::string &msg, severity sev) : msg(msg), sev(sev) { }
};
inline std::ostream &operator<<(std::ostream &o, const prettyErrorGeneral &err) {
	o << sev2col(err.sev) << sev2str(err.sev) << ": " << err.msg << colReset;
	return o;
}
class prettyError {
public:
	std::string msg;
	severity sev;
	program_location loc;
	std::vector<highlight> highlights;
	inline prettyError(const std::string &msg, severity sev, const program_location &loc, const std::initializer_list<highlight> &h) :
		msg(msg), sev(sev), loc(loc), highlights(h) { }
};
inline std::ostream &operator<<(std::ostream &o, const prettyError &err) {
	o << sev2col(err.sev) << sev2str(err.sev) << ": " << err.msg << colReset << std::endl <<
		"in " << err.loc.file << " " << err.loc.line << ":" << err.loc.column << std::endl;
	auto it = cachedfiles.find(err.loc.file);
	std::stringstream fss;
	if (it == cachedfiles.end()) {
		std::ifstream f(err.loc.file);
		fss << f.rdbuf();
		cachedfiles.insert(std::make_pair(err.loc.file, fss.str()));
	} else {
		fss = std::stringstream(it->second);
	}
	for (size_t i = 1; i < err.loc.line; ++i) {
		fss.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	std::string line;
	for (size_t i = err.loc.line; i <= err.loc.endline; ++i) {
		std::getline(fss, line);
		std::string underline(line.size(), ' ');
		for (size_t i = 0; i < line.size(); ++i) {
			if (line[i] == '\t') {
				underline[i] = '\t';
			}
		}
		size_t lastchar = line.size();
		std::map<size_t, std::string> colorinserts;
		std::map<size_t, const highlight *> sortedh;
		for (const highlight &h : err.highlights) {
			if (h.loc.line <= i && h.loc.endline >= i) {
				size_t startcol = h.loc.line == i ? h.loc.column-1 : 0;
				size_t endcol = h.loc.endline == i ? h.loc.endcolumn-1 : line.size();
				if (startcol != endcol) {
					colorinserts[startcol] = sev2col(h.sev);
					auto iter = colorinserts.find(endcol);
					if (iter == colorinserts.end())
						colorinserts.insert(std::make_pair(endcol, colReset));
				}
				if (h.loc.endline == i) {
					sortedh.insert(std::make_pair(h.loc.line == h.loc.endline ? h.loc.column-1 : 0, &h));
					if (h.loc.endcolumn >= lastchar)
						lastchar = h.loc.endcolumn + 1;
				}
			}
		}
		std::string cline(line);
		cline += std::string(lastchar - line.size(), ' ');
		underline += std::string(lastchar - line.size(), ' ');
		for (const highlight &h : err.highlights) {
			if (h.loc.line <= i && h.loc.endline >= i) {
				size_t startcol = h.loc.line == i ? h.loc.column-1 : 0;
				size_t endcol = h.loc.endline == i ? h.loc.endcolumn-1 : line.size();
				for (size_t j = startcol; j < endcol; ++j)
					underline[j] = sev2ul(h.sev);
			}
		}
		for (auto iter = colorinserts.rbegin(); iter != colorinserts.rend(); ++iter) {
			cline.insert(iter->first, iter->second);
			underline.insert(iter->first, iter->second);
		}
		size_t ilen = 1;
		for (size_t l = i; l /= 10; ++ilen) { }
		o << i << "| " << cline << std::endl;
		std::vector<std::string> labels;
		o << std::string(ilen, ' ') << "| " << underline << std::endl;
		std::vector<size_t> blocks;
		std::vector<std::string> lines;
		for (auto iter = sortedh.rbegin(); iter != sortedh.rend(); ++iter) {
			size_t ln;
			for (ln = lines.size(); ln != 0 && iter->first + iter->second->msg.size() + 1 < blocks[ln-1]; --ln) { }
			if (ln >= lines.size()) {
				blocks.push_back(iter->first);
				lines.push_back(std::string(iter->first, ' ') + sev2col(iter->second->sev) + iter->second->msg + colReset);
				for (size_t i = 0; i < iter->first; ++i) {
					if (line[i] == '\t') {
						lines.back()[i] = '\t';
					}
				}
			} else {
				blocks[ln] = iter->first;
				for (size_t j = 0; j < iter->second->msg.size(); ++j)
					lines[ln][iter->first + j] = iter->second->msg[j];
				if (lines[ln][iter->first + iter->second->msg.size()] != '\x1b')
					lines[ln].insert(iter->first + iter->second->msg.size(), colReset);
				lines[ln].insert(iter->first, sev2col(iter->second->sev));
			}
			for (size_t j = 0; j < ln; ++j) {
				blocks[j] = iter->first;
				if (lines[j][iter->first] == ' ')
					lines[j][iter->first] = '|';
				if (lines[j][iter->first + 1] != '\x1b')
					lines[j].insert(iter->first + 1, colReset);
				lines[j].insert(iter->first, sev2col(iter->second->sev));
			}
		}
		for (size_t j = 0; j < lines.size(); ++j) {
			o << std::string(ilen, ' ') << "| " << lines[j];
			if (j < lines.size() - 1)
				o << std::endl;
		}
	}
	return o;
}

#endif
