#include "../include/ConnectionHandler.h"
#include "../include/StompProtocol.h"
#include "../include/event.h"
#include <thread>
#include <iostream>
#include <string>
#include <atomic>
#include <sstream>
#include <chrono>
#include <map>
#include <vector>
#include <fstream>
#include <algorithm>

// globals 
ConnectionHandler* connectionHandler = nullptr;
StompProtocol protocol;
std::atomic<bool> shouldTerminate(false);
std::atomic<bool> isLoggedIn(false);
// global storage for events
std::map<std::string, std::map<std::string, std::vector<Event>>> gameEvents;

// forward declarations
void socketListenerTask();
void keyboardTask();
void handleLogin(std::istringstream& iss);
void handleJoin(std::istringstream& iss);
void handleExit(std::istringstream& iss);
void handleLogout();
void handleReport(std::istringstream& iss);
void handleSummary(std::istringstream& iss);
void handleMessage(const std::string& frame);

int main(int argc, char *argv[]) {
	// create threads
    std::thread socketThread(socketListenerTask);
    std::thread keyboardThread(keyboardTask);
    
    // wait for both threads to end
    keyboardThread.join();
    socketThread.join();
    
    // cleanup
    if (connectionHandler != nullptr) {
        delete connectionHandler;
    }
	return 0;
}

void socketListenerTask() {
    while (!shouldTerminate) {
        if (connectionHandler == nullptr) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        std::string frame;
        if (!connectionHandler->getFrameAscii(frame, '\0')) {
            break;  // connection closed or error
        }

        std::string command = protocol.getCommand(frame);

		if (command == "CONNECTED") {
            std::cout << "Login successful" << std::endl;
            isLoggedIn = true;
        } else if (command == "RECEIPT") {
            auto headers = protocol.getHeaders(frame);
            std::cout << "Received RECEIPT for id: " << headers["receipt-id"] << std::endl;
        } else if (command == "ERROR") {
            auto headers = protocol.getHeaders(frame);
            std::cout << "Error: " << headers["message"] << std::endl;
            shouldTerminate = true;
		} else if (command == "MESSAGE") {
			handleMessage(frame);
		}
    }
}


void keyboardTask() {
    while (!shouldTerminate) {
        std::string line;
        std::getline(std::cin, line);
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "login") {
            handleLogin(iss);
        } else if (command == "join") {
            handleJoin(iss);
        } else if (command == "exit") {
            handleExit(iss);
        } else if (command == "logout") {
            handleLogout();
        } else if (command == "report") {
            handleReport(iss);
        } else if (command == "summary") {
            handleSummary(iss);
        }
    }
}

// Command handlers
void handleLogin(std::istringstream& iss) {
    std::string hostport, user, pass;
    iss >> hostport >> user >> pass;
    
    if (hostport.empty() || user.empty() || pass.empty()) {
        std::cout << "Usage: login {host:port} {username} {password}" << std::endl;
        return;
    }
    
    if (isLoggedIn) {
        std::cout << "The client is already logged in, log out before trying again" << std::endl;
        return;
    }
    
    // Split host:port
    auto colonPos = hostport.find(':');
    if (colonPos == std::string::npos) {
        std::cout << "Could not connect to server" << std::endl;
        return;
    }
    
    std::string host = hostport.substr(0, colonPos);
    short port = static_cast<short>(std::stoi(hostport.substr(colonPos + 1)));
    
    connectionHandler = new ConnectionHandler(host, port);
    if (!connectionHandler->connect()) {
        std::cout << "Could not connect to server" << std::endl;
        delete connectionHandler;
        connectionHandler = nullptr;
        return;
    }
    
    protocol.setConnectionHandler(connectionHandler);
    protocol.setUsername(user);
    std::string frame = protocol.buildConnect(user, pass, "stomp.cs.bgu.ac.il");
    connectionHandler->sendFrameAscii(frame, '\0');
}

void handleJoin(std::istringstream& iss) {
    if (!isLoggedIn) {
        std::cout << "Must login first" << std::endl;
        return;
    }
    
    std::string game_name;
    iss >> game_name;
    
    if (game_name.empty()) {
        std::cout << "Usage: join {game_name}" << std::endl;
        return;
    }
    
    std::string frame = protocol.buildSubscribe(game_name);
    connectionHandler->sendFrameAscii(frame, '\0');
    std::cout << "Joined channel " << game_name << std::endl;
}

void handleExit(std::istringstream& iss) {
    if (!isLoggedIn) {
        std::cout << "Must login first" << std::endl;
        return;
    }
    
    std::string game_name;
    iss >> game_name;
    
    if (game_name.empty()) {
        std::cout << "Usage: exit {game_name}" << std::endl;
        return;
    }
    
    std::string frame = protocol.buildUnsubscribe(game_name);
    if (frame.empty()) {
        std::cout << "Not subscribed to " << game_name << std::endl;
        return;
    }
    
    connectionHandler->sendFrameAscii(frame, '\0');
    protocol.removeSubscription(game_name);
    std::cout << "Exited channel " << game_name << std::endl;
}

void handleLogout() {
    if (!isLoggedIn) {
        std::cout << "Not logged in" << std::endl;
        return;
    }
    
    std::string frame = protocol.buildDisconnect();
    connectionHandler->sendFrameAscii(frame, '\0');
    // Will terminate after receiving RECEIPT in socket thread
}

void handleReport(std::istringstream& iss) {
    // Check if logged in first
	if (!isLoggedIn) {
		std::cout << "Must login first" << std::endl;
		return;
	}
    
    std::string filename;
    iss >> filename;
    // Validate filename is not empty
	if (filename.empty()) {
		std::cout << "Usage: report {file}" << std::endl;
		return;
	}
	
	names_and_events nne = parseEventsFile(filename);

	std::string channel = nne.team_a_name + "_" + nne.team_b_name;

	for (const Event& event : nne.events) {
    	// build the body string for this event
    	std::ostringstream body;
    
    	body << "user:" << protocol.getUsername() << "\n";
    	body << "team a:" << nne.team_a_name << "\n";
    	body << "team b:" << nne.team_b_name << "\n";
    	body << "event name:" << event.get_name() << "\n";
    	body << "time:" << event.get_time() << "\n";
    
    	// add game updates section
    	body << "general game updates:\n";
    	for (const auto& pair : event.get_game_updates()) {
        	body << pair.first << ":" << pair.second << "\n";
    	}
    
    	// add team a updates section
    	body << "team a updates:\n";
    	for (const auto& pair : event.get_team_a_updates()) {
        	body << pair.first << ":" << pair.second << "\n";
    	}
    
    	// add team b updates section
    	body << "team b updates:\n";
    	for (const auto& pair : event.get_team_b_updates()) {
        	body << pair.first << ":" << pair.second << "\n";
    	}
    
    	// add description
    	body << "description:\n";
    	body << event.get_discription() << "\n";
    
    	// build SEND frame and send it
    	std::string frame = protocol.buildSend(channel, body.str());
    	connectionHandler->sendFrameAscii(frame, '\0');

		// store the event locally for summary command
    	gameEvents[channel][protocol.getUsername()].push_back(event);
	}
	std::cout << "Report sent successfully" << std::endl;
	
}

void handleSummary(std::istringstream& iss) { 
	std::string game_file, username, output_file;
	iss >> game_file >> username >> output_file;
	if (game_file.empty() || username.empty() || output_file.empty()) {
		std::cout << "Usage: summary {game} {user} {output_file}" << std::endl;
		return;
	}
	if (gameEvents.find(game_file) == gameEvents.end() || 
    	gameEvents[game_file].find(username) == gameEvents[game_file].end()) {
    	std::cout << "No events found for this game and user" << std::endl;
    	return;
	}

	std::vector<Event>& events = gameEvents[game_file][username];

	std::map<std::string, std::string> generalGameUpdates;
	std::map<std::string, std::string> teamAUpdates;
	std::map<std::string, std::string> teamBUpdates;

	for (const Event& event : events) { // merge latest values
    	// general game updates
    	for (const auto& pair : event.get_game_updates()) {
        	generalGameUpdates[pair.first] = pair.second;
    	}
    
    	// team A updates
    	for (const auto& pair : event.get_team_a_updates()) {
        	teamAUpdates[pair.first] = pair.second;
    	}
    
    	// team B updates
    	for (const auto& pair : event.get_team_b_updates()) {
        	teamBUpdates[pair.first] = pair.second;
    	}
	}

	std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
    	return a.get_time() < b.get_time();
		});

	std::ofstream outFile("data/" + output_file);
	if (!outFile) {
    	std::cout << "Failed to open output file" << std::endl;
    	return;
	}	

	outFile << game_file << " " << username << std::endl << std::endl;

	//general game updates
	outFile << "General game updates:" << std::endl;
	for (const auto& pair : generalGameUpdates) {
    	outFile << "\t" << pair.first << ": " << pair.second << std::endl;
	}
	outFile << std::endl;

	//team A updates
	outFile << events[0].get_team_a_name() << " updates:" << std::endl;
	for (const auto& pair : teamAUpdates) {
    	outFile << "\t" << pair.first << ": " << pair.second << std::endl;
	}
	outFile << std::endl;

	//team B updates
	outFile << events[0].get_team_b_name() << " updates:" << std::endl;
	for (const auto& pair : teamBUpdates) {
    	outFile << "\t" << pair.first << ": " << pair.second << std::endl;
	}
	outFile << std::endl;


	for (const Event& event : events) { //write events
    	outFile << event.get_time() << " - " << event.get_name() << ":" << std::endl;
    	outFile << std::endl;
    	outFile << event.get_discription() << std::endl;
    	outFile << std::endl << std::endl;
	}

	outFile.close(); //close the file
	std::cout << "Summary written to " << output_file << std::endl;

}

void handleMessage(const std::string& frame) {
	std::map<std::string, std::string> headers = protocol.getHeaders(frame);
	std::string channel = headers["destination"];
	std::string body = protocol.getBody(frame);

	std::istringstream bodyStream(body);
	std::string line;

	// user line: "user:NAME"
	std::string username;
	if (std::getline(bodyStream, line)) {
		auto pos = line.find(':');
		username = (pos != std::string::npos) ? line.substr(pos + 1) : line;
		if (!username.empty() && username[0] == ' ') username.erase(0, 1);
	}

	// team a: NAME
	std::string teamA;
	if (std::getline(bodyStream, line)) {
		auto pos = line.find(':');
		teamA = (pos != std::string::npos) ? line.substr(pos + 1) : line;
		if (!teamA.empty() && teamA[0] == ' ') teamA.erase(0, 1);
	}

	// team b: NAME
	std::string teamB;
	if (std::getline(bodyStream, line)) {
		auto pos = line.find(':');
		teamB = (pos != std::string::npos) ? line.substr(pos + 1) : line;
		if (!teamB.empty() && teamB[0] == ' ') teamB.erase(0, 1);
	}

	// event name: NAME
	std::string eventName;
	if (std::getline(bodyStream, line)) {
		auto pos = line.find(':');
		eventName = (pos != std::string::npos) ? line.substr(pos + 1) : line;
		if (!eventName.empty() && eventName[0] == ' ') eventName.erase(0, 1);
	}

	// time: NUMBER
	int eventTime = 0;
	if (std::getline(bodyStream, line)) {
		auto pos = line.find(':');
		std::string t = (pos != std::string::npos) ? line.substr(pos + 1) : line;
		if (!t.empty() && t[0] == ' ') t.erase(0, 1);
		try { eventTime = std::stoi(t); } catch (...) { eventTime = 0; }
	}

	// general game updates:
	std::map<std::string, std::string> gameUpdates;
	if (std::getline(bodyStream, line)) {
		// expect "general game updates:"
	}
	while (std::getline(bodyStream, line)) {
		if (line == "team a updates:") break;
		auto pos = line.find(':');
		if (pos != std::string::npos) {
			std::string key = line.substr(0, pos);
			std::string val = line.substr(pos + 1);
			if (!val.empty() && val[0] == ' ') val.erase(0, 1);
			gameUpdates[key] = val;
		}
	}

	// team a updates:
	std::map<std::string, std::string> teamAUpdates;
	while (std::getline(bodyStream, line)) {
		if (line == "team b updates:") break;
		auto pos = line.find(':');
		if (pos != std::string::npos) {
			std::string key = line.substr(0, pos);
			std::string val = line.substr(pos + 1);
			if (!val.empty() && val[0] == ' ') val.erase(0, 1);
			teamAUpdates[key] = val;
		}
	}

	// team b updates:
	std::map<std::string, std::string> teamBUpdates;
	while (std::getline(bodyStream, line)) {
		if (line == "description:") break;
		auto pos = line.find(':');
		if (pos != std::string::npos) {
			std::string key = line.substr(0, pos);
			std::string val = line.substr(pos + 1);
			if (!val.empty() && val[0] == ' ') val.erase(0, 1);
			teamBUpdates[key] = val;
		}
	}

	// description: collect remaining lines
	std::string description;
	std::string descLine;
	bool firstDesc = true;
	while (std::getline(bodyStream, descLine)) {
		if (!firstDesc) description += "\n";
		description += descLine;
		firstDesc = false;
	}

	Event event(eventName, teamA, teamB, eventTime, gameUpdates, teamAUpdates, teamBUpdates, description);
	gameEvents[channel][username].push_back(event);

	std::cout << "Received event from " << username << " in " << channel << std::endl;
}