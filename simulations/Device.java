public abstract class Device {
    abstract int getPin(); // Gets the pin of the current device.

    abstract void recievedRequest(Device sender, String type); // Does something based on the request type

    abstract void sendChallenge(Device other); // Sends a type of challenge to other devices

    // Given a challenge, solves it.
    public String solveChallenge(String challenge) {
        return "Solution to challenge: Some function of recieved one time pin and our pin which is " + getPin();
    }

    // Verifies the challenge is correct.
    public boolean verifyChallenge(String solution) {
        return (solution.indexOf("Solution to challenge:") != -1);
    }
}
