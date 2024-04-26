#pragma once

#include <string>
#include <WinSock2.h>

namespace my_chat
{
	class Server final
	{
	public:
		Server();
		Server(std::string ip, unsigned short port);
		~Server();

	public:
		void initClient();

		void openConnection();

		void closeConnection();

		void clearClientInfo();

		bool isConnectionOpen();

		std::string receiveFromClient();

		void sendToClient(const std::string& message);

		void setIpAndPort(const std::string& ip, unsigned short port);

	private:
		WSADATA _wsaData;
		SOCKET _serverSocket;
		sockaddr_in _serverInfo;

		std::string _serverIp;
		unsigned short _serverPort;

		SOCKET _clientSocket;
		sockaddr_in _clientInfo;

		bool _isConnectionOpen = false;
	};
}

