#pragma once

#include <iostream>
#include <WinSock2.h>

#include "HTTPRequest.h"
#include "HTTPResponse.h"

#pragma comment(lib, "ws2_32.lib")

typedef int socklen_t;

inline bool InitializeSockets() { WSADATA WsaData; return WSAStartup(MAKEWORD(2, 2), &WsaData) == NO_ERROR; }
inline void ShutdownSockets() { WSACleanup(); }

struct ClientData
{
	sockaddr_in connection;
	socklen_t connection_length	{ sizeof(sockaddr) };
	int handle					{ 0 };
	unsigned int port			{ 0 };
	std::string buffer			{ "" };
	std::string address			{ "" };
	HTTPRequest request;

	bool sendData(std::string data)
	{
		int sent = send(handle, data.c_str(), data.size(), 0);
		return (sent == data.size());
	}

	bool sendData(HTTPResponse& response)
	{
		int sent = send(handle, response.get().c_str(), response.get().size(), 0);
		return (sent == response.get().size());
	}
};

inline std::string convertAddress(unsigned int address)
{
	return std::to_string((address >> 24) & 0xFF) + "." + std::to_string((address >> 16) & 0xFF) + "." + std::to_string((address >> 8) & 0xFF) + "." + std::to_string(address & 0xFF);
}

class Socket
{
public:

	bool open(unsigned short port)
	{
		if ((handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0)
		{
			printf("Failed to create socket.\n");
			return false;
		}

		sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(port);

		// Bind the port to the address
		if (bind(handle, (struct sockaddr*) & address, sizeof(sockaddr_in)) == SOCKET_ERROR)
		{
			printf("Failed to bind socket.\n");
			return false;
		}
		return true;
	}

	void beginListening(int count)
	{
		if (listen(handle, count) < 0)
		{
			printf("Failed to listen for incoming connections.\n");
		}
	}

	bool accepting(ClientData& clientData, const unsigned int buffer_size)
	{
		clientData.handle = accept(handle, (sockaddr*)&clientData.connection, &clientData.connection_length);
		if (clientData.handle < 0) {
			int err = WSAGetLastError();
			if (err != WSAEINTR) {
				std::cout << "Failed to accept new connection with error code: " << err << "\n";
			}
			return false;
		} else {
			clientData.address = convertAddress(ntohl(clientData.connection.sin_addr.s_addr));
			clientData.port = ntohs(clientData.connection.sin_port);

			char* buffer = new char[buffer_size];
			int bytes_recv = recv(clientData.handle, buffer, buffer_size, 0);
			if (bytes_recv > 0) {
				if (bytes_recv < buffer_size-1) {
					buffer[bytes_recv] = '\0';
				}
				std::stringstream ss;
				ss << buffer;
				clientData.buffer = ss.str();
				clientData.request = HTTPRequest(clientData.buffer);
				delete[] buffer;
				return true;
			}
			delete[] buffer;
		}
		return false;
	}

	void close()
	{
		int err = closesocket(handle);
		if (err == SOCKET_ERROR) {
			std::cout << "Failed to close socket with error code: " << WSAGetLastError() << "\n";
		}
	}

	int getHandle()
	{
		return handle;
	}
private:
	int handle	{ 0 };
};
