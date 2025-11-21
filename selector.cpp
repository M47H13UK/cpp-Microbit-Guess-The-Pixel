#include "MicroBit.h"

MicroBit uBit; // The MicroBit object

// Global variables to store cursor position and state
int cursorX = 0; // Cursor X position
int cursorY = 0; // Cursor Y position
int selectedX = -1; // Selected X position (-1 indicates no selection yet)
int selectedY = -1; // Selected Y position (-1 indicates no selection yet)
bool isFinalized = false; // Indicates if the selection is finalized
bool isActive = true; // Device state: active for selector, passive for guesser

// Function to update the display based on the cursor and selection
void updateDisplay() {
    uBit.display.clear();
    if (selectedX != -1 && selectedY != -1) {
        if (cursorX == selectedX && cursorY == selectedY) {
            // Highlight the selected pixel with full brightness if the cursor is on top of it
            uBit.display.image.setPixelValue(selectedX, selectedY, 255);
        } else {
            // Highlight the selected pixel with medium brightness
            uBit.display.image.setPixelValue(selectedX, selectedY, 70);
        }
    }
    // Highlight the cursor position with full brightness.
    uBit.display.image.setPixelValue(cursorX, cursorY, 255);
}

// Event handler for button A (move cursor up)
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

// Event handler for button B (move cursor right)
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

// Event handler for buttons A and B pressed together (select/finalize pixel)
void onButtonAB(MicroBitEvent e) {
    if (!isActive) return; // Ignore if in passive state

    if (isFinalized && cursorX == selectedX && cursorY == selectedY) {
        // Finalize the selection and transmit the coordinates.
        uint8_t msg[3];
        msg[0] = 0;  // 0 for new selected pixel
        msg[1] = selectedX;
        msg[2] = selectedY;
        PacketBuffer pb(msg, 3);
        uBit.radio.datagram.send(pb);
        uBit.display.print("T"); // Indicate transmission
        uBit.audio.soundExpressions.play("happy");
        uBit.sleep(1500); // Show "T" for 1.5 seconds
        uBit.display.clear();
        // Show the selected pixel at medium brightness
        uBit.display.image.setPixelValue(selectedX, selectedY, 70);
        isActive = false;
    } else {
        // Select the pixel under the cursor
        selectedX = cursorX;
        selectedY = cursorY;
        isFinalized = true; // Mark the selection as finalized
        updateDisplay();
    }
}

// Event handler for wireless data reception
void onData(MicroBitEvent e) {
    PacketBuffer buf = uBit.radio.datagram.recv();
    uint8_t *data = buf.getBytes();
    if (data[0] == 1) {
        // 1 for guesser finished playing
        cursorX = 0;
        cursorY = 0;
        selectedX = -1;
        selectedY = -1;
        isFinalized = false;
        isActive = true; // Reset to active state for new selection
        uBit.display.print("R"); // Indicate reset
        uBit.sleep(1000);
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

    // Display initial state
    updateDisplay();

    // Enter the scheduler indefinitely
    release_fiber();
}
