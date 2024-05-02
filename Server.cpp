#include "Server.h"

#include <vector>
#include <iostream>
#include <stdexcept>
#include <WS2tcpip.h>

my_chat::Server::Server()
	: Server(std::string(), 0)
{

}

my_chat::Server::Server(std::string ip, const unsigned short port)
	: _serverIp(std::move(ip)), _serverPort(port)
{
    ZeroMemory(&(_wsaData),      sizeof(WSADATA));

    ZeroMemory(&(_serverSocket), sizeof(SOCKET));
    ZeroMemory(&(_serverInfo),   sizeof(sockaddr_in));

    ZeroMemory(&(_clientSocket), sizeof(SOCKET));
    ZeroMemory(&(_clientInfo),   sizeof(sockaddr_in));

    if (WSAStartup(MAKEWORD(2, 2), &_wsaData) != 0)
	{
		throw std::runtime_error("Cannot initialize WinSock: " +
			std::to_string(WSAGetLastError()));
	}
}

my_chat::Server::~Server()
{
	closeConnection();

	WSACleanup();
}

void my_chat::Server::initClient()
{
    const auto clientInfo = reinterpret_cast<sockaddr*>(&_clientInfo);
	int clientSize = sizeof(sockaddr_in);
    _clientSocket = accept(_serverSocket, clientInfo, &clientSize);
    if (_clientSocket == INVALID_SOCKET)
	{
		throw std::runtime_error("Cannot initialize Client Socket: " +
			std::to_string(WSAGetLastError()));
	}
    _isConnectionOpen = true;
}

void my_chat::Server::openConnection()
{
    _serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_serverSocket == INVALID_SOCKET)
	{
		throw std::runtime_error("Cannot initialize Server Socket: " +
			std::to_string(WSAGetLastError()));
	}

	in_addr serverIp;
	if (inet_pton(AF_INET, _serverIp.c_str(), &serverIp) <= 0)
	{
		throw std::runtime_error("Cannot cast ip: " +
			std::to_string(WSAGetLastError()));
	}

    _serverInfo.sin_family = AF_INET;
    _serverInfo.sin_port = htons(_serverPort);
    _serverInfo.sin_addr = serverIp;

    const auto serverInfo = reinterpret_cast<sockaddr*>(&_serverInfo);
    if (bind(_serverSocket, serverInfo, sizeof(sockaddr_in)))
	{
		throw std::runtime_error("Cannot bind socket to server: " +
			std::to_string(WSAGetLastError()));
	}

    if (listen(_serverSocket, SOMAXCONN) != 0)
	{
		throw std::runtime_error("Cannot listen to client: " +
			std::to_string(WSAGetLastError()));
	}

    _isConnectionOpen = true;
}

void my_chat::Server::closeConnection()
{
    _isConnectionOpen = false;

    closesocket(_clientSocket);
    closesocket(_serverSocket);
}

void my_chat::Server::clearClientInfo()
{
    closesocket(_clientSocket);

    ZeroMemory(&(_clientSocket), sizeof(SOCKET));
    ZeroMemory(&(_clientInfo), sizeof(sockaddr_in));
}

std::string my_chat::Server::receiveFromClient()
{
	std::vector<char> message(50);

    const auto err = recv(_clientSocket, message.data(), message.size(), 0);

	if (err == SOCKET_ERROR)
	{
        _isConnectionOpen = false;
		if (WSAGetLastError() == 10054)
		{
			return std::string{ "" };
		}
		throw std::runtime_error("Cannot receive message: " +
			std::to_string(WSAGetLastError()));
	}

	return std::string{ message.begin(), message.end() };
}

void my_chat::Server::sendToClient(const std::string& message)
{
    const auto err = send(_clientSocket, message.data(), message.size(), 0);
	if (err == SOCKET_ERROR)
	{
        _isConnectionOpen = false;
		throw std::runtime_error("Cannot send message: " +
			std::to_string(WSAGetLastError()));
	}
}

void my_chat::Server::setIpAndPort(const std::string& ip, const unsigned short port)
{
    _serverIp = ip;
    _serverPort = port;
}

bool my_chat::Server::isConnectionOpen()
{
    return _isConnectionOpen;
}
