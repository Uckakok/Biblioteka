#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <mutex>
#include <thread>
#include "LibrarySystem.h"
#include <unordered_map>
#include "Structs.h"
#include <iostream>

using namespace std;

class Server {
public:
    static Server* GetInstance() {
        if (Instance) {
            return Instance;
        }
        Instance = new Server();
        Instance->Start();
        return Instance;
    }

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

private:
    void SendUserNotification(SOCKET clientSocket, string Message);

    LibrarySystem* System;
    // Method to start the server
    void Start();

    void HandleClient(SOCKET clientSocket);
    static Server* Instance;
    Server() = default;

    SOCKET serverSocket;
    vector<string> ActiveSessionIds;
    unordered_map<string, Reader*> SessionReaders;

    // Method to accept incoming connections and handle packets
    void AcceptConnections();

    // Method to process received packet
    Reader* ProcessPacket(const Packet& packet, SOCKET clientSocket);

    // Method to generate session Id for user. Needs improvement
    string GenerateSessionId();

    // Verifies if session id is recognized
    bool VerifySessionId(const string& sessionId);

    // Call when user sent a request with incorrect session id
    void UserVerificationFailed(SOCKET clientSocket);
};