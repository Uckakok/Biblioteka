# Biblioteka
Projekt na inżynierię oprogramowania. Pozwala uruchomić serwer biblioteki oraz klienta biblioteki.

Użycie:
W "Structs.h" należy zmodyfikować IPADDRESS na adres na jakim biblioteka ma być hostowana. Zbudować projekt i uruchomić program z flagą -server, by odpalić jako serwer. Serwer wczytuje z pliku "books" oraz pliku "users" dostępne książki wraz z ich dostępnością, oraz z pliku users konta użytkowników biblioteki. 
Aby uruchomić klienta, należy normalnie odpalić plik .exe. Dane użytkownika zostaną wysłane przez serwer. 

Serwer jest w stanie obsługiwać wielu połączonych klientów jednocześnie.

UWAGA: aktualnie program nie ma żadnych zabezpieczeń i nie powinny być w nim wykorzystywane prawdziwe hasła.
