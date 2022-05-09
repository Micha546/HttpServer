#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include "utils.h"

class Http {
public:
	bool is_bad() { return m_bad_http; }
	const std::map<std::string, std::string>& get_headers() const { return m_headers; }
	std::map<std::string, std::string>& get_headers() { return m_headers; }
	std::string get_header(const std::string& header_name) const;
	std::string get_data() const { return m_data; }
	void add_header(const std::string& header_name, const std::string& value);
	void remove_header(const std::string& header_name);
	void set_data(const std::string& data) { m_data = data; }
	void set_version(float version) { m_version = version; }
	bool has_header(const std::string& header_name) const { return m_headers.find(header_name) != m_headers.end(); }
	std::string to_string() const;

protected:
	std::map<std::string, std::string> m_headers;
	std::string m_data;
	float m_version;
	bool m_bad_http;

	Http() : m_version(0.f), m_bad_http(true) {}

	bool parse(const std::string& http_string);
	virtual bool parse_first_line(const std::string& first_line_string) = 0;
	bool parse_header(const std::string& header_string);
	bool parse_data(const std::string& data_string);
	bool parse_version(const std::string& version_string);
	
	virtual std::string first_line_to_string() const = 0;
	std::string headers_to_string() const;
	std::string data_to_string() const;
	std::string version_to_string() const;

	static constexpr char CR = '\r';
	static constexpr char LF = '\n';
	static constexpr const char* CRLF = "\r\n";

private:
	bool parse_line(std::string& line);
};