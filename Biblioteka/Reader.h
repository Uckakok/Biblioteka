#pragma once
#include "iostream"
#include "Book.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include "Structs.h"
#include "vector"

using namespace std; // Forward declaration of LibrarySystem class

class Reader {
private:
	SOCKET ServerSocket;
	void SendPacket(const Packet& packet);
	void ListenToMessages();
	void ProcessPacket(Packet packet);
public:
	string Id;
	vector<string> Notifications;
	Reader(string NewSessionId, SOCKET NewServerSocket) : ServerSocket(NewServerSocket), SessionId(NewSessionId) { ; };
	void ClientMenu();
	void AddBorrowedBook(Book* NewBook);
	void PostUserNotification(string Message);
	string Username;
	string HashedPassword;
	string SessionId;
	vector<Book*> Books;
};