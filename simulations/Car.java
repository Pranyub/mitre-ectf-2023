import java.util.*;

public class Car extends Device {
    private boolean unlocked;
    private int pin;
    private List<String> features; // A list of the features.

    // Creates a new car
    public Car(int pin) {
        unlocked = false;
        this.pin = pin;
        features = new ArrayList<>();
    }

    public void recievedRequest(Device sender, String type) {
        String[] request = type.split("|");
        switch (request[0]) {
            case "Feature":
                // I dont know for now, but just add the feature i guess.
                features.add(request[1]);
                break;
            case "Unlock":
                sendChallenge(sender);
                break;
        }

    }

    // Sends challenge to the FOB.
    public void sendChallenge(Device other) {
        String challenge = "Haha funny challenge:[Insert One time pin here]";
        String resp = other.solveChallenge(challenge);
        if (verifyChallenge(resp)) {
            System.out.println("Car is unlocked!");
            unlocked = true;
        }
    }

    // Debug code for verifying a feature actually is on the car.
    public boolean verifyFeature(int feature) {
        return features.contains(feature);
    }

    // Debug code for if the car is unlocked.
    public boolean unlocked() {
        return unlocked;
    }

    // Gets the car ID
    public int getPin() {
        return pin;
    }
}
