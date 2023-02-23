public class Fob extends Device {
    private int pin; // This is our pin!
    private int pairedPin; // This is to verify the sender is actually who we are paired with.
    private Device pairedCar; // This is to remember who we are paired with.

    // Creates a paired fob.
    public Fob(Device car, int pin) {
        this.pin = pin;
        pairedCar = car;
    }

    // Creates an unpaired fob.
    public Fob(int pin) {
        this.pin = pin;
    }

    public void recievedRequest(Device sender, String type) {
        // Break this up, since different requests will be different lengths.
        String[] args = type.split(" ");
        switch (args[0]) {
            case "Feature":
                // If the case is a feature request, then verify if the feature is for the car
                if (args[1].equals(pairedPin)) {
                    // It is, we can send a request to the paired car.
                    pairedCar.recievedRequest(this, "Feature " + args[2]);
                }
                break;
            case "Unlock":
                // Unlock request, send the unlock request to the car.
                if (pairedCar == null) {
                    throw new IllegalStateException("I'm not paired!");
                } else {
                    sender.recievedRequest(this, "Unlock");
                }
                break;
            case "Pair":
                // Pair request, make sure we're paired to a car, then initiate pairing to fob
                if (pairedCar == null) {
                    throw new IllegalStateException("I'm not paired!");
                } else {
                    sendChallenge(sender);
                }
                break;
            default:
                throw new IllegalArgumentException("None of the possible cases match");
        }
    }

    // In this case, the challenge will just be "Haha funny challenge"
    public void sendChallenge(Device other) {
        String challenge = "Haha funny challenge:[Insert One time pin here]";
        String resp = other.solveChallenge(challenge);
        if (verifyChallenge(resp)) {
            // Only time we send a challenge is when unpaired key wants to pair w/ us
            ((Fob) other).pair(pairedCar);
        }
    }

    /*
     * Pre: We haven't been paired
     * Initiates a pair request to the other device.
     */
    public void initPairing(Device other) {
        if (pairedCar != null) {
            throw new IllegalStateException("I'm already paired!");
        }
        String request = "Pair";
        other.recievedRequest(this, request);
    }

    // Pairs the two devices, no questions asked
    public void pair(Device other) {
        if (pairedCar != null) {
            throw new IllegalStateException("I'm already paired!");
        }
        this.pairedPin = other.getPin();
        pairedCar = other;
    }

    // Debug method for pin.
    public int getPin() {
        return pin;
    }

    // Debug method for if it's paired
    public String isPaired() {
        if (pairedCar != null) {
            return "True! my car is: " + pairedCar.getPin();
        }
        return "False :(";
    }

}
