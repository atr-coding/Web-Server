#pragma comment(linker, "/SUBSYSTEM:WINDOWS");
#pragma comment(linker, "/entry:mainCRTStartup");
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>

#include "Misc.h"
#include "Socket.h"
#include "HTTPResponse.h"
#include "HTTPRequest.h"

#include "Tray.h"

#define PORT 8000

const std::string server_path = "www";

std::atomic<bool> run;
std::mutex m;
Socket server;

void tray_thread()
{
	tray t("icon.ico");
	t.addItem("Restart", 0, 0, nullptr, nullptr);
	t.addItem("-", 0, 0, nullptr, nullptr);
	t.addItem("Quit", 0, 0, [&] {
		run = false;
		std::lock_guard<std::mutex> g(m);
		shutdown(server.getHandle(), SD_BOTH);
		server.close();
		tray_exit();
	}, nullptr);

	tray_init(&t);
	while (tray_loop(1) == 0 && run){
		tray_update(&t);
	}
	run = false;
}

int main()
{
	if (!InitializeSockets()) {
		printf("Failed to initialize servers.\n");
	}

	run = true;
	std::thread trayThread(tray_thread);

	if (server.open(PORT) == false) {
		printf("Failed to open server.\n");
		run = false;
	}

	std::string index = loadFile(server_path + "/index.html");

	HTTPResponse indexResponse("HTTP/1.1 200 OK");
	indexResponse.setContent(index, "text/html");
	indexResponse.compile();

	HTTPResponse notFound("HTTP/1.1 404 Not Found");
	notFound.setContent("<html><head></head><body><div style='padding:0px; margin:100px auto; width:600px; font-size:72px; color:#333; text-align:center;'>404 Not Found</div></body></html>", "text/html");
	notFound.compile();

	printf("Listening...\n");
	server.beginListening(10);

	while (run)
	{
		ClientData client;
		if (server.accepting(client, 2048))
		{
			std::cout << "Connection from: " << client.address << ":" << client.port << "\t[" << (client.request.getType() == GET ? "GET" : "POST") << " " << client.request.getPath() << "]" << "\n";
			
			if (client.request.getPath() == "/") {
				client.sendData(indexResponse);
			} else {
				auto dots = explode(client.request.getPath(), '.');
			
				std::string file_extension = "txt";
				if (dots.size() > 0) { file_extension = dots.back(); }
				std::string file_type = "text/plain";
			
				if (file_extension == "html") { file_type = "text/html"; }
				else if (file_extension == "png") { file_type = "image/png"; }
			
				std::string response = notFound.get();
			
				HTTPResponse fileResponse("HTTP/1.1 200 OK");
				std::string file = loadFile(server_path + client.request.getPath(), true);
				if (file != "") {
					fileResponse.setContent(file, file_type);
					fileResponse.compile();
					response = fileResponse.get();
				}
			
				client.sendData(response);
			}
			closesocket(client.handle);
		}

	}
	server.close();
	ShutdownSockets();
	trayThread.join();
	return 0;
}
/*
if (lines.size() > 0)
{
	auto line = explode(lines.at(0), ' ');
	if (line.size() > 2)
	{
		if (line.at(0) == "GET" || line.at(0) == "POST")
		{
			std::string response;
			if (line.at(1).size() > 1)
			{
				std::cout << line.at(1) << "\n";
				auto dots = explode(line.at(1), '.');
				std::string file_extension = "txt";
				if (dots.size() > 0)
				{
					file_extension = dots.back();
				}
				std::string file_type = "text/plain";
				if (file_extension == "html")
				{
					file_type = "text/html";
				}
				else if (file_extension == "png")
				{
					file_type = "image/png";
				}

				HTTPResponse fileResponse("HTTP/1.1 200 OK");
				std::string file = loadFile(server_path + line.at(1), true);
				std::cout << "File: " << line.at(1) << "\n";
				if (file == "")
				{
					response = notFound.get();
				}
				else
				{
					fileResponse.setContent(file, file_type);
					response = fileResponse.get();
				}
			}
			else
			{
				// Initial request, send main page
				response = indexResponse.compile();
			}
			int sent = send(client.handle, response.c_str(), response.size(), 0);
			if (sent == response.size())
			{
				printf("Response sent successfully.\n");
			}
		}
	}
}
*/