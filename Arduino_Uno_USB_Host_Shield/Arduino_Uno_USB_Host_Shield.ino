#include <usbh_midi.h>
#include <usbhub.h>
#include <Wire.h>
#include <hd44780.h>                       // Main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // I2C expander i/o class header
#include <Adafruit_NeoPixel.h>             // Library for WS2812B LEDs

// --- Hardware Configuration ---
#define LED_PIN     A1
#define NUM_LEDS    8

// Declare peripheral objects
hd44780_I2Cexp lcd; 
USB Usb;
USBH_MIDI MIDI(&Usb);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Pin allocation for the 8 buttons
const uint8_t buttonPins[8] = {2, 3, 4, 5, 6, 7, 8, A0};

// Names of the buttons assigned to each slot
const char* buttonNames[8] = {
  "PKET", "SWET", "CHILL", "SOUL", // Buttons 1 to 4 (Bottom Row)
  "METL", "BLUS", "ROCK",  "UKIN"  // Buttons 5 to 8 (Top Row)
};

// --- State Variables & Timers ---
bool lastMidiState = false;
bool btnState[8] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
bool lastBtnState[8] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};

// Timers for non-blocking operations (LED duration and debounce)
unsigned long ledOffTime[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned long lastDebounceTime[8] = {0, 0, 0, 0, 0, 0, 0, 0};
const unsigned long debounceDelay = 50; // 50ms debounce time

void setup() {
  Serial.begin(115200);
  
  // Initialize LCD
  lcd.begin(20, 4); 
  lcd.backlight();
  
  // Initialize WS2812B Addressable LEDs
  strip.begin();
  strip.setBrightness(100); // Adjust brightness (0-255) to manage power consumption
  strip.show();             // Turn off all LEDs initially

  // Initialize all button pins with internal pull-up resistors
  for (int i = 0; i < 8; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  // Display initial connection status on boot
  lcd.setCursor(0, 0); lcd.print("USB HOST: Init...   ");
  lcd.setCursor(0, 1); lcd.print("Waiting for device  ");

  if (Usb.Init() == -1) {
    lcd.setCursor(10, 0); lcd.print("Failed  ");
    while (1); // Halt execution if the host shield fails to respond
  }
  
  lcd.setCursor(10, 0); lcd.print("Success ");
  delay(1000); // Brief pause to show initialization status
  lcd.clear();
}

void loop() {
  // Always poll the USB Host Shield task handler
  Usb.Task();

  bool currentMidiState = (bool)MIDI;

  // Handle LCD display transition based on MIDI device availability
  if (currentMidiState != lastMidiState) {
    lcd.clear();
    if (currentMidiState) {
      drawStaticLayout();
    } else {
      lcd.setCursor(0, 0); lcd.print("USB HOST: Disconn  ");
      lcd.setCursor(0, 1); lcd.print("Connect MIDI Device ");
    }
    lastMidiState = currentMidiState;
  }

  // Process inputs and lighting only if the MIDI engine is running
  if (currentMidiState) {
    handleButtons();
    handleLedTimers();
  }
}

// Draw the required layout on the screen once connected
void drawStaticLayout() {
  // Top row: Buttons 5 to 8 layout mapping
  lcd.setCursor(0, 0);  lcd.print("METL BLUS  ROCK UKIN");
  
  // Second row: Bank indicator
  lcd.setCursor(0, 1);  lcd.print("BANK 1");
  
  // Third row: Placeholder for user interactive actions
  lcd.setCursor(0, 2);  lcd.print("Press button...");
  
  // Bottom row: Buttons 1 to 4 layout mapping
  lcd.setCursor(0, 3);  lcd.print("PKET SWET  CHIL SOUL");
}

// Non-blocking button scanning routine with hardware debounce processing
void handleButtons() {
  for (int i = 0; i < 8; i++) {
    int reading = digitalRead(buttonPins[i]);

    if (reading != lastBtnState[i]) {
      lastDebounceTime[i] = millis();
    }

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (reading != btnState[i]) {
        btnState[i] = reading;

        // Button action triggered on transition to LOW (pressed down)
        if (btnState[i] == LOW) {
          uint8_t midiChannel = 0; // MIDI Channel 1 (index 0)
          uint8_t ccNumber = 1;    // CC#1
          uint8_t ccValue = i + 1; // Maps index 0-7 to CC value 1-8

          // 1. Transmit MIDI Control Change Packet
          sendMIDI_CC(midiChannel, ccNumber, ccValue);

          // 2. Trigger corresponding NeoPixel LED to turn solid White
          strip.setPixelColor(i, strip.Color(255, 255, 255)); // White color parameters
          strip.show();
          ledOffTime[i] = millis() + 1000; // Set timestamp to turn off in 1000ms (1 seconds)

          // 3. Dynamic LCD text layout updates
          lcd.setCursor(0, 2);
          lcd.print("Press: ");
          lcd.print(buttonNames[i]);
          lcd.print("        "); // Blank trailing padding spaces to clear previous text artifacts
          
          // Debug mirroring to the PC environment
          Serial.print("Button "); Serial.print(i + 1);
          Serial.print(" Pressed. Sent CC#1 Value: "); Serial.println(ccValue);
        }
      }
    }
    lastBtnState[i] = reading;
  }
}

// Evaluates ongoing tracking variables for individual LED timers
void handleLedTimers() {
  bool stripChanged = false;
  
  for (int i = 0; i < 8; i++) {
    // Check if an LED is currently active and has exceeded its 2-second window
    if (ledOffTime[i] != 0 && millis() >= ledOffTime[i]) {
      strip.setPixelColor(i, strip.Color(0, 0, 0)); // Turn LED Off
      ledOffTime[i] = 0;                            // Reset active timer tracking register
      stripChanged = true;
    }
  }
  
  // Push batch state configuration to the LEDs simultaneously if changes occurred
  if (stripChanged) {
    strip.show();
  }
}

// Low-level helper routine processing raw serial arrays for USB stream output
void sendMIDI_CC(uint8_t chan, uint8_t control, uint8_t val) {
  uint8_t buf[3];
  buf[0] = 0xB0 | chan; // Status indicator for CC commands
  buf[1] = control;     // Targeted CC Assignment ID
  buf[2] = val;         // Configured value parameters
  
  MIDI.SendData(buf);
}
