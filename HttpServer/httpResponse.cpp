#include "httpResponse.h"

HttpResponse::Status HttpResponse::E_Status::parse(const std::string& status_string) {
	Status status = Status::NONE;
	for (int i = 0; i < len; ++i) {
		if (status_string == names[i]) {
			status = static_cast<Status>(i);
			break;
		}
	}

	return status;
}

HttpResponse::HttpResponse() : Http() {}

HttpResponse::HttpResponse(const std::string& http_string) : HttpResponse() {
	m_bad_http = !parse(http_string);
}

bool HttpResponse::parse_first_line(const std::string& first_line_string) {
	auto splited = Utils::split(first_line_string, " ");
	if (splited.size() != 3) {
		return false;
	}

	if (!parse_version(splited[0])) {
		return false;
	}

	if (!parse_status_name(splited[2])) {
		return false;
	}

	return check_status_code_equals(splited[1]);
}

bool HttpResponse::parse_status_name(const std::string& status_name_string) {
	m_status = E_Status::parse(status_name_string);
	return m_status.get() != Status::NONE;
}

bool HttpResponse::check_status_code_equals(const std::string& status_code_string) {
	if (!Utils::is_int(status_code_string)) {
		return false;
	}

	int parsed_code = std::stoi(status_code_string);
	return parsed_code == m_status.code();
}

std::string HttpResponse::first_line_to_string() const {
	return version_to_string() + " " + status_to_string() + Http::CRLF;
}
