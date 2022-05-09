#pragma once
#include "httpRequest.h"
#include "httpResponse.h"
#include "utils.h"
#include <iostream>
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <array>
#include <exception>
#include <map>
#include <queue>
#include <fstream>
#include <cstdio>


class Server {
public:
	Server();
	void listen_forever(int port);		//TODO

private:
	struct SocketStatus {
		enum class ReceiveStatus {
			EMPTY, IDLE, RECIVE
		};

		enum class SendStatus {
			EMPTY, IDLE, SEND
		};


		static constexpr int BUFF_SIZE = 2048;

		SOCKET _socket;
		struct sockaddr_in _socket_info;
		ReceiveStatus _recv_status;
		SendStatus _send_status;
		time_t _last_msg_time;
		std::queue<HttpResponse> _res_queue;

		SocketStatus();
		void init(const SOCKET& socket, const sockaddr_in& socket_info);
		bool try_init(const SOCKET& socket, const sockaddr_in& socket_info);
		bool is_empty() const { return _recv_status == ReceiveStatus::EMPTY && _send_status == SendStatus::EMPTY; }
		void clear();
		std::string ip_port_string() const;
	};

	SOCKET m_listen_socket;
	std::array<SocketStatus, 20> m_sockets;

	void listen_setup(int port);
	void handle_timeouts(double timeout_seconds = 120.0);
	int select_wrapper(fd_set* recv_set, fd_set* send_set);
	int create_sets(fd_set* recv_set, fd_set* send_set);
	void accept_connections();
	bool add_socket(const SOCKET& socket, const sockaddr_in& socket_info);
	void receive_message(SocketStatus& sock_stat);
	void send_message(SocketStatus& sock_stat);

	void server_work(HttpRequest& request, HttpResponse& response);
	void on_get(HttpRequest& request, HttpResponse& response);
	void on_head(HttpRequest& request, HttpResponse& response);
	void on_post(HttpRequest& request, HttpResponse& response);
	void on_put(HttpRequest& request, HttpResponse& response);
	void on_delete(HttpRequest& request, HttpResponse& response);
	void on_trace(HttpRequest& request, HttpResponse& response);
	void on_options(HttpRequest& request, HttpResponse& response);

	std::string create_path(const HttpRequest& req) const;

	static constexpr const char* BASE_DIR = "C:\\temp";
};