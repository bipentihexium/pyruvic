#include "util.hpp"
#include <string>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
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
