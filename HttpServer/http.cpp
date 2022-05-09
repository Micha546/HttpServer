#include "http.h"


std::string Http::get_header(const std::string& header_name) const {
	if (m_headers.find(header_name) != m_headers.end()) {
		return m_headers.at(header_name);
	}

	return "";
}

void Http::add_header(const std::string& header_name, const std::string& value) {
	m_headers.insert({ header_name, value });
}

void Http::remove_header(const std::string& header_name) {
	if (has_header(header_name)) {
		m_headers.erase(header_name);
	}
}

std::string Http::to_string() const {
	std::string ret;
	ret += first_line_to_string();
	ret += headers_to_string();
	ret += CRLF;
	ret += data_to_string();
	return ret;
}

bool Http::parse(const std::string& http_string) {
	std::istringstream stream(http_string);
	std::string line;
	bool legal_flag = true;
	bool data_flag = false;
	bool first_iteration_flag = true;


	while (legal_flag && std::getline(stream, line, Http::LF) && !data_flag) {
		if (!parse_line(line)) {
			return false;
		}

		if (first_iteration_flag) {
			first_iteration_flag = false;
			legal_flag = parse_first_line(line);
		}
		else if (line.size() == 0) {
			data_flag = true;
			break;
		}
		else {
			legal_flag = parse_header(line);
		}
	}

	while (legal_flag && std::getline(stream, line, '\0')) {
		legal_flag = parse_data(line);
	}

	return legal_flag && !first_iteration_flag && data_flag;	//has to have first line and has to have \r\n after the headers
}

bool Http::parse_header(const std::string& header_string) {
	size_t pos = header_string.find_first_of(":");
	if(pos == std::string::npos) {
		return false;
	}

	m_headers.insert({ header_string.substr(0, pos), header_string.substr(pos + 1, std::string::npos) });
	return true;
}

bool Http::parse_data(const std::string& data_string) {
	m_data += data_string;
	return true;
}

bool Http::parse_version(const std::string& version_string) {
	auto splited = Utils::split(version_string, "/");
	if (splited.size() != 2) {
		return false;
	}

	if (splited[0] != "HTTP") {
		return false;
	}

	if (!Utils::is_number(splited[1])) {
		return false;
	}

	m_version = std::stof(splited[1]);
	return true;
}

std::string Http::headers_to_string() const {
	std::string ret;
	for (auto pair : m_headers) {
		ret += pair.first + ":" + pair.second + CRLF;
	}

	return ret;
}

std::string Http::data_to_string() const {
	return m_data;
}

std::string Http::version_to_string() const {
	std::string ret = "HTTP/" + std::to_string(m_version);
	ret.erase(ret.find_last_not_of('0') + 1, std::string::npos);
	return ret;
}

bool Http::parse_line(std::string& line) {
	if (line.back() != Http::CR) {
		return false;
	}

	line.pop_back();
	return true;
}
