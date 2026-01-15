#include "../include/StompProtocol.h"
#include <sstream>

StompProtocol::StompProtocol() 
    : connectionHandler(nullptr), nextSubscriptionId(0), nextReceiptId(0), 
      channelToSubscriptionId(), username("") {}

StompProtocol::~StompProtocol() {}

void StompProtocol::setConnectionHandler(ConnectionHandler* handler) {
    connectionHandler = handler;
}

void StompProtocol::setUsername(const std::string& user) {
    username = user;
}

const std::string& StompProtocol::getUsername() const {
    return username;
}

int StompProtocol::generateSubscriptionId() {
    return nextSubscriptionId++;
}

int StompProtocol::generateReceiptId() {
    return nextReceiptId++;
}

// Build CONNECT frame
std::string StompProtocol::buildConnect(const std::string& username, const std::string& password, const std::string& host) {
    std::ostringstream frame;
    frame << "CONNECT\n";
    frame << "accept-version:1.2\n";
    frame << "host:" << host << "\n";
    frame << "login:" << username << "\n";
    frame << "passcode:" << password << "\n";
    frame << "\n";  // blank line (end of headers)
    frame << '\0';  // null terminator
    return frame.str();
}

// Build SUBSCRIBE frame
std::string StompProtocol::buildSubscribe(const std::string& channel) {
    int subId = generateSubscriptionId();
    int receiptId = generateReceiptId();
    
    std::ostringstream frame;
    frame << "SUBSCRIBE\n";
    frame << "destination:/" << channel << "\n";
    frame << "id:" << subId << "\n";
    frame << "receipt:" << receiptId << "\n";
    frame << "\n";
    frame << '\0';
    
    // Track subscription
    channelToSubscriptionId[channel] = subId;
    
    return frame.str();
}

// Build UNSUBSCRIBE frame
std::string StompProtocol::buildUnsubscribe(const std::string& channel) {
    if (!isSubscribed(channel)) {
        return "";  // Error: not subscribed
    }
    
    int subId = channelToSubscriptionId[channel];
    int receiptId = generateReceiptId();
    
    std::ostringstream frame;
    frame << "UNSUBSCRIBE\n";
    frame << "id:" << subId << "\n";
    frame << "receipt:" << receiptId << "\n";
    frame << "\n";
    frame << '\0';
    
    return frame.str();
}

// Build SEND frame
std::string StompProtocol::buildSend(const std::string& channel, const std::string& body) {
    std::ostringstream frame;
    frame << "SEND\n";
    frame << "destination:/" << channel << "\n";
    frame << "\n";  // blank line before body
    frame << body;
    frame << '\0';
    return frame.str();
}

// Build DISCONNECT frame
std::string StompProtocol::buildDisconnect() {
    int receiptId = generateReceiptId();
    
    std::ostringstream frame;
    frame << "DISCONNECT\n";
    frame << "receipt:" << receiptId << "\n";
    frame << "\n";
    frame << '\0';
    return frame.str();
}

// Extract command from frame
std::string StompProtocol::getCommand(const std::string& frame) {
    size_t pos = frame.find('\n');
    if (pos == std::string::npos) {
        return "";
    }
    return frame.substr(0, pos);
}

// Extract headers from frame
std::map<std::string, std::string> StompProtocol::getHeaders(const std::string& frame) {
    std::map<std::string, std::string> headers;
    
    size_t start = frame.find('\n') + 1;  // skip command line
    size_t end = frame.find("\n\n", start);  // find blank line
    
    if (end == std::string::npos) {
        return headers;
    }
    
    std::string headerSection = frame.substr(start, end - start);
    std::istringstream stream(headerSection);
    std::string line;
    
    while (std::getline(stream, line)) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            headers[key] = value;
        }
    }
    
    return headers;
}

// Extract body from frame
std::string StompProtocol::getBody(const std::string& frame) {
    size_t blankLinePos = frame.find("\n\n");
    if (blankLinePos == std::string::npos) {
        return "";
    }
    
    size_t bodyStart = blankLinePos + 2;
    size_t nullPos = frame.find('\0', bodyStart);
    
    if (nullPos == std::string::npos) {
        return frame.substr(bodyStart);
    }
    
    return frame.substr(bodyStart, nullPos - bodyStart);
}

// Subscription tracking
bool StompProtocol::isSubscribed(const std::string& channel) {
    return channelToSubscriptionId.find(channel) != channelToSubscriptionId.end();
}

int StompProtocol::getSubscriptionId(const std::string& channel) {
    if (isSubscribed(channel)) {
        return channelToSubscriptionId[channel];
    }
    return -1;
}

void StompProtocol::addSubscription(const std::string& channel, int subscriptionId) {
    channelToSubscriptionId[channel] = subscriptionId;
}

void StompProtocol::removeSubscription(const std::string& channel) {
    channelToSubscriptionId.erase(channel);
}
