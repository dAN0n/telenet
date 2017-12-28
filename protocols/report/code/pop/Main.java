public class Main {
    public static void main(String[] args) {
        MainThread st=new MainThread();
        new Thread(st).start();
    }
}
