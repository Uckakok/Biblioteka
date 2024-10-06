#include "Server.h"
#include <string>
#include <chrono>
#include <algorithm>

Server* Server::Instance{ nullptr };

void Server::Start() {
    // Initialize library system
    System = LibrarySystem::GetInstance();

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;
        return;
    }

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Error creating socket." << endl;
        WSACleanup();
        return;
    }

    // Bind socket
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Error binding socket." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    // Listen for connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Error listening on socket." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    cout << "Server started. Listening on port " << PORT << "..." << endl;

    // Accept incoming connections and handle packets
    AcceptConnections();
}

void Server::AcceptConnections() {
    while (true) {
        // Accept incoming connection
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Error accepting connection." << endl;
            continue;
        }

        u_long mode = 1; // 1 to enable non-blocking mode
        if (ioctlsocket(clientSocket, FIONBIO, &mode) != NO_ERROR) {
            cerr << "Error setting socket to non-blocking mode." << endl;
            closesocket(clientSocket);
            continue;
        }

        cout << "Client connected." << endl;

        // Create a new thread to handle the client connection
        std::thread ClientThread(&Server::HandleClient, this, clientSocket);
        ClientThread.detach();
    }
}

void Server::HandleClient(SOCKET clientSocket) {
    // Receive and process messages from the client

    cout << "Started handling new client" << endl;

    Reader* HandledUser = nullptr;

    timeval timeout;
    timeout.tv_sec = 1; // 1 second timeout
    timeout.tv_usec = 0;

    while (true) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(clientSocket, &readSet);

        int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        if (selectResult == SOCKET_ERROR) {
            cerr << "Error in select." << endl;
            break;
        }
        else if (selectResult != 0) {
            Packet packet;
            int bytesReceived = recv(clientSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
            if (bytesReceived == SOCKET_ERROR) {
                int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) {
                    // No data available to read without blocking, continue waiting
                    continue;
                }
                cerr << "Error receiving packet from client: " << error << endl;
                break;
            }
            else if (bytesReceived == 0) {
                int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK) {
                    // Client has gracefully closed the connection
                    cout << "Client disconnected." << endl;
                    //cleanup
                }
                break;
            }
            else {
                // Process packet
                Reader* ProcessResult;
                ProcessResult = ProcessPacket(packet, clientSocket);
                if (!ProcessResult) {
                    //user couldn't login. disconnect
                    break;
                }
                else if (ProcessResult->SessionId == "Ok") {
                    delete ProcessResult;
                }
                else {
                    HandledUser = ProcessResult;
                }
            }
        }
        
        //poll notifications
        if (HandledUser) {
            if (!HandledUser->Notifications.empty()) {
                SendUserNotification(clientSocket, HandledUser->Notifications.front());
                HandledUser->Notifications.erase(HandledUser->Notifications.begin());
            }
        }

        //limit CPU usage
        this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    cout << "Finished handling client" << endl;
    // Close the client socket
    closesocket(clientSocket);
}

Reader* Server::ProcessPacket(const Packet& packet, SOCKET clientSocket) {

    Packet response;

    // Process the packet according to the action
    switch (packet.action) {
    case CREATE_ACCOUNT:{
        cout << "Received request to create account for user: " << packet.Username << endl;

        Reader* NewUser = System->CreateAccount(packet.Username, packet.Password);

        if (!NewUser) {
            cout << "Failed to create new account" << endl;
            response.action = LOGIN_FAIL;
        }
        else {
            string sessionId = GenerateSessionId();
            ActiveSessionIds.push_back(sessionId);

            SessionReaders[sessionId] = NewUser;

            response.action = LOGIN_SUCCESS;
            strncpy_s(response.SessionId, sessionId.c_str(), _TRUNCATE);
        }

        // Send packet to client
        send(clientSocket, reinterpret_cast<const char*>(&response), sizeof(response), 0);
        return NewUser;
    }
    case LOGIN: {
        cout << "Received login request for user: " << packet.Username << endl;

        Reader* NewUser = System->Login(packet.Username, packet.Password);

        if (!NewUser) {
            cout << "Someone tried to login as " << packet.Username << " with invalid login credentials" << endl;
            response.action = LOGIN_FAIL;
        }
        else {
            string sessionId = GenerateSessionId();
            ActiveSessionIds.push_back(sessionId);

            SessionReaders[sessionId] = NewUser;

            response.action = LOGIN_SUCCESS;
            strncpy_s(response.SessionId, sessionId.c_str(), _TRUNCATE);
        }

        // Send packet to client
        send(clientSocket, reinterpret_cast<const char*>(&response), sizeof(response), 0);
        return NewUser; 
    }
    case CLOSE_SESSION:
    {
        cout << "Closing session for user: " << SessionReaders[packet.SessionId]->Username << endl;
        ActiveSessionIds.erase(std::remove(ActiveSessionIds.begin(), ActiveSessionIds.end(), packet.SessionId), ActiveSessionIds.end());

        auto it = SessionReaders.find(packet.SessionId);
        if (it != SessionReaders.end()) {
            SessionReaders.erase(it);
        }
        else {
            std::cout << "User associated with session ID " << packet.SessionId << " not found." << std::endl;
        }
        return nullptr;
    }
    case BORROW_BOOK: 
        if (!VerifySessionId(packet.SessionId)) {
            UserVerificationFailed(clientSocket);
            break;
        }
        cout << "Processing request to borrow book: " << packet.Title << " by user: " << SessionReaders[packet.SessionId]->Username << endl;

        if (!System->BorrowBook(SessionReaders[packet.SessionId], packet.Title)) {
            response.action = BORROW_FAIL;
        }
        else {
            response.action = BORROW_BOOK;
        }
        strncpy_s(response.Title, packet.Title, _TRUNCATE);
        send(clientSocket, reinterpret_cast<const char*>(&response), sizeof(response), 0);
        break;
    case RETURN_BOOK:
        if (!VerifySessionId(packet.SessionId)) {
            UserVerificationFailed(clientSocket);
            break;
        }

        cout << "Processing request to return a book: " << packet.Title << " by user: " << SessionReaders[packet.SessionId]->Username << endl;

        if (!System->ReturnBook(SessionReaders[packet.SessionId], packet.Title)) {
            response.action = RETURN_FAIL;
        }
        else {
            response.action = RETURN_BOOK;
        }

        strncpy_s(response.Title, packet.Title, _TRUNCATE);
        send(clientSocket, reinterpret_cast<const char*>(&response), sizeof(response), 0);
        break;
    case SUBSCRIBE_BOOK:
        if (!VerifySessionId(packet.SessionId)) {
            UserVerificationFailed(clientSocket);
            break;
        }

        cout << "Processing request to subscribe to book: " << packet.Title << " by user: " << SessionReaders[packet.SessionId]->Username << endl;

        if (!System->SubscribeUserToBook(SessionReaders[packet.SessionId], packet.Title)){
            response.action = SUBSCRIBE_FAIL;
        }
        else {
            response.action = SUBSCRIBE_BOOK;
        }

        strncpy_s(response.Title, packet.Title, _TRUNCATE);
        send(clientSocket, reinterpret_cast<const char*>(&response), sizeof(response), 0);
        break;
    case AVAILABILITY_BOOK:
        if (!VerifySessionId(packet.SessionId)) {
            UserVerificationFailed(clientSocket);
            break;
        }

        cout << "Processing request to check availability of book: " << packet.Title << " by user: " << SessionReaders[packet.SessionId]->Username << endl;

        if (System->CheckBookAvailibility(packet.Title) != BookStatus::available) {
            response.action = BOOK_UNAVAILABLE;
        }
        else {
            response.action = BOOK_AVAILABLE;
        }

        strncpy_s(response.Title, packet.Title, _TRUNCATE);
        send(clientSocket, reinterpret_cast<const char*>(&response), sizeof(response), 0);
        break;
    case REQUEST_CATALOGUE:
        if (!VerifySessionId(packet.SessionId)) {
            UserVerificationFailed(clientSocket);
            break;
        }

        cout << "Processing request to show catalogue by user: " << SessionReaders[packet.SessionId]->Username << endl;

        for (auto& book : System->ShowCatalogue()) {
            response.action = BOOK_INFO;
            strncpy_s(response.Title, book->GetTitle().c_str(), _TRUNCATE);
            strncpy_s(response.Author, book->GetAuthor().c_str(), _TRUNCATE);
            send(clientSocket, reinterpret_cast<const char*>(&response), sizeof(response), 0);
        }
        break;
    case REQUEST_OWNED:
        if (!VerifySessionId(packet.SessionId)) {
            UserVerificationFailed(clientSocket);
            break;
        }

        cout << "Processing request to show owned books by user: " << SessionReaders[packet.SessionId]->Username << endl;

        for (auto& book : System->ShowOwned(SessionReaders[packet.SessionId])) {
            response.action = BOOK_INFO;
            strncpy_s(response.Title, book->GetTitle().c_str(), _TRUNCATE);
            strncpy_s(response.Author, book->GetAuthor().c_str(), _TRUNCATE);
            send(clientSocket, reinterpret_cast<const char*>(&response), sizeof(response), 0);
        }
        break;
    default:
        cout << "Unexpected user action encountered" << endl;
    }
    return new Reader("Ok", SOCKET_ERROR);
}

string Server::GenerateSessionId()
{
    return "SESSION_" + to_string(rand());
}


bool Server::VerifySessionId(const string& sessionId)
{
    return find(ActiveSessionIds.begin(), ActiveSessionIds.end(), sessionId) != ActiveSessionIds.end();
}

void Server::SendUserNotification(SOCKET clientSocket, string Message)
{
    Packet response;
    response.action = NOTIFICATION;
    strncpy_s(response.Username, Message.c_str(), _TRUNCATE);

    send(clientSocket, reinterpret_cast<const char*>(&response), sizeof(response), 0);
}

void Server::UserVerificationFailed(SOCKET clientSocket)
{
    cout << "User sent a request with invalid session id!" << endl;

    Packet response;
    response.action = LOGIN_FAIL;

    send(clientSocket, reinterpret_cast<const char*>(&response), sizeof(response), 0);
}
