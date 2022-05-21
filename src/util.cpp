#include "util.hpp"
#include <string>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __linux__
#endif

std::string get_exe_path() {
#ifdef _WIN32
	char buf[1024];
	int length = GetModuleFileNameA(nullptr, buf, 1024);
	if (length < 1024) {
		return std::filesystem::path(std::string(buf, length)).parent_path().string();
	}
#endif
#ifdef __linux__
	return std::filesystem::canonical("/proc/self/exe").parent_path().string();
#endif
	return "";
}

bool command_exists(const std::string &cmd) {
	if (std::filesystem::path(cmd).is_absolute())
		return std::filesystem::exists(cmd);
	std::string path(getenv("PATH"));
#ifdef _WIN32
	for (size_t i = 0, j; (j = path.find(';', i+1)) != std::string::npos; i = j) {
		std::filesystem::path p(path.substr(i+1, j-i-1));
		if (std::filesystem::exists(p + "/" + cmd + ".com"))
			return true;
		if (std::filesystem::exists(p + "/" + cmd + ".exe"))
			return true;
		if (std::filesystem::exists(p + "/" + cmd + ".bat"))
			return true;
	}
#endif
#ifdef __linux__
	for (size_t i = 0, j; (j = path.find(':', i+1)) != std::string::npos; i = j) {
		std::filesystem::path p(path.substr(i+1, j-i-1) + "/" + cmd);
		if (std::filesystem::exists(p))
			return true;
	}
#endif
	return false;
}
