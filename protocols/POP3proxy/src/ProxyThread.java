import javax.net.ssl.SSLSocketFactory;
import java.io.*;
import java.net.Socket;
import java.net.SocketAddress;
import java.util.logging.Level;
import java.util.logging.Logger;

public class ProxyThread extends Thread {
    private Socket clientSocket;
    private Socket serverSocket;
    private SocketAddress clientAddr;
    private MainThread ST;

    private String inputLine;

    private static int SERVER_PORT=995;
    private String SERVER_IP;

    public ProxyThread(Socket clientSocket, MainThread ST) {
        this.clientSocket = clientSocket;
        this.ST=ST;
        clientAddr = this.clientSocket.getRemoteSocketAddress();
    }

    public void run() {
        try{
            System.out.println("New connection: " + getAddr());

            PrintWriter outClient = new PrintWriter(clientSocket.getOutputStream(), true);
            BufferedReader inClient = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));

            SERVER_IP = getServerAddress(inClient, outClient);

            SSLSocketFactory fac = (SSLSocketFactory) SSLSocketFactory.getDefault();
            serverSocket = fac.createSocket(SERVER_IP, SERVER_PORT);

            PrintWriter outServer = new PrintWriter(serverSocket.getOutputStream(), true);
            BufferedReader inServer = new BufferedReader(new InputStreamReader(serverSocket.getInputStream()));

            Thread t = new Thread(() -> responseRedirect(inServer, outClient));
            t.start();

            // Get +OK respond from server and send USER
            inServer.readLine();
            outServer.println(inputLine);

            System.out.println("Connected to " + SERVER_IP + ":" + SERVER_PORT);

            requestRedirect(inClient, outServer);
        }catch (IOException e) {
            ST.removeClient(this);
            System.out.println("Disconnected: " + getAddr() +" (" + e.getMessage() + ")");
            try {
                if(serverSocket != null) serverSocket.close();
                clientSocket.close();
                System.out.println("Socket closed.");
            } catch (IOException ex) {
                Logger.getLogger(ProxyThread.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
    }

    private void requestRedirect(BufferedReader in, PrintWriter outEcho) throws IOException {
        while ((inputLine = in.readLine()) != null) {
            outEcho.println(inputLine);
            if(inputLine.contains("PASS")) inputLine = "PASS **********";
            System.out.println(getAddr() + "\tclient -> " + inputLine);
        }
        throw new IOException("Dropped from client");
    }

    private void responseRedirect(BufferedReader inEcho, PrintWriter out) {
        StringBuilder response = new StringBuilder();
        String line;
        try {
            while((line = inEcho.readLine()) != null){
                response.append(line);
                response.append("\r\n");

                if(inputLine.matches("(RETR|TOP|LIST|UIDL)(.*)") && !line.equals(".")) continue;
                if(!inEcho.ready()){
                    String stdoutString = response.toString();
                    if(inputLine.matches("(RETR|TOP)(.*)"))
                        stdoutString = stdoutString.split("\r\n", 2)[0] + " <...>";
                    System.out.println(SERVER_IP + ":" + SERVER_PORT + "\tserver -> " + stdoutString);
                    out.println(response.toString());

                    if(inputLine.contains("PASS") && response.toString().contains("-ERR")) finish();
                    response.setLength(0);
                }

                if(inputLine.equals("QUIT") && inputLine != null) finish();
            }
        } catch (IOException | NullPointerException e) {}
    }

    private String getServerAddress(BufferedReader in, PrintWriter out) throws IOException {
        inputLine = "";
        String ip = null;
        String proxyResponse = "+OK POP3";
        boolean work = true;

        out.println(proxyResponse);

        while(work && (inputLine = in.readLine()) != null){
            System.out.println(getAddr() + "\tclient -> " + inputLine);
            if(!inputLine.contains("USER")){
                if(inputLine.contains("QUIT")) {
                    proxyResponse = "+OK Bye";
                    System.out.println("proxy -> " + proxyResponse);
                    out.println(proxyResponse);
                    finish();
                }else{
                    if (inputLine.contains("AUTH")) proxyResponse = "-ERR What?";
                    if (inputLine.contains("CAPA"))
                        proxyResponse = "+OK Capability list follows\r\nUSER\r\n.";
                    System.out.println(proxyResponse);
                    out.println(proxyResponse);
                }
            }else {
                String[] login = inputLine.trim().split("@");
                try {
                    ip = "pop." + login[1];
                }catch (ArrayIndexOutOfBoundsException e){
                    proxyResponse = "-ERR Wrong login, please use full email address";
                    out.println(proxyResponse);
                    finish();
                }
                work = false;
            }
        }

        return ip;
    }

    public void finish(){
        try{
            if(serverSocket != null) serverSocket.close();
            clientSocket.close();
        }catch(final IOException e){
            System.out.println("Can't close socket (" + e.getMessage() + ")");
        }
    }

    public String getAddr(){
        return clientAddr.toString().replace("/", "");
    }
}
