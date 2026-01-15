package bgu.spl.net.impl.stomp;

import bgu.spl.net.impl.data.Database;
import bgu.spl.net.impl.data.User;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.ConnectionHandler;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class ConnectionsImpl<T> implements Connections<T> {

    // acts as a "phone book" mapping connection IDs to their network handlers
    private ConcurrentHashMap<Integer, ConnectionHandler<T>> connectionHandlers = new ConcurrentHashMap<>();

    @Override
    public boolean send(int connectionId, T msg) {
        // send message to specific connection
        ConnectionHandler<T> handler = connectionHandlers.get(connectionId);
        if (handler != null) {
            handler.send(msg);
            return true;
        }
        return false;
    }


    @Override
    public void send(String channel, T msg) {// send message to all users that subscribed to this channel
        Database db = Database.getInstance();
        for (Map.Entry<Integer, ConnectionHandler<T>> entry : connectionHandlers.entrySet()) {
            int connId = entry.getKey();
            ConnectionHandler<T> handler = entry.getValue();
            User user = db.getUserByConnectionId(connId);
            if (user != null && user.getSubscribedChannels().containsKey(channel)) {
                handler.send(msg);
            }
        }
    }

    @Override
    public void disconnect(int connectionId) {
        connectionHandlers.remove(connectionId);
    }

    // helper method - this method is called by the protocol when connections start)
    public void addConnection(int connectionId, ConnectionHandler<T> handler) {
        connectionHandlers.put(connectionId, handler);
    }


}