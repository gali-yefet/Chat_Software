#pragma once
#include "Helper.h"

#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <queue>
#include <windows.h>
#include <map>
#include <mutex>
#include <condition_variable>
#include <string>
#include <fstream>
#include <exception>
#include <thread>
#include <sstream>

using namespace std; 

struct messageD
{
	string senderUsername;
	string recvUsername;
	string message;
};

class Server
{
public:
	Server();
	~Server();
	void buildServer();
	void acceptClients();
	void messagesHandler();

private:

	void clientHandler(SOCKET clientSocket);
	string usersString();

	SOCKET _serverSocket;
	queue<messageD> _messages;
	map<string, SOCKET> _connectedUsers;
	mutex _messagesMtx;
	mutex _usersMtx;
	condition_variable _cvMessages;
};

