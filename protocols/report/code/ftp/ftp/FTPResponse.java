package ftp;

public class FTPResponse {
    private final int code;
    private final String text;
    private final String delimiter;

    public FTPResponse(String answer) {
        int number;
        int pos;
        try {
            String num = answer.trim().split("(\\s|-)", 2)[0];
            number = Integer.parseInt(num);
            pos = String.valueOf(number).length();
        }catch (IndexOutOfBoundsException | NumberFormatException e){
            number = 0;
            pos = 0;
        }
        this.code = number;

        this.delimiter = Character.toString(answer.charAt(pos));

        this.text = answer.substring(pos + 1).trim();
    }

    public int getCode() {
        return code;
    }
    public String getText() {
        return text;
    }
    public boolean isDelimiter() {
        return delimiter.equals("-");
    }

}