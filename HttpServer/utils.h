#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>

class Utils {
public:
	static std::vector<std::string> split(const std::string& s, const std::string& delimiter);
	static bool is_number(const std::string& s);
	static bool is_int(const std::string& s);
	static bool is_set_empty(fd_set const* check);
	static bool is_file_exists(const std::string& path);
};