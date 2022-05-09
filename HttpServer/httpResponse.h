#pragma once
#include "http.h"

class HttpResponse : public Http {
public:
	enum class Status {
		NONE, CONTINUE, OK, CREATED, ACCEPTED, NO_CONTENT, BAD_REQUEST, NOT_FOUND,
		METHOD_NOT_ALLOWED, INTERNAL_SERVER_ERROR, NOT_IMPLEMENTED, HTTP_VERSION_NOT_SUPPORTED
	};

	HttpResponse();
	HttpResponse(const std::string& http_string);
	virtual ~HttpResponse() {}

	Status get_status() const { return m_status.get(); }
	void set_status(const Status status) { m_status.set(status); }
	std::string get_status_name() const { return m_status.name(); }
	int get_status_code() const { return m_status.code(); }

	friend std::ostream& operator<<(std::ostream& os, const HttpResponse& res) { return os << res.to_string(); }

private:
	class E_Status {
	public:
		E_Status() : e_status(Status::NONE) {}
		E_Status(Status status) : e_status(status) {}
		E_Status(const std::string& status_string) : E_Status() { e_status = parse(status_string); }

		static Status parse(const std::string& status_string);
		Status get() const { return e_status; }
		void set(Status status) { e_status = status; }
		int code() const { return codes[static_cast<int>(e_status)]; }
		std::string name() const { return names[static_cast<int>(e_status)]; }
		static int get_code(Status status) { return codes[static_cast<int>(status)]; }
		std::string to_string() const { return std::to_string(code()) + " " + name(); }

		friend std::ostream& operator<<(std::ostream& os, const E_Status& s) { return os << s.to_string(); }

	private:
		Status e_status = Status::NONE;
		static constexpr const char* names[] =
		{ "NONE",
		"Continue",
		"OK", "Created", "Accepted", "No Content",
		"Bad Request", "Not Found", "Method Not Allowed",
		"Internal Server Error", "Not Implemented", "HTTP Version Not Supported" };
		static constexpr int codes[] = { 0, 100, 200, 201, 202, 204, 400, 404, 405, 500, 501, 505 };
		static constexpr int len = 12;
	};

	E_Status m_status = Status::NONE;

	virtual bool parse_first_line(const std::string& first_line_string);
	bool parse_status_name(const std::string& status_name_string);
	bool check_status_code_equals(const std::string& status_code_string);
	virtual std::string first_line_to_string() const;
	std::string status_to_string() const { return m_status.to_string(); }
};