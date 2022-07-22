#pragma comment (lib, "ws2_32.lib")

#include "WSAInitializer.h"
#include "Server.h"
#include <iostream>
#include <exception>

int main()
{
	try
	{
		WSAInitializer wsaInit;
		Server myServer;

		//creating server
		myServer.buildServer();

		// main thread- handles the messages
		thread mainThread(&Server::messagesHandler, ref(myServer));
		mainThread.detach();

		//connector thread - keeps accepting clients
		thread connectorThread(&Server::acceptClients, ref(myServer));
		connectorThread.join();

	}
	catch (std::exception& e)
	{
		std::cout << "Error occured: " << e.what() << std::endl;
	}
	system("PAUSE");
	return 0;
}