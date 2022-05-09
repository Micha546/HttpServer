#include "utils.h"


std::vector<std::string> Utils::split(const std::string& s, const std::string& delimiter) {
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	std::string token;
	std::vector<std::string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
		token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(s.substr(pos_start));
	return res;
}

bool Utils::is_number(const std::string& s) {
	if (s.find_first_not_of("-.0123456789") != std::string::npos) {
		return false;
	}

	size_t minus_count = std::count(s.begin(), s.end(), '-');
	size_t point_count = std::count(s.begin(), s.end(), '.');
	if (minus_count > 1) {
		return false;
	}

	if (minus_count == 1 && *s.begin() != '-') {
		return false;
	}

	if (point_count > 1) {
		return false;
	}

	if (point_count == 1 && *s.begin() == '.') {
		return false;
	}

	return true;
}

bool Utils::is_int(const std::string& s) {
	if (!is_number(s)) {
		return false;
	}

	if (s.find('.') != std::string::npos) {
		return false;
	}

	return true;
}

bool Utils::is_set_empty(fd_set const* check) {
	if (check == NULL) {
		return true;
	}

	return check->fd_count == 0;
}

bool Utils::is_file_exists(std::string path) {
	std::ifstream file(path);
	bool exists = file.is_open();
	file.close();
	return exists;
}