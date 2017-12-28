package ftp;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class FTPFile {
    private String name = "";
    private String type = "";
    private long size;

    public FTPFile(String line) {
        Pattern regex = Pattern.compile("(.).{9}(?:\\s+.+){3}\\s+(\\d+)\\s+\\w{3}\\s+\\d{1,2}\\s+[\\d:]{4,5}\\s+(.*)");
        Matcher matcher = regex.matcher(line);
        if (matcher.find()) {
            this.type = matcher.group(1);
            if(this.type.equals("-")) this.type = "f";
            this.size = Long.parseLong(matcher.group(2));
            this.name = matcher.group(3);
        }
    }

    public String toString(){
        return "type = " + type + "\nsize = " + size + "\nname = " + name + "\n\n";
    }

    public long getSize() {
        return size;
    }

    public String getName() {
        return name;
    }

    public String getType() {
        return type;
    }
}