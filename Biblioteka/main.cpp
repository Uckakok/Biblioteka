

#pragma comment(lib, "ws2_32.lib")

#include "LibrarySystem.h"
#include "iostream"
#include "string"
#include "Interface.h"
#include "Server.h"
#include <Windows.h>

using namespace std;

int main(int agrc, char* argv[]) {

	srand(static_cast<unsigned int>(std::time(nullptr)));

	bool bIsClient = true;
	if (agrc == 2 && strcmp(argv[1], "-server") == 0) {
		cout << "Running as server" << endl;
		Server::GetInstance();//Will automatically start with first GetInstance
		bIsClient = false;
	}
	else {
		UserInterface* CurrentInterface = new UserInterface();
		CurrentInterface->MainMenu();
	}

	cout << "Finished program for: " << (bIsClient ? "client" : "server") << endl;
	return 0;
}