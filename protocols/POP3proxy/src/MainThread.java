import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Map;
import java.util.Scanner;
import java.util.concurrent.ConcurrentHashMap;

public class MainThread implements Runnable {
    private int           SERVER_PORT = 12555;
    private String        SERVER_IP   = "127.0.0.1";

    private ServerSocket serverSocket = null;
    private boolean       isStopped    = false;

    private ConcurrentHashMap<ProxyThread, String> mClients;

    private Scanner s = new Scanner(System.in);

    public void run(){
        Runnable server = () -> {
            while(!isStopped){
                String input = s.next();
                switch (input) {
                    case "l":
                        showClients();
                        break;
                    case "k":
                        int id=Integer.parseInt(s.next());
                        deleteClientById(id);
                        break;
                    case "q":
                        stop();
                        break;
                }
            }
        };
        new Thread(server).start();
        mClients = new ConcurrentHashMap<>();
        openServerSocket();

        while(!isStopped){
            Socket clientSocket;
            try {
                clientSocket = this.serverSocket.accept();
            } catch (IOException e) {
                System.out.println("Can't accept socket.");
                return;
            }
            addClient(clientSocket);
        }
    }

    private void addClient(Socket clientSocket){
        ProxyThread CT=new ProxyThread(clientSocket, this);
        CT.start();
        synchronized (mClients) {
            mClients.put(CT, "");
        }
    }
    private synchronized void deleteClientById(int id){
        int i=1;
        for (Map.Entry<ProxyThread, String> entry : mClients.entrySet()) {
            ProxyThread key = entry.getKey();
            if(i==id){
                key.finish();
                mClients.remove(key);
                return;
            }
            i++;
        }
        System.out.println("Wrong ID");
    }
    public synchronized void removeClient(ProxyThread CT){
        synchronized (mClients) {
            mClients.remove(CT);
        }
    }
    private synchronized void showClients(){
        int i=1;
        for (Map.Entry<ProxyThread, String> entry : mClients.entrySet()) {
            ProxyThread key = entry.getKey();
            System.out.println(i+"|"+key.getAddr());
            i++;
        }
        if(mClients.size() == 0) System.out.println("Empty");
    }

    public synchronized void stop(){
        for (Map.Entry<ProxyThread, String> entry : mClients.entrySet()) {
            ProxyThread key = entry.getKey();
            key.finish();
            mClients.remove(key);
        }
        this.isStopped = true;
        try {
            this.serverSocket.close();
            System.out.println("Server stopped.");
        } catch (IOException e) {
            System.out.println("Error closing server (" + e + ")");
        }
    }

    private void openServerSocket() {
        try {
            this.serverSocket = new ServerSocket(this.SERVER_PORT,0, InetAddress.getByName(SERVER_IP));
        } catch (IOException e) {
            throw new RuntimeException("Cannot open port " + SERVER_PORT, e);
        }
        System.out.println("Server started.") ;
    }
}
