    #include "MicroBit.h"

MicroBit uBit; // The MicroBit object

// Global variables to store cursor position, state, and guessed pixels
int cursorX = 0; // Cursor X position
int cursorY = 0; // Cursor Y position
int receivedX = -1; // Received X position of the selected pixel
int receivedY = -1; // Received Y position of the selected pixel
bool isActive = false; // Device state: active for guesser, passive for selector
const int maxAttempts = 8; // Maximum allowed attempts
int guessedPixels[25][2]; // Store the X, Y coordinates of guessed pixels
int guessedCount = 0; // Number of guessed positions stored

// Function to initialize the guessedPixels array
void initializeGuessedPixels() {
    for (int i = 0; i < 25; i++) {
        guessedPixels[i][0] = -1;
        guessedPixels[i][1] = -1;
    }
}

// Function to update the display based on the cursor and guessed pixels.
void updateDisplay() {
    uBit.display.clear();

    // Highlight all previously guessed pixels with medium brightness
    for (int i = 0; i < guessedCount; i++) {
        if (guessedPixels[i][0] != -1 && guessedPixels[i][1] != -1) {
            uBit.display.image.setPixelValue(guessedPixels[i][0], guessedPixels[i][1], 70);
        }
    }

    // Highlight the cursor position with full brightness
    uBit.display.image.setPixelValue(cursorX, cursorY, 255);
}

// Function to reset the game state
void resetGame() {
    cursorX = 0;
    cursorY = 0;
    receivedX = -1;
    receivedY = -1;
    guessedCount = 0; // Clear guessed pixels
    initializeGuessedPixels(); // Reset guessedPixels array
    isActive = false;
    updateDisplay();
}

// Event handler for button A (move right)
void onButtonA(MicroBitEvent e) {
    if (!isActive) return; // Ignore if in passive state

    if (cursorX > 0) {
        cursorX -= 1;
    } else {
        cursorX = 4;
        cursorY = (cursorY > 0) ? cursorY - 1 : 4; // Increment row or wrap around to the top row
    }
    updateDisplay();
}

// Event handler for button B (move left)
void onButtonB(MicroBitEvent e) {
    if (!isActive) return; // Ignore if in passive state

    if (cursorX < 4) {
        cursorX += 1;
    } else {
        cursorX = 0;
        cursorY = (cursorY < 4) ? cursorY + 1 : 0; // Increment row or wrap around to the top row
    }
    updateDisplay();
}

// Event handler for buttons A and B pressed together (make a guess)
void onButtonAB(MicroBitEvent e) {
    if (!isActive) return; // Ignore if in passive state

    if (cursorX == receivedX && cursorY == receivedY) {
        // Correct guess
        uBit.display.clear();
        uBit.display.print(MicroBitImage("0,9,0,9,0\n \
                                          0,9,0,9,0\n \
                                          0,0,0,0,0\n \
                                          9,0,0,0,9\n \
                                          0,9,9,9,0\n"));
        uBit.audio.soundExpressions.play("happy");
        uBit.sleep(1500); // Show the happy face for 1.5 seconds

        // Send message to selector indicating turn is over
        uint8_t msg[3];
        msg[0] = 1; // 1 to indicate turn is over
        msg[1] = 0;
        msg[2] = 0;
        PacketBuffer pb(msg, 3);
        uBit.radio.datagram.send(pb);

        initializeGuessedPixels();
        resetGame(); // Restart the game
        
        // Display "W" to indicate waiting for data
        uBit.display.print("W");
    } else {
        // Incorrect guess

        // Store the guessed position if not already stored
        bool alreadyGuessed = false;
        for (int i = 0; i < guessedCount; i++) {
            if (guessedPixels[i][0] == cursorX && guessedPixels[i][1] == cursorY) {
                alreadyGuessed = true;
                break;
            }
        }
        if (!alreadyGuessed && guessedCount < 25) {
            guessedPixels[guessedCount][0] = cursorX;
            guessedPixels[guessedCount][1] = cursorY;
            guessedCount++;
        }

        if (guessedCount >= maxAttempts) {
            // Max attempts reached, display sad face
            uBit.display.clear();
            uBit.display.print(MicroBitImage("0,9,0,9,0\n \
                                              0,9,0,9,0\n \
                                              0,0,0,0,0\n \
                                              0,9,9,9,0\n \
                                              9,0,0,0,9\n"));
            uBit.audio.soundExpressions.play("sad");
            uBit.sleep(1500); // Show the sad face for 1.5 seconds

            // Send message to selector indicating turn is over
            uint8_t msg[3];
            msg[0] = 1; // 1 ot indicate turn is over
            msg[1] = 0;
            msg[2] = 0;
            PacketBuffer pb(msg, 3);
            uBit.radio.datagram.send(pb);

            initializeGuessedPixels();
            resetGame(); // Restart the game
            // Display "W" to indicate waiting for data
            uBit.display.print("W");
        } else {
            updateDisplay();
        }
    }
}

// Event handler for wireless data reception
void onData(MicroBitEvent e) {
    PacketBuffer buf = uBit.radio.datagram.recv();
    if (buf.length() != 3) {
        uBit.display.print("E"); // Display error if message length is not 3
        return;
    }

    uBit.display.print("T");
    uBit.sleep(1500);
    uint8_t *data = buf.getBytes();
    if (data[0] == 0) {
        // 0 for new selected pixel
        receivedX = data[1];
        receivedY = data[2];
        isActive = true; // Switch to active state
        updateDisplay();
    }
}

int main() {
    // Initialise the micro:bit
    uBit.init();

    // Ensure different levels of brightness can be displayed
    uBit.display.setDisplayMode(DISPLAY_MODE_GREYSCALE);

    // Set up a listener for the radio
    uBit.radio.enable();
    uBit.radio.setGroup(42); // Set the radio channel to 42
    uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, onData);

    // Set up listeners for buttons A, B, and AB
    uBit.messageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_BUTTON_EVT_CLICK, onButtonA);
    uBit.messageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_CLICK, onButtonB);
    uBit.messageBus.listen(MICROBIT_ID_BUTTON_AB, MICROBIT_BUTTON_EVT_CLICK, onButtonAB);

    // Initialize guessed pixels array
    initializeGuessedPixels();

    // Display initial state
    resetGame();

    // Display "W" to indicate waiting for data
    uBit.display.print("W");

    // Enter the scheduler indefinitely
    release_fiber();
}
