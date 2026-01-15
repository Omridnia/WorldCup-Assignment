package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        if (args.length < 2) {
            System.out.println("Usage: StompServer <port> <tcp|reactor>");
            return;
        }

        int port = Integer.parseInt(args[0]);
        String serverType = args[1];

        if (serverType.equalsIgnoreCase("tcp")) {
            StompTCPServer<String> server = new StompTCPServer<>(
                port,
                () -> new StompMessagingProtocolImpl(),
                () -> new StompMessageEncoderDecoder()
            );
            server.serve();
        } else if (serverType.equalsIgnoreCase("reactor")) {
            // Create Connections instance (like TPC does)
            Connections<String> connections;
            try {
                connections = (Connections<String>) Class.forName("bgu.spl.net.impl.stomp.ConnectionsImpl")
                    .getDeclaredConstructor().newInstance();
            } catch (Exception e) {
                System.err.println("Failed to instantiate ConnectionsImpl: " + e.getMessage());
                return;
            }
        
            // Create and run reactor server
            Server<String> server = Server.reactor(
                10,  // number of threads in thread pool
                port,
                () -> new StompMessagingProtocolImpl(),
                () -> new StompMessageEncoderDecoder(),
                connections
            );
            server.serve();
        } else {
            System.err.println("Unknown server type: " + serverType + ". Use 'tcp' or 'reactor'");
        }
    }
}
    


