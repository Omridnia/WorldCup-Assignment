package bgu.spl.net.impl.stomp;

import bgu.spl.net.impl.data.Database;
import bgu.spl.net.impl.data.LoginStatus;
import bgu.spl.net.impl.data.User;
import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.impl.data.LoginStatus;
import bgu.spl.net.srv.Connections;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<String> {

    private int connectionId;
    private Connections<String> connections;
    private boolean shouldTerminate = false;


    @Override
    public void start(int connectionId, Connections<String> connections) {
        this.connectionId = connectionId;
        this.connections = connections;
    }

    @Override
    public String process(String message) {
        String[] lines = message.split("\n");
        String command = lines[0];  // "CONNECT"

        // For CONNECT command, find login and passcode
        if (command.equals("CONNECT")) {
            String login = null;
            String passcode = null;
        
            for (int i = 1; i < lines.length; i++) {
                if (lines[i].startsWith("login:")) {
                    login = lines[i].substring(6); // Skip "login:"
                }
                if (lines[i].startsWith("passcode:")) {
                    passcode = lines[i].substring(9); // Skip "passcode:"
                }
            }

            if(login==null || passcode==null){
                // send ERROR and terminate
                connections.send(connectionId, "ERROR\nmessage:Missing credentials\n\n\0");
                shouldTerminate = true;
                return null;
            }
            
            // Try to login using Database
            // Send CONNECTED or ERROR based on result
            LoginStatus status = Database.getInstance().login(connectionId, login, passcode);
            if (status == LoginStatus.LOGGED_IN_SUCCESSFULLY || 
                status == LoginStatus.ADDED_NEW_USER) {
                // success - Send CONNECTED
                connections.send(connectionId, "CONNECTED\nversion:1.2\n\n\0");
            } else {
                // failure - Send ERROR and terminate
                connections.send(connectionId, "ERROR\nmessage:Login failed\n\n\0");
                shouldTerminate = true;
                return null;
                }


            System.out.println("Login: " + login);
            System.out.println("Passcode: " + passcode);
        }

        else if(command.equals("DISCONNECT")){
            Database.getInstance().logout(connectionId);
            shouldTerminate = true;
        }

        else if(command.equals("SEND")){
            // find destination header
            String destination = null;
            int bodyStartLine = -1;

            for (int i = 1; i < lines.length; i++) {
                if (lines[i].startsWith("destination:")) {
                destination = lines[i].substring(12).trim();
            }
                if (lines[i].isEmpty()) {
                    bodyStartLine = i + 1; // Body starts after empty line
                    break;
                }
            }
            
            // validate if destination exists
            if (destination == null) {
                connections.send(connectionId, "ERROR\nmessage:Missing destination\n\n\0");
                shouldTerminate = true;
                return null;
            }

            // validate if sender is subscribed to the channel
            User sender = Database.getInstance().getUserByConnectionId(connectionId);
            if (sender == null || !sender.getSubscribedChannels().containsKey(destination)) {
                connections.send(connectionId, "ERROR\nmessage:User not subscribed to channel\n\n\0");
                shouldTerminate = true;
                return null;
            }

            // extract body (everything after empty line)
            StringBuilder body = new StringBuilder();
            for (int i = bodyStartLine; i < lines.length; i++) {
            body.append(lines[i]).append("\n");
            }


            // build MESSAGE frame to send to subscribers
            String messageFrame = "MESSAGE\ndestination:" + destination + "\n\n" 
                + body.toString() + "\0";
    
            // send to all subscribers of this channel
            connections.send(destination, messageFrame);
            

        }

        else if(command.equals("SUBSCRIBE")){
            // Find destination and id headers
            String destination = null;
            String id = null;
            String receipt = null;

            for (int i = 1; i < lines.length; i++) {
                if (lines[i].startsWith("destination:")) {
                destination = lines[i].substring(12).trim();
                }
                if (lines[i].startsWith("id:")) {
                    id = lines[i].substring(3).trim();
                }
                if (lines[i].startsWith("receipt:")) {
                    receipt = lines[i].substring(8).trim();
                }
            }

            if(destination == null || id == null){
                connections.send(connectionId, "ERROR\nmessage:Missing subscription headers\n\n\0");
                shouldTerminate = true;
                return null;
            }
            Database.getInstance().subscribe(connectionId, destination, id);
            if (receipt != null) {
                connections.send(connectionId, "RECEIPT\nreceipt-id:" + receipt + "\n\n\0");
            }
        
        
        
        }
        

        else if (command.equals("UNSUBSCRIBE")) {
            // Find id and receipt headers
            String id = null;
            String receipt = null;
    
            for (int i = 1; i < lines.length; i++) {
                if (lines[i].startsWith("id:")) {
                id = lines[i].substring(3).trim();
                }
                if (lines[i].startsWith("receipt:")) {
                receipt = lines[i].substring(8).trim();
                }
            }
    
            // TODO: validate id and remove subscription.
            // Optionally send RECEIPT if provided.
            if(id == null){
                connections.send(connectionId, "ERROR\nmessage:Missing subscription id\n\n\0");
                shouldTerminate = true;
                return null;
            }
            Database.getInstance().unsubscribe(connectionId, id);
            if (receipt != null) {
                connections.send(connectionId, "RECEIPT\nreceipt-id:" + receipt + "\n\n\0");
            }
        }
        return null;  // STOMP sends responses via connections, not return values
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }
}