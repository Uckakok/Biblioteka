#include "Interface.h"

#include <iostream>
#include <string>
#include "Reader.h"

using namespace std;

UserInterface::~UserInterface()
{
    WSACleanup();
}

UserInterface::UserInterface()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed. Critical error" << endl;
        exit(1);
    }
}

void UserInterface::MainMenu()
{
    do {
        try {
            int ChoiceCode;
            cout << "1 - add a new account" << endl;
            cout << "2 - log in" << endl;
            cout << "0 - exit" << endl;

           while (!(cin >> ChoiceCode)) {
               cout << "Invalid input. Please enter a valid option." << endl;
               cin.clear(); // Clear fail state
               cin.ignore(1000, '\n');
           }
           cin.clear(); // Clear fail state
           cin.ignore(1000, '\n');

            switch (ChoiceCode) {
            case 0:
                return;
            case 1:
                CreateAccount();
                break;
            case 2:
                Login();
                break;
            default:
                cout << "unknown option" << endl;
            }
        }
        catch (const LibraryException& e) {
            std::cerr << "Error encountered: " << e.what() << std::endl;
        }

    } while (true);
}

void UserInterface::CreateAccount() {
    Packet packet;
    packet.action = CREATE_ACCOUNT;

    // Request username
    cout << "Enter the username: ";
    string username;
    getline(cin, username);
    if (username.length() > MAXPACKETTEXT) {
        throw LibraryException("Password is too long.");
    }
    strncpy_s(packet.Username, username.c_str(), _TRUNCATE);

    // Request password
    cout << "Enter the password: ";
    string password;
    getline(cin, password);
    if (password.length() > MAXPACKETTEXT) {
        throw LibraryException("Password is too long.");
    }
    strncpy_s(packet.Password, password.c_str(), _TRUNCATE);

    // Send packet to server
    SendPacket(packet);
}

void UserInterface::Login() {
    Packet packet;
    packet.action = LOGIN;

    // Request username
    cout << "Enter the username: ";
    string username;
    getline(cin, username);
    if (username.length() > MAXPACKETTEXT) {
        throw LibraryException("Username is too long.");
    }
    strncpy_s(packet.Username, username.c_str(), _TRUNCATE);

    // Request password
    cout << "Enter the password: ";
    string password;
    getline(cin, password);
    if (password.length() > MAXPACKETTEXT) {
        throw LibraryException("Password is too long.");
    }
    strncpy_s(packet.Password, password.c_str(), _TRUNCATE);

    // Send packet to server
    SendPacket(packet);
}

void UserInterface::SendPacket(const Packet& packet)
{
    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        throw LibraryException("Error creating socket.");
    }

    // Server address and port
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(IPADDRESS);

    // Connect to server
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        throw LibraryException("Error connecting to server.");
    }

    // Send packet
    if (send(sock, reinterpret_cast<const char*>(&packet), sizeof(packet), 0) == SOCKET_ERROR) {
        closesocket(sock);
        throw LibraryException("Error sending packet to server.");
    }

    // Set timeout for receiving reply
    timeval timeout;
    timeout.tv_sec = 10; // 10 seconds timeout
    timeout.tv_usec = 0;

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);

    // Wait for socket to become readable with timeout
    int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);
    if (selectResult == SOCKET_ERROR) {
        closesocket(sock);
        throw LibraryException("Error in select.");
    }
    else if (selectResult == 0) {
        closesocket(sock);
        throw LibraryException("Timeout occurred. No reply from server.");
    }

    // Receive reply from server
    Packet replyPacket;
    int bytesReceived = recv(sock, reinterpret_cast<char*>(&replyPacket), sizeof(replyPacket), 0);
    if (bytesReceived == SOCKET_ERROR) {
        throw LibraryException("Error receiving reply from server.");
    }
    else if (bytesReceived == 0) {
        throw LibraryException("server disconnected");
    }
    else {
        // Process reply packet here
        if (replyPacket.action == LOGIN_SUCCESS && replyPacket.SessionId != "") {
            try {
                u_long mode = 1; // 1 to enable non-blocking mode
                if (ioctlsocket(sock, FIONBIO, &mode) != NO_ERROR) {
                    closesocket(sock);
                    throw LibraryException("Error setting socket to non-blocking mode.");
                }

                StartUserSession(replyPacket.SessionId, sock);
            }
            catch (const LibraryException& e) {
                std::cerr << "Error encountered: " << e.what() << std::endl;
                //cleanup is done either way a few lines down
            }
        }
        else {
            throw LibraryException("Unsuccessful login atempt");
        }
    }

    // Close socket
    closesocket(sock);
}

void UserInterface::StartUserSession(string SessionId, SOCKET ServerSocket)
{
    Reader User(SessionId, ServerSocket);
    User.ClientMenu();
}
