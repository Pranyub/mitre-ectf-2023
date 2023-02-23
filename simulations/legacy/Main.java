public class Main {
    public static void main(String[] args) {
        Device car = new Car(123456);
        Device unpairedFob = new Fob(111111);
        Device pairedFob = new Fob(car, 654321);

        // Insert any test code below, this is all very rudimentary

    }
}
