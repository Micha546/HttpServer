#include "server.h"
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

Server::Server() : m_listen_socket(INVALID_SOCKET), m_sockets() {
	m_listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == m_listen_socket)
	{
		WSACleanup();
		throw std::exception("error in socket() function");
	}
}

void Server::listen_forever(int port) {
	listen_setup(port);
	std::cout << "Server is listening on port " << port << std::endl;

	while (true) {
		handle_timeouts();
		accept_connections();

		fd_set wait_recv, wait_send;
		int nfd = create_sets(&wait_recv, &wait_send);

		for (auto& sock_stat : m_sockets) {
			if (FD_ISSET(sock_stat._socket, &wait_recv)) {
				receive_message(sock_stat);
			}

			if (FD_ISSET(sock_stat._socket, &wait_send)) {
				send_message(sock_stat);
			}
		}
	}
}

Server::SocketStatus::SocketStatus()
	: _socket(INVALID_SOCKET), _socket_info({}), _recv_status(SocketStatus::ReceiveStatus::EMPTY), _send_status(SocketStatus::SendStatus::EMPTY),
	_recv_str(""), _last_msg_time(0), _res_queue() {}

void Server::SocketStatus::init(const SOCKET& socket, const sockaddr_in& socket_info) {
	_socket = socket;
	_socket_info = socket_info;
	_recv_status = SocketStatus::ReceiveStatus::RECIVE_HEADERS;
	_send_status = SocketStatus::SendStatus::IDLE;
	_recv_str = "";
	_last_msg_time = time(NULL);
}

bool Server::SocketStatus::try_init(const SOCKET& socket, const sockaddr_in& socket_info) {
	if (!is_empty()) {
		return false;
	}

	init(socket, socket_info);
	return true;
}

void Server::SocketStatus::clear() {
	closesocket(_socket);
	_socket = INVALID_SOCKET;
	_socket_info = {};
	_recv_status = ReceiveStatus::EMPTY;
	_send_status = SendStatus::EMPTY;
	_recv_str = "";
	_last_msg_time = 0;
	_res_queue = std::queue<HttpResponse>();
}

std::string Server::SocketStatus::ip_port_string() const {
	std::string ret = inet_ntoa(_socket_info.sin_addr);
	return ret + ":" + std::to_string(ntohs(_socket_info.sin_port));
}

void Server::listen_setup(int port) {
	sockaddr_in server_service;
	server_service.sin_family = AF_INET;
	server_service.sin_addr.s_addr = INADDR_ANY;
	server_service.sin_port = htons(port);

	if (SOCKET_ERROR == bind(m_listen_socket, (SOCKADDR*)&server_service, sizeof(server_service))) {
		closesocket(m_listen_socket);
		WSACleanup();
		throw std::exception("error in bind() function");
	}

	if (SOCKET_ERROR == listen(m_listen_socket, 5)) {
		closesocket(m_listen_socket);
		WSACleanup();
		throw std::exception("error in listen() function");
	}
}

void Server::handle_timeouts(double timeout_seconds) {
	static time_t last_time = 0;	// no need to check same second twice
	time_t now = time(NULL);
	if (now == last_time) {
		return;
	}

	last_time = now;
	for (auto& sock_stat : m_sockets) {
		if (!sock_stat.is_empty() && difftime(now, sock_stat._last_msg_time) > timeout_seconds) {
			std::cout << "Server: Client " << sock_stat.ip_port_string() << " is disconnected due to timeout." << std::endl;
			sock_stat.clear();
		}
	}
}

int Server::select_wrapper(fd_set* recv_set, fd_set* send_set) {
	if (Utils::is_set_empty(recv_set) && Utils::is_set_empty(send_set)) {
		return 0;
	}

	timeval timeout = { 0, 0 };		// we dont want to wait at all if there are no fd that are ready
	int nfd = select(0, recv_set, send_set, NULL, &timeout);
	if (nfd == SOCKET_ERROR)
	{
		std::cout << WSAGetLastError() << std::endl;
		WSACleanup();
		throw std::exception("Error at select()");
	}

	return nfd;
}

int Server::create_sets(fd_set* recv_set, fd_set* send_set) {
	FD_ZERO(recv_set);
	FD_ZERO(send_set);
	for (auto& sock_stat : m_sockets) {
		if (sock_stat._recv_status == SocketStatus::ReceiveStatus::RECIVE_HEADERS || 
			sock_stat._recv_status == SocketStatus::ReceiveStatus::RECIVE_DATA) {
			FD_SET(sock_stat._socket, recv_set);
		}

		if (sock_stat._send_status == SocketStatus::SendStatus::SEND) {
			FD_SET(sock_stat._socket, send_set);
		}
	}

	return select_wrapper(recv_set, send_set);
}

void Server::accept_connections() {
	fd_set listen_set;
	FD_ZERO(&listen_set);
	FD_SET(m_listen_socket, &listen_set);

	if (select_wrapper(&listen_set, NULL) && FD_ISSET(m_listen_socket, &listen_set)) {
		struct sockaddr_in from;
		int fromLen = sizeof(from);
		unsigned long flag = 1;

		SOCKET msgSocket = accept(m_listen_socket, (struct sockaddr*)&from, &fromLen);
		if (INVALID_SOCKET == msgSocket) {
			std::cout << "Server: " << WSAGetLastError() << std::endl;
			return;
		}

		std::cout << "Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << std::endl;
		if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0) {
			std::cout << "Server: Error at ioctlsocket(): " << WSAGetLastError() << std::endl;
		}

		if (add_socket(msgSocket, from) == false) {
			std::cout << "\t\tToo many connections, dropped!" << std::endl;
			closesocket(msgSocket);
		}
	}
}

bool Server::add_socket(const SOCKET& socket, const sockaddr_in& socket_info) {
	for (auto& sock_stat : m_sockets) {
		if (sock_stat.try_init(socket, socket_info)) {
			return true;
		}
	}

	return false;
}

bool Server::recv_wrapper(SocketStatus& sock_stat) {
	static const std::string HEADER_BREAK_LINE = std::string(Http::CRLF) + std::string(Http::CRLF);
	char recv_buff[sock_stat.BUFF_SIZE];
	int bytes_recv = recv(sock_stat._socket, recv_buff, sock_stat.BUFF_SIZE - 1, 0);
	if (SOCKET_ERROR == bytes_recv) {
		std::cout << "Server: Error at recv(): " << WSAGetLastError() << std::endl;
		sock_stat.clear();
		return false;
	}

	if (bytes_recv == 0) {
		std::cout << "Server: Client " << sock_stat.ip_port_string() << " requested to disconnect." << std::endl;
		sock_stat.clear();
		return false;
	}

	recv_buff[bytes_recv] = '\0';
	sock_stat._recv_str += recv_buff;
	sock_stat._last_msg_time = time(NULL);
	if (sock_stat._recv_status == SocketStatus::ReceiveStatus::RECIVE_HEADERS &&
		sock_stat._recv_str.find(HEADER_BREAK_LINE) != std::string::npos) {
		sock_stat._recv_status = SocketStatus::ReceiveStatus::RECIVE_DATA;
	}

	if (sock_stat._recv_status == SocketStatus::ReceiveStatus::RECIVE_DATA) {
		HttpRequest req(sock_stat._recv_str.substr(0, sock_stat._recv_str.find(HEADER_BREAK_LINE) + HEADER_BREAK_LINE.length()));
		if (req.has_header("Content-Length")) {
			int data_length = std::stoi(req.get_header("Content-Length"));
			int curr_data_length = sock_stat._recv_str.substr(
				sock_stat._recv_str.find(HEADER_BREAK_LINE) + HEADER_BREAK_LINE.length(), std::string::npos
				).length();
			if (data_length == curr_data_length) {
				return true;
			}
		}
		else {
			return true;
		}
	}

	return false;
}

void Server::receive_message(SocketStatus& sock_stat) {
	if (recv_wrapper(sock_stat)) {
		HttpRequest req(sock_stat._recv_str);
		HttpResponse res;

		sock_stat._recv_str = "";
		sock_stat._recv_status = SocketStatus::ReceiveStatus::RECIVE_HEADERS;

		std::cout << "Server: got request from " << sock_stat.ip_port_string() << std::endl;

		/*std::cout << "Server: got request from " << sock_stat.ip_port_string() << ", the request is:" << std::endl;
		std::cout << "******************************************************************************" << std::endl;
		std::cout << req << std::endl;
		std::cout << "******************************************************************************" << std::endl;*/

		server_work(req, res);

		sock_stat._res_queue.push(res);
		sock_stat._send_status = SocketStatus::SendStatus::SEND;
	}
}

void Server::send_message(SocketStatus& sock_stat) {
	if (sock_stat._res_queue.empty()) {
		std::cout << "Server: Tried to send with empty queue... aborting function." << std::endl;
		return;
	}
	char send_buff[sock_stat.BUFF_SIZE];

	std::string res_str = sock_stat._res_queue.front().to_string();
	sock_stat._res_queue.pop();
	memcpy(send_buff, res_str.c_str(), res_str.length() * sizeof(char));

	int bytesSent = send(sock_stat._socket, send_buff, res_str.length() * sizeof(char), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		std::cout << "Server: Error at send(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		throw std::exception("Error at send()");
	}

	if (sock_stat._res_queue.empty()) {
		sock_stat._send_status = SocketStatus::SendStatus::IDLE;
	}
}

void Server::server_work(HttpRequest& request, HttpResponse& response) {
	response.set_version(1.1f);

	if (request.is_bad()) {
		response.set_status(HttpResponse::Status::BAD_REQUEST);
	}
	else {
		switch (request.get_method()) {
		case HttpRequest::Method::GET:
			on_get(request, response);
			break;
		case HttpRequest::Method::HEAD:
			on_head(request, response);
			break;
		case HttpRequest::Method::POST:
			on_post(request, response);
			break;
		case HttpRequest::Method::PUT:
			on_put(request, response);
			break;
		case HttpRequest::Method::_DELETE:
			on_delete(request, response);
			break;
		case HttpRequest::Method::TRACE:
			on_trace(request, response);
			break;
		case HttpRequest::Method::OPTIONS:
			on_options(request, response);
			break;
		default:
			response.set_status(HttpResponse::Status::NOT_IMPLEMENTED);
			break;
		}
	}

	if (request.get_method() != HttpRequest::Method::HEAD) {
		response.add_header("Content-Length", std::to_string(response.get_data().length()));
	}
	response.add_header("Server", "Micha's server");
}

void Server::on_get(HttpRequest& request, HttpResponse& response) {
	std::string path = create_path(request);
	std::ifstream file(path);
	if (file.is_open()) {
		response.set_status(HttpResponse::Status::OK);
		std::ostringstream stream;
		stream << file.rdbuf();
		response.set_data(stream.str());
		response.add_header("Content-Type", "text/html; charset=UTF-8");
	}
	else {
		response.set_status(HttpResponse::Status::NOT_FOUND);
	}
}

void Server::on_head(HttpRequest& request, HttpResponse& response) {
	on_get(request, response);
	response.add_header("Content-Length", std::to_string(response.get_data().length()));
	response.set_data("");
}

void Server::on_post(HttpRequest& request, HttpResponse& response) {
	if (!request.has_header("Content-Length")) {
		response.set_status(HttpResponse::Status::BAD_REQUEST);
		response.set_data("Post request must contain a 'Content-Length' header...");
		return;
	}

	response.set_status(HttpResponse::Status::OK);
	std::cout << "POST content: " << request.get_data() << std::endl;
}

void Server::on_put(HttpRequest& request, HttpResponse& response) {
	if (!request.has_header("Content-Length")) {
		response.set_status(HttpResponse::Status::BAD_REQUEST);
		response.set_data("Post request must contain a 'Content-Length' header...");
		return;
	}

	std::string path = create_path(request);
	std::ofstream ofile;
	if (Utils::is_file_exists(path)) {
		response.set_status(HttpResponse::Status::NO_CONTENT);
	}
	else {
		response.set_status(HttpResponse::Status::CREATED);
	}

	ofile.open(path);
	if (!ofile.is_open()) {	//means that the url contains folders or is bad url...
		response.set_status(HttpResponse::Status::BAD_REQUEST);
		response.set_data("The url provided is either a bad url or contains folders...");
		ofile.close();
		return;
	}

	ofile.write(request.get_data().c_str(), request.get_data().length());
	ofile.close();
}

void Server::on_delete(HttpRequest& request, HttpResponse& response) {
	std::string path = create_path(request);
	if (Utils::is_file_exists(path)) {
		if (std::remove(path.c_str()) != 0) {
			response.set_status(HttpResponse::Status::INTERNAL_SERVER_ERROR);
			response.set_data("An error accrued when trying to delete the file.");
		}
		else {
			response.set_status(HttpResponse::Status::OK);
			response.set_data("File deleted");
		}
	}
	else {
		response.set_status(HttpResponse::Status::NOT_FOUND);
	}
}

void Server::on_trace(HttpRequest& request, HttpResponse& response) {
	std::string path = create_path(request);
	if (Utils::is_file_exists(path)) {
		response.set_status(HttpResponse::Status::OK);
		response.add_header("Content-Type", "message/http");
		response.set_data(request.get_data());
	}
	else {
		response.set_status(HttpResponse::Status::NOT_FOUND);
	}
}

void Server::on_options(HttpRequest& request, HttpResponse& response) {
	std::string path = create_path(request);
	std::ifstream file(path);
	if (request.get_url() != "*" && !file.is_open()) {
		response.set_status(HttpResponse::Status::NOT_FOUND);
	}
	else {
		response.set_status(HttpResponse::Status::OK);
		response.add_header("Allow", "GET, HEAD, POST, PUT, DELETE, TRACE, OPTIONS");
	}
	file.close();
}

std::string Server::create_path(const HttpRequest& req) const {
	std::string path = std::string(BASE_DIR);
	if (req.has_query_param("lang")) {
		path += "\\" + req.get_query_param("lang");
	}
	else {
		path += "\\en";
	}

	path += "\\" + req.get_url();
	return path;
}
