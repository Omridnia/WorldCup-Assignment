package bgu.spl.net.impl.data;

import java.util.ArrayList;
import java.util.concurrent.ConcurrentHashMap;

public class User {
	public final String name;
	public final String password;
	private int connectionId;
	private boolean isLoggedIn = false;
	private final ConcurrentHashMap<String, String> subscriptions = new ConcurrentHashMap<>();

	public User(int connectionId, String name, String password) {
		this.connectionId = connectionId;
		this.name = name;
		this.password = password;
	}

	public boolean isLoggedIn() {
		return isLoggedIn;
	}

	public void login() {
		isLoggedIn = true;
	}

	public void logout() {
		isLoggedIn = false;
	}

	public int getConnectionId() {
		return connectionId;
	}

	public void setConnectionId(int connectionId) {
		this.connectionId = connectionId;
	}

	public ConcurrentHashMap<String, String> getSubscribedChannels(){
		return subscriptions;
	}

	public void subscribe(String channel, String subscriptionId){
		subscriptions.put(channel, subscriptionId);
	}

	public void unsubscribe(String subscriptionId){
		for (String id : new ArrayList<>(subscriptions.values())) {
    		if (id.equals(subscriptionId)) {
        		subscriptions.values().remove(id);
        		break;
    		}
		}
	}
}
