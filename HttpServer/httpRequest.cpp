#include "httpRequest.h"


HttpRequest::Method HttpRequest::E_Method::parse(const std::string& method_string) {
	Method meth = Method::NONE;
	for (int i = 0; i < names_len; ++i) {
		if (method_string == names[i]) {
			meth = static_cast<Method>(i);
			break;
		}
	}

	return meth;
}

HttpRequest::HttpRequest() : Http() {};

HttpRequest::HttpRequest(const std::string& http_string) : Http() {
	m_bad_http = !parse(http_string);
}

std::string HttpRequest::get_query_param(const std::string& param_name) const {
	if (m_query_params.find(param_name) != m_query_params.end()) {
		return m_query_params.at(param_name);
	}

	return "";
}

void HttpRequest::remove_query_param(const std::string& param_name) {
	if (has_query_param(param_name)) {
		m_query_params.erase(param_name);
	}
}

void HttpRequest::add_query_param(const std::string& param_name, const std::string& value) {
	m_query_params.insert({ param_name, value });
}

bool HttpRequest::parse_first_line(const std::string& first_line_string) {
	auto splited = Utils::split(first_line_string, " ");
	bool is_good = true;
	if (splited.size() != 3) {
		return false;
	}

	is_good = parse_method(splited[0]);
	is_good = parse_url(splited[1]) && is_good;
	is_good = parse_version(splited[2]) && is_good;
	return is_good;
}

bool HttpRequest::parse_method(const std::string& method_string) {
	m_method = E_Method(method_string);
	return m_method.get() != Method::NONE;
}

bool HttpRequest::parse_url(const std::string& url_string) {
	if (url_string.find('?') == std::string::npos) {
		m_url = url_string;
	}
	else {
		m_url = url_string.substr(0, url_string.find('?'));
		return parse_query_params(url_string.substr(url_string.find('?') + 1, std::string::npos));
	}

	return true;
}

bool HttpRequest::parse_query_params(const std::string& parse_query_string) {
	auto splited = Utils::split(parse_query_string, "&");
	for (auto s : splited) {
		m_query_params.insert({ s.substr(0, s.find('=')), s.substr(s.find('=') + 1, std::string::npos) });
	}

	return true;
}

std::string HttpRequest::first_line_to_string() const {
	return method_to_string() + " " + url_to_string() + " " + version_to_string() + Http::CRLF;
}

std::string HttpRequest::method_to_string() const {
	return m_method.to_string();
}

std::string HttpRequest::url_to_string() const {
	std::string ret = m_url;
	if (m_query_params.size() > 0) {
		ret += "?";
		for (auto p : m_query_params) {
			ret += p.first + "=" + p.second + "&";
		}

		ret.pop_back();
	}

	return ret;
}