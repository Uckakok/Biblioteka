#pragma once

#include "Structs.h"
#include <string>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>

using namespace std;

class UserInterface {
public:
    ~UserInterface();

    UserInterface();

    void MainMenu();


private:

    // Method to create a new account
    void CreateAccount();

    // Method to login
    void Login();

    void SendPacket(const Packet& packet);
    void StartUserSession(string SessionId, SOCKET ServerSocket);
};