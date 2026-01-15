#pragma once

#include "../include/ConnectionHandler.h"
#include <string>
#include <map>

class StompProtocol
{
private:
    ConnectionHandler* connectionHandler;
    int nextSubscriptionId;
    int nextReceiptId;
    std::map<std::string, int> channelToSubscriptionId;  // channel to subscription id
    std::string username;

    // helper methods
    int generateSubscriptionId();
    int generateReceiptId();

public:
    StompProtocol();
    ~StompProtocol();

    // setup
    void setConnectionHandler(ConnectionHandler* handler);
    void setUsername(const std::string& user);
    const std::string& getUsername() const;

    // frame builders, returns frame strings ready to send)
    std::string buildConnect(const std::string& username, const std::string& password, const std::string& host);
    std::string buildSubscribe(const std::string& channel);
    std::string buildUnsubscribe(const std::string& channel);
    std::string buildSend(const std::string& channel, const std::string& body);
    std::string buildDisconnect();

    // frame parsers, extracts info from received frames
    std::string getCommand(const std::string& frame);
    std::map<std::string, std::string> getHeaders(const std::string& frame);
    std::string getBody(const std::string& frame);

    // subscription track
    bool isSubscribed(const std::string& channel);
    int getSubscriptionId(const std::string& channel);
    void addSubscription(const std::string& channel, int subscriptionId);
    void removeSubscription(const std::string& channel);
};
