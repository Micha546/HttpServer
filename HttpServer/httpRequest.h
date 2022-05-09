#pragma once
#include "http.h"


class HttpRequest : public Http {
public:
	enum class Method {
		NONE, GET, HEAD, POST, PUT, _DELETE, TRACE, OPTIONS
	};
	
	HttpRequest();
	HttpRequest(const std::string& http_string);
	virtual ~HttpRequest() {}

	Method get_method() const { return m_method.get(); }
	void set_method(Method method) { m_method.set(method); }
	std::string get_method_name() const { return m_method.name(); }
	std::string get_url() const { return m_url; }
	void set_url(const std::string& url) { m_url = url; }
	const std::map<std::string, std::string>& get_query_params() const { return m_query_params; }
	std::map<std::string, std::string>& get_query_params() { return m_query_params; }
	std::string get_query_param(const std::string& param_name) const;
	float get_http_version() const { return m_version; }
	void add_query_param(const std::string& param_name, const std::string& value);
	void remove_query_param(const std::string& param_name);
	bool has_query_param(const std::string& param_name) const { return m_query_params.find(param_name) != m_query_params.end(); }

	friend std::ostream& operator<<(std::ostream& os, const HttpRequest& req) { return os << req.to_string(); }

private:
	class E_Method {
	public:
		E_Method() : e_method(Method::NONE) {}
		E_Method(Method method) : e_method(method) {}
		E_Method(const std::string& method_string) : E_Method() { e_method = parse(method_string); }

		Method get() const { return e_method; }
		void set(Method method) { e_method = method; }
		std::string name() const { return names[static_cast<int>(e_method)]; }
		std::string to_string() const { return name(); }
		static Method parse(const std::string& method_string);

		friend std::ostream& operator<<(std::ostream& os, const E_Method cls) { return os << cls.to_string(); }
	private:
		Method e_method;
		static constexpr const char* names[] = { "NONE", "GET", "HEAD", "POST", "PUT", "DELETE", "TRACE", "OPTIONS" };
		static constexpr int names_len = 8;
	};

	E_Method m_method = Method::GET;
	std::string m_url;
	std::map<std::string, std::string> m_query_params;

	virtual bool parse_first_line(const std::string& first_line_string) override;
	bool parse_method(const std::string& method_string);
	bool parse_url(const std::string& url_string);
	bool parse_query_params(const std::string& query_params_string);

	virtual std::string first_line_to_string() const override;
	std::string method_to_string() const;
	std::string url_to_string() const;
};

