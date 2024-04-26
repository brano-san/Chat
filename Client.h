#pragma once

#include <string>
#include <WinSock2.h>

namespace my_chat
{
	class Client final
	{
	public:
		Client();
		Client(std::string ip, unsigned short port);

		~Client();

	public:
		void openConnection();

		void closeConnection();

		bool isConnectionOpen();

		std::string receiveFromServer();

		void sendToServer(const std::string& message);

		void setIpAndPort(const std::string& ip, unsigned short port);

	private:
		WSADATA _wsaData;
		SOCKET _serverSocket;
		sockaddr_in _serverInfo;

		std::string _serverIp;
		unsigned short _serverPort;

        bool _isConnectionOpen = false;
    };
}
