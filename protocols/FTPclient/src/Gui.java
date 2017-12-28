import ftp.FTPClient;
import ftp.FTPFile;

import javax.swing.*;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.IOException;
import java.net.SocketException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Gui extends JFrame {
    private JButton stopButton, startButton, uploadButton, mkdirButton;
    private JTextField hostField, portField, loginField, passwordField, pathField;
    private FTPClient ftp;
    private String dir;

    public Gui(){
        super("FTP Passive Mode Client");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        dir = "/";

        JPanel mainPanel = new JPanel();
        mainPanel.setLayout(new BorderLayout(5, 5));
        mainPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));

        JPanel connectPanel = new JPanel();
        connectPanel.setLayout(new GridLayout(3, 4, 5, 5));
        mainPanel.add(connectPanel, BorderLayout.NORTH);

        JPanel fieldPanel = new JPanel();
        fieldPanel.setLayout(new GridLayout(1, 4, 5, 0));
        connectPanel.add(fieldPanel, BorderLayout.NORTH);

        hostField = new JTextField("ftp.neva.ru");
//        hostField = new JTextField("ftp.maths.tcd.ie");
        fieldPanel.add(hostField);
        portField = new JTextField("21");
        fieldPanel.add(portField);
        loginField = new JTextField("anonymous");
        fieldPanel.add(loginField);
        passwordField = new JTextField("");
        fieldPanel.add(passwordField);

        JPanel buttonsPanel1 = new JPanel();
        buttonsPanel1.setLayout(new GridLayout(1, 2, 5, 0));
        connectPanel.add(buttonsPanel1, BorderLayout.CENTER);

        JPanel pathPanel = new JPanel();
        pathPanel.setLayout(new GridLayout(1, 1, 5, 0));
        connectPanel.add(pathPanel, BorderLayout.SOUTH);

        pathField = new JTextField("/");
        pathField.setEditable(false);
        pathPanel.add(pathField);

        JPanel buttonsPanel2 = new JPanel();
        buttonsPanel2.setLayout(new GridLayout(1, 2, 5, 0));
        mainPanel.add(buttonsPanel2, BorderLayout.SOUTH);

        final DefaultListModel listModel = new DefaultListModel();

        final JList listField = new JList(listModel);
        listField.setFocusable(false);
        listField.setEnabled(false);
        listField.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        listField.setFont(new Font("monospaced", Font.PLAIN, 12));
        mainPanel.add(new JScrollPane(listField), BorderLayout.CENTER);

        listField.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent e) {
                JList tempList = (JList) e.getSource();
                if (SwingUtilities.isRightMouseButton(e)) {
                    tempList.setSelectedIndex(tempList.locationToIndex(e.getPoint()));
                    String value = tempList.getSelectedValue().toString();
                    String type = value.substring(0, 1);
                    String name = getFileFolderName(value, type);

                    JPopupMenu menu = new JPopupMenu();
                    JMenuItem itemRemove = new JMenuItem("Delete");
                    if(type.equals("f")){
                        JMenuItem itemDownload = new JMenuItem("Download");
                        itemDownload.addActionListener(e1 -> {
                            JFileChooser downloadFolder = new JFileChooser();
                            downloadFolder.setCurrentDirectory(new java.io.File("."));
                            downloadFolder.setDialogTitle("Download file");
                            downloadFolder.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
                            downloadFolder.setAcceptAllFileFilterUsed(false);

                            if (downloadFolder.showDialog(null, "Select folder") == JFileChooser.APPROVE_OPTION) {
                                String path = downloadFolder.getSelectedFile() + "\\" + name;

                                Thread t = new Thread(() -> {
                                    try {
                                        ftp.getFile(name, path);
                                    } catch (SocketException e2){
                                        System.out.println("File " + name + " not fully downloaded!");
                                    } catch (Exception e2) {
                                        e2.printStackTrace();
                                    }
                                });

                                t.start();
                            }
                        });
                        menu.add(itemDownload);
                    }
                    itemRemove.addActionListener(e12 -> {
                        int confirm = JOptionPane.showConfirmDialog(null, "Are you sure to delete " + name + "?", "Delete file/folder", JOptionPane.OK_CANCEL_OPTION);
                        if(confirm == JOptionPane.OK_OPTION){
                            if(type.equals("f")){
                                try {
                                    ftp.deleteFile(name);
                                    fillList(listModel);
                                } catch (Exception e1) {
                                    e1.printStackTrace();
                                }
                            }
                            else{
                                try {
                                    ftp.deleteDir(name);
                                    fillList(listModel);
                                } catch (IOException e1) {
                                    e1.printStackTrace();
                                }
                            }
                        }
                    });
                    menu.add(itemRemove);
                    menu.show(tempList, e.getPoint().x, e.getPoint().y);
                } else {
                    try {
                        String value = tempList.getSelectedValue().toString();
                        String type = value.substring(0, 1);
                        if (e.getClickCount() == 2 && !type.equals("f")) {
                            String folderName = getFileFolderName(value, type);

                            try {
                                dir = ftp.cwd(folderName);
                                pathField.setText(dir);
                                fillList(listModel);
                            } catch (IOException e1) {
                                System.out.print("Connection failed!\n");
                            }
                        }
                    } catch (NullPointerException e1) {
                        System.out.print("Empty click on list\n");
                    }
                }
            }
        });

        startButton = new JButton("Connect");
        startButton.setFocusable(false);
        startButton.addActionListener(e -> {

            try {
                ftp = new FTPClient(hostField.getText(), Integer.parseInt(portField.getText()), loginField.getText(), passwordField.getText());
                ftp.connect();
                fillList(listModel);
                startButton.setEnabled(false);
                stopButton.setEnabled(true);
                uploadButton.setEnabled(true);
                mkdirButton.setEnabled(true);
            } catch (IOException | NullPointerException | NumberFormatException e1) {
                System.out.print("Connection failed!\n");
            }
            listField.setEnabled(true);
        });
        buttonsPanel1.add(startButton);

        stopButton = new JButton("Stop");
        stopButton.setFocusable(false);
        stopButton.addActionListener(e -> {
            ftp.close();
            stopButton.setEnabled(false);
            startButton.setEnabled(true);
            uploadButton.setEnabled(false);
            mkdirButton.setEnabled(false);
            listField.setEnabled(false);
            listModel.removeAllElements();
            dir = "/";
            pathField.setText(dir);
        });
        stopButton.setEnabled(false);
        buttonsPanel1.add(stopButton);

        uploadButton = new JButton("Upload file");
        uploadButton.setFocusable(false);
        uploadButton.addActionListener(e -> {
            JFileChooser fileopen = new JFileChooser();
            int ret = fileopen.showDialog(null, "Upload file");
            if(ret == JFileChooser.APPROVE_OPTION){
                String filepath = fileopen.getSelectedFile().getAbsolutePath();

                Thread t = new Thread(() ->{
                    try {
                        ftp.uploadFile(filepath, dir);
                        fillList(listModel);
                    } catch (SocketException e1){
                        System.out.println("File " + filepath + " not fully uploaded!");
                    } catch (Exception e1){
                        e1.printStackTrace();
                    }
                });

                t.start();
            }
        });
        uploadButton.setEnabled(false);
        buttonsPanel2.add(uploadButton);

        mkdirButton = new JButton("Create folder");
        mkdirButton.setFocusable(false);
        mkdirButton.addActionListener(e -> {
            try {
                String name = JOptionPane.showInputDialog(null, "Enter new folder name:", "Create folder", JOptionPane.QUESTION_MESSAGE);
                if(name != null && !name.isEmpty()){
                    ftp.makeDir(name);
                    fillList(listModel);
                }
            } catch (IOException e1) {
                e1.printStackTrace();
            }
        });
        mkdirButton.setEnabled(false);
        buttonsPanel2.add(mkdirButton);

        getContentPane().add(mainPanel);
        setPreferredSize(new Dimension(600, 500));
        setResizable(false);
        pack();
        setLocationRelativeTo(null);
        setVisible(true);
    }

    private String getFileFolderName(String value, String type) {
        String folderName = "";
        Pattern regex;
        if (type.equals("l"))
            regex = Pattern.compile("(?:.){4}(.+)\\s+->\\s+.+");
        else regex = Pattern.compile("(?:.){4}(.+)");
        Matcher matcher = regex.matcher(value);
        if (matcher.find()) folderName = matcher.group(1);
        return folderName;
    }

    private void fillList(DefaultListModel listModel) throws IOException {
        listModel.removeAllElements();
        FTPFile[] list = ftp.list(dir);
        if (!dir.equals("/")) listModel.addElement("d | ..");
        for (FTPFile file : list) {
            if(file.getType().equals("") || file.getName().equals("")) continue;
            String entry = file.getType() + " | " + file.getName();
//            if(file.getType().equals("f")) entry += " (" + file.getSize() + " Bytes)";
//            System.out.print(file.toString());
//                    System.out.print(entry);
            listModel.addElement(entry);
        }
    }

    public static void main(String args[]){
        JFrame.setDefaultLookAndFeelDecorated(true);
        new Gui();
    }
}
