#include "Reader.h"
#include "string"
#include <chrono>
#include <thread>
#include "iostream"

using namespace std;

void Reader::SendPacket(const Packet& packet)
{
	// Send packet
	if (send(ServerSocket, reinterpret_cast<const char*>(&packet), sizeof(packet), 0) == SOCKET_ERROR) {
		closesocket(ServerSocket);
		throw LibraryException("Error sending packet to server.");
	}
}

void Reader::ListenToMessages()
{
	while (true) {
		// Read from the socket and process messages
		Packet packet;
		int bytesReceived = recv(ServerSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
		if (bytesReceived == SOCKET_ERROR) {
			int error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK) {
				//std::cerr << "Error receiving packet from server." << std::endl;
				break;
			}
		}
		else if (bytesReceived == 0) {
			int error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK) {
				//Server has closed connection
				cout << "Server disconnected." << endl;
				break;
			}
			break;
		}
		else {
			// Process the received packet
			ProcessPacket(packet);
		}

		// Sleep for 100 milliseconds
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void Reader::ProcessPacket(Packet packet)
{
	//Simply print what happened
	switch (packet.action) {
	case BORROW_BOOK:
		cout << "Book " << packet.Title << " borrowed successfully." << endl;
		break;
	case BORROW_FAIL:
		cout << "Failed to borrow book " << packet.Title << "." << endl;
		break;
	case RETURN_BOOK:
		cout << "Book " << packet.Title << " returned successfully." << endl;
		break;
	case RETURN_FAIL:
		cout << "Failed to return book" << packet.Title << "." << endl;
		break;
	case SUBSCRIBE_BOOK:
		cout << "Subscribed to book " << packet.Title << " successfully." << endl;
		break;
	case SUBSCRIBE_FAIL:
		cout << "Failed to subscribe to book " << packet.Title << "." << endl;
		break;
	case BOOK_AVAILABLE:
		cout << "Book " << packet.Title << " is available." << endl;
		break;
	case BOOK_UNAVAILABLE:
		cout << "Book " << packet.Title << " is unavailable." << endl;
		break;
	case NOTIFICATION:
		cout << "Notification: " << packet.Username << endl;
		break;
	case BOOK_INFO:
		cout << "Title: " << packet.Title << "\tAuthor: " << packet.Author << endl;
	default:
		//cout << "Unexpected packet action encountered." << endl;
		;
	}
}

void Reader::ClientMenu()
{
	std::thread readerThread(&Reader::ListenToMessages, this);
	readerThread.detach();

	do {
		cout << "1 - borrow a book" << endl;
		cout << "2 - display borrowed books" << endl;
		cout << "3 - return a book" << endl;
		cout << "4 - subscribe to a book" << endl;
		cout << "5 - check book availability" << endl;
		cout << "6 - display catalog" << endl;
		cout << "0 - logout" << endl;
		int Choice;
		while (!(cin >> Choice)) {
			cout << "Invalid input. Please enter a valid option." << endl;
			cin.clear(); // Clear fail state
			cin.ignore(1000, '\n');
		}
		switch (Choice) {
		case 0:
			//send close session packet
			Packet packet;
			packet.action = CLOSE_SESSION;
			strncpy_s(packet.SessionId, SessionId.c_str(), _TRUNCATE);
			SendPacket(packet);
			return;
		case 1: {
			char Title[MAXPACKETTEXT];
			cout << "Enter title of the book you want to borrow: ";
			while (getchar() != '\n');
			if (scanf_s("%[^\n]", Title, static_cast<unsigned int>(sizeof(Title))) != 1) {
				cout << "Invalid input. Please enter a valid title." << endl;
				while (getchar() != '\n');
			}
			while (getchar() != '\n');
			Title[sizeof(Title) - 1] = '\0';
			Packet packet;
			strncpy_s(packet.Title, Title, _TRUNCATE);
			packet.action = BORROW_BOOK;
			strncpy_s(packet.SessionId, SessionId.c_str(), _TRUNCATE);
			SendPacket(packet);
			break; }
		case 2:{
			Packet packet;
			packet.action = REQUEST_OWNED;
			strncpy_s(packet.SessionId, SessionId.c_str(), _TRUNCATE);
			SendPacket(packet);
			break; }
		case 3: {
			char Title[MAXPACKETTEXT];
			cout << "Enter title of the book you want to return: ";
			while (getchar() != '\n');
			if (scanf_s("%[^\n]", Title, static_cast<unsigned int>(sizeof(Title))) != 1) {
				cout << "Invalid input. Please enter a valid title." << endl;
				while (getchar() != '\n');
			}
			while (getchar() != '\n');
			Title[sizeof(Title) - 1] = '\0';
			Packet packet;
			strncpy_s(packet.Title, Title, _TRUNCATE);
			packet.action = RETURN_BOOK;
			strncpy_s(packet.SessionId, SessionId.c_str(), _TRUNCATE);
			SendPacket(packet);
			break;
		}
		case 4: {
			char Title[MAXPACKETTEXT];
			cout << "Enter title of the book you want to subscribe to: ";
			while (getchar() != '\n');
			if (scanf_s("%[^\n]", Title, static_cast<unsigned int>(sizeof(Title))) != 1) {
				cout << "Invalid input. Please enter a valid title." << endl;
				while (getchar() != '\n');
			}
			while (getchar() != '\n');
			Title[sizeof(Title) - 1] = '\0';
			Packet packet;
			strncpy_s(packet.Title, Title, _TRUNCATE);
			packet.action = SUBSCRIBE_BOOK;
			strncpy_s(packet.SessionId, SessionId.c_str(), _TRUNCATE);
			SendPacket(packet);
			break; }
		case 5: {
			char Title[MAXPACKETTEXT];
			cout << "Enter title of the book you want to check availability for: ";
			while (getchar() != '\n');
			if (scanf_s("%[^\n]", Title, static_cast<unsigned int>(sizeof(Title))) != 1) {
				cout << "Invalid input. Please enter a valid title." << endl;
				while (getchar() != '\n');
			}
			while (getchar() != '\n');
			Title[sizeof(Title) - 1] = '\0';
			Packet packet;
			strncpy_s(packet.Title, Title, _TRUNCATE);
			packet.action = AVAILABILITY_BOOK;
			strncpy_s(packet.SessionId, SessionId.c_str(), _TRUNCATE);
			SendPacket(packet);
			break;
		}
		case 6: {
			Packet packet;
			packet.action = REQUEST_CATALOGUE;
			strncpy_s(packet.SessionId, SessionId.c_str(), _TRUNCATE);
			SendPacket(packet);
			break; }
		default:
			break;
		}

	} while (true);
}

void Reader::AddBorrowedBook(Book * NewBook)
{
	Books.push_back(NewBook);
}

void Reader::PostUserNotification(string Message)
{
	Notifications.push_back(Message);
}
