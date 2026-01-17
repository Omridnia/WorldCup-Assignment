package bgu.spl.net.impl.data;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

public class SqlClient {
    private final String host;
    private final int port;

    public SqlClient(String host, int port) {
        this.host = host;
        this.port = port;
    }

    public String send(String sql) {
        try (Socket socket = new Socket(host, port);
             PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
             BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()))) {

            out.print(sql + '\0');
            out.flush();

            StringBuilder response = new StringBuilder();
            int ch;
            while ((ch = in.read()) != -1 && ch != '\0') {
                response.append((char) ch);
            }
            return response.toString();
        } catch (Exception e) {
            return "ERROR:" + e.getMessage();
        }
    }
}
