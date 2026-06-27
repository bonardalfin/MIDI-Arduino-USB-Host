#include <usbh_midi.h>
#include <usbhub.h>
#include <Wire.h>
#include <hd44780.h>                       // Main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // I2C expander i/o class header

// Declare the LCD object
hd44780_I2Cexp lcd; 

// Initialize USB Host and USB MIDI
USB Usb;
USBH_MIDI MIDI(&Usb);

// Variables for MIDI CC logic and non-blocking timers
uint8_t ccValue = 1;
unsigned long lastMidiSendTime = 0; 
bool usbInitialized = false;
bool lastMidiState = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize LCD: 20 columns, 4 rows
  lcd.begin(20, 4); 
  lcd.backlight();
  
  // Display initial boot status
  lcd.setCursor(0, 0);
  lcd.print("USB HOST: Wait      ");
  lcd.setCursor(0, 1);
  lcd.print("Waiting for USB MIDI");
  lcd.setCursor(0, 2);
  lcd.print("device to be        ");
  lcd.setCursor(0, 3);
  lcd.print("plugged in.         ");

  // Initialize USB Host Shield
  if (Usb.Init() == -1) {
    Serial.println("USB Host Shield initialization failed!");
    lcd.setCursor(10, 0);
    lcd.print("Failed ");
    while (1); // Halt program execution if shield is missing/broken
  }
  
  usbInitialized = true;
  lcd.setCursor(10, 0);
  lcd.print("Success");
  Serial.println("USB Host Shield initialized successfully.");
}

void loop() {
  // Always poll the USB task to maintain stable connection
  Usb.Task();

  // Check current MIDI connection state
  bool currentMidiState = (bool)MIDI;

  // Handle LCD transition when device state changes
  if (currentMidiState != lastMidiState) {
    lcd.clear(); // Clear old text layout
    
    if (currentMidiState) {
      // Setup permanent static layout for connected state
      lcd.setCursor(0, 0);
      lcd.print("ARDUINO UNO USB HOST");
      lcd.setCursor(0, 1);
      lcd.print("MIDI Channel 1");
      lcd.setCursor(0, 2);
      lcd.print("Control Change #1");
      lcd.setCursor(0, 3);
      lcd.print("Value ");
    } else {
      // Revert back to waiting layout
      lcd.setCursor(0, 0);
      lcd.print("USB HOST: Success   ");
      lcd.setCursor(0, 1);
      lcd.print("Waiting for USB MIDI");
      lcd.setCursor(0, 2);
      lcd.print("device to be        ");
      lcd.setCursor(0, 3);
      lcd.print("plugged in.         ");
    }
    lastMidiState = currentMidiState;
  }

  // If MIDI device is connected, run the 1-second transmission interval
  if (currentMidiState) {
    if (millis() - lastMidiSendTime >= 1000) {
      lastMidiSendTime = millis();

      // MIDI configuration parameters (Channel 1 maps to index 0)
      uint8_t midiChannel = 0; 
      uint8_t ccNumber = 1;    
      
      // Send the MIDI Control Change packet
      sendMIDI_CC(midiChannel, ccNumber, ccValue);

      // Print debug data to Serial Monitor
      Serial.print("Sending MIDI CC#1 | Channel 1 | Value: ");
      Serial.println(ccValue);

      // Dynamically update the CC value on the 4th row of the LCD
      lcd.setCursor(6, 3);
      lcd.print(ccValue);
      lcd.print("  "); // Extra spaces to wipe leftover characters (e.g., when 10 turns back to 1)

      // Increment value and loop back from 1 to 10
      ccValue++;
      if (ccValue > 10) {
        ccValue = 1;
      }
    }
  }
}

// Helper function to build and transfer raw MIDI CC streams
void sendMIDI_CC(uint8_t chan, uint8_t control, uint8_t val) {
  uint8_t buf[3];
  buf[0] = 0xB0 | chan;   // 0xB0 status byte represents Control Change
  buf[1] = control;       // CC Number
  buf[2] = val;           // CC Value
  
  MIDI.SendData(buf);
}
