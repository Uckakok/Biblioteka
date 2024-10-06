#pragma once
#include <exception>
#include <string>

#define MAXPACKETTEXT   64
#define IPADDRESS       "192.168.0.202"
#define PORT            49833

enum Action {
    CREATE_ACCOUNT,
    LOGIN,
    LOGIN_SUCCESS,
    LOGIN_FAIL,
    CLOSE_SESSION,
    BORROW_BOOK,
    BORROW_FAIL,
    RETURN_BOOK,
    RETURN_FAIL,
    SUBSCRIBE_BOOK,
    SUBSCRIBE_FAIL,
    AVAILABILITY_BOOK,
    BOOK_AVAILABLE,
    BOOK_UNAVAILABLE,
    NOTIFICATION,
    REQUEST_CATALOGUE,
    REQUEST_OWNED,
    BOOK_INFO,
};

struct Packet {
    char Username[MAXPACKETTEXT]; // Username
    char Password[MAXPACKETTEXT]; // Password
    char SessionId[MAXPACKETTEXT];
    char Title[MAXPACKETTEXT];
    char Author[MAXPACKETTEXT];
    Action action;     // Action type
};


class LibraryException : public std::exception {
public:
    LibraryException(const std::string& message) : m_message(message) {}

    const char* what() const noexcept override {
        return m_message.c_str();
    }

private:
    std::string m_message;
};