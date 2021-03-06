#include "server.h"


int main() {
	constexpr int PORT = 3000;
	constexpr bool LOG_FLAG = false;
	WSAData wsaData;
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		std::cout << "Time Server: Error at WSAStartup()" << std::endl;
		return 1;
	}

	try {
		Server server(LOG_FLAG);
		server.listen_forever(PORT);
	}
	catch (std::exception& ex) {
		std::cout << ex.what() << std::endl;
		return 1;
	}

	return 0;
}