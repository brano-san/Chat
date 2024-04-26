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
	ZeroMemory(&(this->_wsaData),      sizeof(WSADATA));

	ZeroMemory(&(this->_serverSocket), sizeof(SOCKET));
	ZeroMemory(&(this->_serverInfo),   sizeof(sockaddr_in));

	ZeroMemory(&(this->_clientSocket), sizeof(SOCKET));
	ZeroMemory(&(this->_clientInfo),   sizeof(sockaddr_in));

	if (WSAStartup(MAKEWORD(2, 2), &this->_wsaData) != 0)
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
	const auto clientInfo = reinterpret_cast<sockaddr*>(&this->_clientInfo);
	int clientSize = sizeof(sockaddr_in);
	this->_clientSocket = accept(this->_serverSocket, clientInfo, &clientSize);
	if (this->_clientSocket == INVALID_SOCKET)
	{
		throw std::runtime_error("Cannot initialize Client Socket: " +
			std::to_string(WSAGetLastError()));
	}
    this->_isConnectionOpen = true;
}

void my_chat::Server::openConnection()
{
    this->_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (this->_serverSocket == INVALID_SOCKET)
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

	this->_serverInfo.sin_family = AF_INET;
	this->_serverInfo.sin_port = htons(this->_serverPort);
	this->_serverInfo.sin_addr = serverIp;

	const auto serverInfo = reinterpret_cast<sockaddr*>(&this->_serverInfo);
	if (bind(this->_serverSocket, serverInfo, sizeof(sockaddr_in)))
	{
		throw std::runtime_error("Cannot bind socket to server: " +
			std::to_string(WSAGetLastError()));
	}

	if (listen(this->_serverSocket, SOMAXCONN) != 0)
	{
		throw std::runtime_error("Cannot listen to client: " +
			std::to_string(WSAGetLastError()));
	}

	this->_isConnectionOpen = true;
}

void my_chat::Server::closeConnection()
{
	this->_isConnectionOpen = false;

	closesocket(this->_clientSocket);
	closesocket(this->_serverSocket);
}

void my_chat::Server::clearClientInfo()
{
	closesocket(this->_clientSocket);

	ZeroMemory(&(this->_clientSocket), sizeof(SOCKET));
	ZeroMemory(&(this->_clientInfo), sizeof(sockaddr_in));
}

std::string my_chat::Server::receiveFromClient()
{
	std::vector<char> message(50);

	const auto err = recv(this->_clientSocket, message.data(), message.size(), 0);

	if (err == SOCKET_ERROR)
	{
		this->_isConnectionOpen = false;
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
	const auto err = send(this->_clientSocket, message.data(), message.size(), 0);
	if (err == SOCKET_ERROR)
	{
		this->_isConnectionOpen = false;
		throw std::runtime_error("Cannot send message: " +
			std::to_string(WSAGetLastError()));
	}
}

void my_chat::Server::setIpAndPort(const std::string& ip, const unsigned short port)
{
	this->_serverIp = ip;
	this->_serverPort = port;
}

bool my_chat::Server::isConnectionOpen()
{
	return this->_isConnectionOpen;
}
