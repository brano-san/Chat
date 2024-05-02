#include "Client.h"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <WS2tcpip.h>

my_chat::Client::Client()
	: Client(std::string(), 0)
{
	
}

my_chat::Client::Client(std::string ip, const unsigned short port)
	: _serverIp(std::move(ip)) ,_serverPort(port)
{
    ZeroMemory(&(_wsaData),      sizeof(WSADATA));
    ZeroMemory(&(_serverSocket), sizeof(SOCKET));
    ZeroMemory(&(_serverInfo),   sizeof(sockaddr_in));

    if (WSAStartup(MAKEWORD(2, 2), &_wsaData) != 0)
	{
		throw std::runtime_error("Cannot initialize WinSock: " +
			std::to_string(WSAGetLastError()));
	}
}

my_chat::Client::~Client()
{
	WSACleanup();
}

void my_chat::Client::openConnection()
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
    if (connect(_serverSocket, serverInfo, sizeof(sockaddr_in)) != 0)
	{
		throw std::runtime_error("Cannot connect to server: " +
			std::to_string(WSAGetLastError()));
	}

    _isConnectionOpen = true;
}

void my_chat::Client::closeConnection()
{
    _isConnectionOpen = false;

    closesocket(_serverSocket);
}

std::string my_chat::Client::receiveFromServer()
{
    std::vector<char> message(512);

    const auto err = recv(_serverSocket, message.data(), message.size(), 0);

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

void my_chat::Client::sendToServer(const std::string& message)
{
    const auto err = send(_serverSocket, message.data(), message.size(), 0);
	if (err == SOCKET_ERROR)
	{
        _isConnectionOpen = false;
		throw std::runtime_error("Cannot send message: " +
			std::to_string(WSAGetLastError()));
	}
}

void my_chat::Client::setIpAndPort(const std::string& ip, const unsigned short port)
{
    _serverIp = ip;
    _serverPort = port;
}

bool my_chat::Client::isConnectionOpen()
{
    return _isConnectionOpen;
}
