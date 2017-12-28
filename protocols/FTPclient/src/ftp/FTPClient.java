package ftp;

import java.io.*;
import java.net.Socket;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class FTPClient {
    private final String host;
    private final int port;
    private final String user;
    private final String password;

    private Socket socket;
    private BufferedReader in;
    private BufferedWriter out;

    private ConcurrentHashMap<Socket, String> mSockets;

    public enum FTPCommand {
        LIST, PASS, PASV, CWD, PWD, MKD, RMD, QUIT, RETR, SIZE, STOR, SYST, TYPE, USER, DELE, NLST
    }

    public FTPClient(String host, int port, String user, String password) {
        assert host != null;

        this.host = host;
        this.port = port == 0 ? 21 : port;
        this.user = user;
        this.password = password;
    }

    public void connect() throws IOException {
        socket = new Socket(host, port);

        in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
        out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));

        mSockets = new ConcurrentHashMap<>();

        login(in, out);
    }

    public synchronized void close() {
        try {
            if (socket != null) {
                request(FTPCommand.QUIT, out);
                longResponse(in);
                socket.close();

                for(Map.Entry<Socket, String> entry : mSockets.entrySet()){
                    Socket key = entry.getKey();
                    key.close();
                    mSockets.remove(key);
                }
            }
        } catch (IOException cause) {
            // Skip exceptions
        } finally {
            socket = null;
        }
    }

    public long size(String filename) throws IOException {
        request(FTPCommand.SIZE, out, filename);
        FTPResponse response = response(in);
        return response.getCode() == 213 ? Long.parseLong(response.getText()) : -1;
    }

    public FTPFile[] list(String dir) throws IOException {
        Socket socket = openChannel(in, out);
        request(FTPCommand.LIST, out, dir == null ? "/" : dir);
        response(in);

        BufferedInputStream input = new BufferedInputStream(socket.getInputStream());

        byte[] buffer = new byte[1024];
        StringBuilder buf = new StringBuilder();
        while (input.read(buffer) > 0) {
            buf.append(new String(buffer));
        }
        input.close();

        response(in);

        String[] lines = buf.toString().trim().split("\n");
        FTPFile[] files = new FTPFile[lines.length];
        for (int i = 0; i < lines.length; i++) {
            files[i] = new FTPFile(lines[i]);

        }

        return files;
    }

    public String cwd(String dir) throws IOException {
        request(FTPCommand.CWD, out, dir == null ? "/" : dir);

        FTPResponse response;
        do response = response(in);
        while (response.isDelimiter() || response.getCode() == 0);

        request(FTPCommand.PWD, out);

        String currentDir = response(in).getText();
        currentDir = currentDir.substring(1, currentDir.lastIndexOf("\""));

        return currentDir;
    }


    public void uploadFile(String filepath, String dir) throws Exception {

        File firstLocalFile = new File(filepath);

        String firstRemoteFile = dir + "/" + firstLocalFile.getName();
        InputStream inputStream = new FileInputStream(firstLocalFile);

        Socket newSocket = new Socket(host, port);

        synchronized (mSockets) {
            mSockets.put(newSocket, "");
        }

        BufferedReader newIn = new BufferedReader(new InputStreamReader(newSocket.getInputStream()));
        BufferedWriter newOut = new BufferedWriter(new OutputStreamWriter(newSocket.getOutputStream()));

        Socket socket = getTransmissionSocket(newIn, newOut);

        request(FTPCommand.STOR, newOut, firstRemoteFile);
        FTPResponse response = response(newIn);
        if (!(response.getCode() == 150 || response.getCode() == 226)) {
            return;
        }

        BufferedInputStream input = new BufferedInputStream(inputStream);
        BufferedOutputStream output = new BufferedOutputStream(socket.getOutputStream());

        byte[] buffer = new byte[4096];
        int bytesRead;
        while ((bytesRead = input.read(buffer)) != -1) {
            output.write(buffer, 0, bytesRead);
        }
        output.flush();
        output.close();
        input.close();

        response(newIn);

        synchronized (mSockets) {
            mSockets.remove(newSocket);
        }

        newSocket.close();
    }

    public void getFile(String filename, String path) throws Exception {

        File firstLocalFile = new File(path);
        OutputStream outputStream = new FileOutputStream(firstLocalFile);

        Socket newSocket = new Socket(host, port);

        synchronized (mSockets) {
            mSockets.put(newSocket, "");
        }

        BufferedReader newIn = new BufferedReader(new InputStreamReader(newSocket.getInputStream()));
        BufferedWriter newOut = new BufferedWriter(new OutputStreamWriter(newSocket.getOutputStream()));

        Socket socket = getTransmissionSocket(newIn, newOut);

        request(FTPCommand.RETR, newOut, filename);
        FTPResponse response = response(newIn);
        if (!(response.getCode() == 150 || response.getCode() == 226)) {
            return;
        }

        BufferedInputStream input = new BufferedInputStream(socket.getInputStream());

        byte[] buffer = new byte[4096];
        int bytesRead;
        while ((bytesRead = input.read(buffer)) != -1) {
            outputStream.write(buffer, 0, bytesRead);
        }
        outputStream.close();
        input.close();

        response(newIn);

        synchronized (mSockets) {
            mSockets.remove(newSocket);
        }

        newSocket.close();
    }

    public void deleteFile(String filename) throws Exception {
        request(FTPCommand.DELE, out, filename);
        FTPResponse response = response(in);
        if (!(response.getCode() == 150 || response.getCode() == 226)) {
            return;
        }
    }

    public FTPFile[] nlist(String dir) throws IOException {
        Socket socket = openChannel(in, out);
        request(FTPCommand.NLST, out, dir == null ? "/" : dir);
        response(in);

        BufferedInputStream input = new BufferedInputStream(socket.getInputStream());

        byte[] buffer = new byte[1024];
        StringBuilder buf = new StringBuilder();
        while (input.read(buffer) > 0) {
            buf.append(new String(buffer));
        }
        input.close();

        response(in);

        String[] lines = buf.toString().trim().split("\r\n");
        FTPFile[] files = new FTPFile[lines.length];
        for (int i = 0; i < lines.length; i++) {
            files[i] = new FTPFile(lines[i]);
        }

        return files;
    }

    public void makeDir(String name) throws IOException {
        request(FTPCommand.MKD, out, name);
        response(in);
    }

    public void deleteDir(String name) throws IOException {
        request(FTPCommand.RMD, out, name);
        response(in);
    }

    private void login(BufferedReader in, BufferedWriter out) throws IOException {
        longResponse(in);
        request(FTPCommand.USER, out, user);
        response(in);
        request(FTPCommand.PASS, out, password);
        longResponse(in);
    }

    private void request(FTPCommand command, BufferedWriter out, String... args) throws IOException {
        StringBuilder request = new StringBuilder(command.toString());
        for (String arg : args) {
            request.append(' ').append(arg);
        }
        log(request.toString(), false);

        out.write(request.toString() + "\r\n");
        out.flush();
    }

    private FTPResponse response(BufferedReader in) throws IOException {
        String str = in.readLine();
        log(str, true);
        return new FTPResponse(str);
    }

    private void longResponse(BufferedReader in) throws IOException {
        FTPResponse response;
        do response = response(in);
        while (response.isDelimiter());
    }

    private void log(String message, boolean in) {
        System.out.println((in ? "<<< " : ">>> ") + message);
    }

    private Socket openChannel(BufferedReader in, BufferedWriter out) throws IOException {
        request(FTPCommand.SYST, out);
        response(in);
        request(FTPCommand.PWD, out);
        response(in);
        request(FTPCommand.TYPE, out, "I");
        response(in);

        request(FTPCommand.PASV, out);

        FTPResponse response = response(in);
        if(response.getCode() == 530) return null;

        String text = response.getText();
        text = text.substring(text.indexOf('(') + 1, text.indexOf(')'));
        String[] buf = text.split(",");

        String ip = buf[0] + "." + buf[1] + "." + buf[2] + "." + buf[3];
        int port = (Integer.parseInt(buf[4]) << 8) | Integer.parseInt(buf[5]);

        log(ip + ":" + port, true);

        return new Socket(host, port);
    }

    private Socket getTransmissionSocket(BufferedReader newIn, BufferedWriter newOut) throws IOException {
        login(newIn, newOut);
        return openChannel(newIn, newOut);
    }
}