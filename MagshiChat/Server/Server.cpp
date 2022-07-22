#include "Server.h"

#define PORT 8876

Server::Server()
{

	// this server use TCP. that why SOCK_STREAM & IPPROTO_TCP
	// if the server use UDP we will use: SOCK_DGRAM & IPPROTO_UDP
	_serverSocket = socket(AF_INET,  SOCK_STREAM,  IPPROTO_TCP); 

	if (_serverSocket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__ " - socket");
}

Server::~Server()
{
	try
	{
		// the only use of the destructor should be for freeing 
		// resources that was allocated in the constructor
		closesocket(_serverSocket);
	}
	catch (...) {}
}

void Server::buildServer()
{
	
	struct sockaddr_in sa = { 0 };
	
	sa.sin_port = htons(PORT); // port that server will listen for
	sa.sin_family = AF_INET;   // must be AF_INET
	sa.sin_addr.s_addr = INADDR_ANY;    // when there are few ip's for the machine. We will use always "INADDR_ANY"

	// Connects between the socket and the configuration (port and etc..)
	if (bind(this->_serverSocket, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - bind");
	
	// Start listening for incoming requests of clients
	if (listen(this->_serverSocket, SOMAXCONN) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - listen");
	std::cout << "Listening on port " << PORT << std::endl;
}


void Server::acceptClients()
{
	while (true)
	{
		// this accepts the client and create a specific socket from server to this client
		// the process will not continue until a client connects to the server
		SOCKET client_socket = accept(this->_serverSocket, NULL, NULL);
		if (client_socket == INVALID_SOCKET)
			throw std::exception(__FUNCTION__);

		std::cout << "Client accepted!" << std::endl;
		//creating a thread for the user
		thread clientThread(&Server::clientHandler, this, client_socket);
		clientThread.detach();
	}
}


void Server::clientHandler(SOCKET clientSocket)
{
	string username = "";
	string secUsername = "";
	string message = "";
	int lenUsername = 0;
	int lenMessage = 0;

	try
	{
		if (Helper::getMessageTypeCode(clientSocket) == MT_CLIENT_LOG_IN)// user login message
		{
			lenUsername = Helper::getIntPartFromSocket(clientSocket, 2);// get the length of the username
			username = Helper::getStringPartFromSocket(clientSocket, lenUsername);// get username of the connected user
			unique_lock<mutex> usersLock(this->_usersMtx);// lock the users map 
			this->_connectedUsers.insert(pair<string, SOCKET>(username, clientSocket));
			usersLock.unlock();// unlock the users map
			Helper::send_update_message_to_client(clientSocket, "", "", usersString());
		}
		else
		{
			throw(exception("INVALID LOGIN!\n"));
		}

		while (true)
		{
			if (Helper::getMessageTypeCode(clientSocket) == MT_CLIENT_UPDATE)
			{
				lenUsername = Helper::getIntPartFromSocket(clientSocket, 2);// get the length of the second username
				if (lenUsername != 0)// the client wants to send a message to another client or get update about the chat with him
				{
					secUsername = Helper::getStringPartFromSocket(clientSocket, lenUsername);// get second username
					lenMessage = Helper::getIntPartFromSocket(clientSocket, 5);// get the length of the new message
					if (lenMessage != 0)// the client wants to send a message
					{
						message = Helper::getStringPartFromSocket(clientSocket, lenMessage);// get new message
						messageD m;
						m.senderUsername = username;
						m.recvUsername = secUsername;
						m.message = message;

						unique_lock<mutex> mLock(this->_messagesMtx);// lock the messages queue 
						this->_messages.push(m);// adding message details struct to messages queue
						mLock.unlock();// unlock the messages queue
						this->_cvMessages.notify_one();// notifing the thread which handle the messages
					}
					else // the client wants an update of the chat without sending a message
					{
						vector<string> arr = { username, secUsername };
						sort(arr.begin(), arr.end());
						string path = "chats\\" + arr[0] + "&" + arr[1] + ".txt";
						
						ifstream file;
						file.open(path);
						stringstream chatContent;
						chatContent << file.rdbuf();
						Helper::send_update_message_to_client(clientSocket, chatContent.str(), secUsername, usersString());
					}				
				}
				else// the client wants an update message
				{
					Helper::send_update_message_to_client(clientSocket, "", "", usersString());
				}				
			}
		}
	}
	catch (const std::exception& e)
	{
		unique_lock<mutex> usersLock(this->_usersMtx);// lock the users map 
		this->_connectedUsers.erase(username);
		usersLock.unlock();// unlock the users map

		// Closing the socket (in the level of the TCP protocol)
		closesocket(clientSocket);
	}
}

//function handles the messages
//input: none
//output: none
void Server::messagesHandler()
{
	string fileContent = "";

	while (true)
	{
		unique_lock<mutex> lock(this->_messagesMtx);
		this->_cvMessages.wait(lock, [&] { return (!this->_messages.empty()) ? true : false; });
		messageD m = this->_messages.front();
		this->_messages.pop();
		lock.unlock();

		vector<string> arr = { m.senderUsername, m.recvUsername };
		sort(arr.begin(), arr.end());
		string path = "chats\\" + arr[0] + "&" + arr[1] + ".txt";
		ofstream chatContent(path, ios_base::app);
		if (!chatContent)
		{
			cout << "Error with creating chat content file." + path << endl;
			exit(1);
		}
		chatContent << "&MAGSH_MESSAGE&&Author&" + m.senderUsername + "&DATA&" + m.message;
		chatContent.close();

		ifstream file;
		file.open(path);
		stringstream content;
		content << file.rdbuf();

		Helper::send_update_message_to_client(this->_connectedUsers.find(m.senderUsername)->second, content.str(), m.recvUsername, usersString());
	}
}

//function makes the connected users names into string by the protocol
//input: none
//output: the string
string Server::usersString()
{
	string users = "";

	unique_lock<mutex> usersLock(this->_usersMtx);// lock the users map 
	for (auto user : this->_connectedUsers)
		users += (user.first + "&");
	usersLock.unlock();// unlock the users map

	users = users.substr(0, users.length() - 1);// cutting the last '&'

	return users;
}