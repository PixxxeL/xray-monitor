#include "State.h"
#include <chrono>


void State::updateUser(const std::string& email, const std::string& ip, bool connected, uint64_t downlink, uint64_t uplink) {
    auto it = users.find(email);
    if (it != users.end()) {
        it->second.connected = connected;
        it->second.lastSeen = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if (!ip.empty()) {
            it->second.ip = ip;
        }
        if (downlink > 0) it->second.downlink = downlink;
        if (uplink > 0) it->second.uplink = uplink;
        // @TODO: Update lastSeen too?
    }
    else {
        User user;
        user.email = email;
        user.ip = ip;
        user.connected = connected;
        user.lastSeen = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        user.downlink = downlink;
        user.uplink = uplink;
        users[email] = user;
    }
}

void State::removeUser(const std::string& email) {
    users.erase(email);
}

User State::getUser(const std::string& email) const {
    auto it = users.find(email);
    if (it != users.end()) {
        return it->second;
    }
    return User();
}

std::unordered_map<std::string, User> State::getUsers() const {
    return users;
}

std::unordered_map<std::string, User> State::getConnectedUsers() const {
    std::unordered_map<std::string, User> connected;
    for (const auto& pair : users) {
        if (pair.second.connected) {
            connected[pair.first] = pair.second;
        }
    }
    return connected;
}

std::unordered_map<std::string, User> State::getDisconnectedUsers() const {
    std::unordered_map<std::string, User> disconnected;
    for (const auto& pair : users) {
        if (!pair.second.connected) {
            disconnected[pair.first] = pair.second;
        }
    }
    return disconnected;
}

void State::disconnectAllUsers() {
    for (auto& [email, user] : users) {
        user.connected = false;
    }
}
