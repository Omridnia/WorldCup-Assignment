package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.MessageEncoderDecoder;
import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.BlockingConnectionHandler;
import bgu.spl.net.srv.ConnectionHandler;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.Server;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.function.Supplier;

public class StompTCPServer<T> implements Server<T> {

    private final int port;
    private final Supplier<StompMessagingProtocol<T>> protocolFactory;
    private final Supplier<MessageEncoderDecoder<T>> encdecFactory;
    private ServerSocket sock;
    private Connections<T> connections;
    private int connectionIdCounter = 1;

    public StompTCPServer(
            int port,
            Supplier<StompMessagingProtocol<T>> protocolFactory,
            Supplier<MessageEncoderDecoder<T>> encdecFactory) {

        this.port = port;
        this.protocolFactory = protocolFactory;
        this.encdecFactory = encdecFactory;
        this.sock = null;
        try {
            this.connections = (Connections<T>) Class.forName("bgu.spl.net.impl.stomp.ConnectionsImpl")
                .getDeclaredConstructor().newInstance();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public void serve() {
        try (ServerSocket serverSock = new ServerSocket(port)) {
            System.out.println("Server started");
            this.sock = serverSock;

            while (!Thread.currentThread().isInterrupted()) {
                Socket clientSock = serverSock.accept();
                System.out.println("Client connected");

                StompMessagingProtocol<T> protocol = protocolFactory.get();
                int connectionId = connectionIdCounter++;
                System.out.println("Connection ID: " + connectionId);

                BlockingConnectionHandler<T> handler = new BlockingConnectionHandler<>(
                        clientSock,
                        encdecFactory.get(),
                        protocol);

                try {
                    Class<?> connectionsClass = connections.getClass();
                    java.lang.reflect.Method addConnMethod = 
                        connectionsClass.getDeclaredMethod("addConnection", int.class, ConnectionHandler.class);
                    addConnMethod.invoke(connections, connectionId, handler);
                    System.out.println("Handler registered in connections");
                } catch (Exception e) {
                    System.out.println("Error registering handler: " + e.getMessage());
                    e.printStackTrace();
                }

                Thread handlerThread = new Thread(handler);
                handlerThread.start();
                System.out.println("Handler thread started");
                
                // give thread time to initialize streams
                try { Thread.sleep(10); } catch (InterruptedException e) {}
                
                protocol.start(connectionId, connections);
                System.out.println("Protocol started");
            }
        } catch (IOException ex) {
        }

        System.out.println("server closed!!!");
    }

    @Override
    public void close() throws IOException {
        if (sock != null)
            sock.close();
    }

}
