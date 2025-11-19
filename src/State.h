#ifndef STATE_H
#define STATE_H

#include <unordered_map>
#include <string>
#include <cstdint>


struct User {
    std::string id;
    std::string email;
    std::string ip;
    bool connected = false;
    time_t lastSeen = 0;
    uint64_t downlink = 0;
    uint64_t uplink = 0;
};

class State {
public:
    State() = default;

    void updateUser(const std::string& email, const std::string& ip, bool connected, uint64_t downlink = 0, uint64_t uplink = 0);
    void removeUser(const std::string& email);
    User getUser(const std::string& email) const;
    std::unordered_map<std::string, User> getUsers() const;
    std::unordered_map<std::string, User> getConnectedUsers() const;
    std::unordered_map<std::string, User> getDisconnectedUsers() const;
    void disconnectAllUsers();

    void setRunning(bool running) { isRunning = running; }
    bool getRunning() const { return isRunning; }

private:
    std::unordered_map<std::string, User> users;
    bool isRunning{ true };
};

#endif
