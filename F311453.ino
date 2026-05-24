/*
 * 23COA202 Embedded Systems Programming Coursework
 * "Parking Management System"
 * By F311453
 * 
 * ALL WORK IN THIS FILE WAS WRITTEN BY MYSELF, WITH NO HELP FROM OTHERS
 *
 */

// Library imports to get required functionality (Commented includes are unused allowed librarys)
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <EEPROM.h>
//#include <avr/eeprom.h>
//#include <TimeLib.h>
//#include <TimerOne.h>
#include <MemoryFree.h>


// Defining the backlight colors for the LCD for ease of use
#define blOff 0
#define blRed 1
#define blGreen 2
#define blYellow 3
#define blBlue 4
#define blPurple 5
#define blCyan 6
#define blWhite 7

// Defining states for the "hold select for >1s" FSM
#define selectStateTrue 1
#define selectStateFalse 0

// Defining states for the HCI FSM
#define leftHCI 1
#define centreHCI 2
#define rightHCI 3

// Needed in global scope
static int HCI_STATE = centreHCI;  // variable to control the HCI FSM

// Initialise the LCD display
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// Initialise msg string to recieve serial inputs -- ONLY "String" USED
String msg = "";  // Using a char array on the variable input proves rather difficult, but at the very least this string has a known size of max 65 bytes.


// Struct to hold the data related to each vehicle. Can't use classes as EEPROM wont store an object, but will store a struct.
struct vehicle {
  char regLoc[19];
  unsigned int typeEntry;
  unsigned int PaidExit;
};

unsigned long timeNow;  // Initilise in global scope

// Array that will hold all data associated to any of the vehicles that are instantiated during the course of the program
vehicle vehicles[44] = {
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
  { "", 0, 0 },
};

// Arrays and other variables to keep track of which vehicles are NPD or PD
int npdVehicles[44];
int pdVehicles[44];
int npdMax = 0;
int pdMax = 0;
int npdIDX;
int pdIDX;

// Boolean to show if vehicle data is being shown (for SCROLL)
bool displayData;

// Null vehicle used to reset data points
vehicle nullVehicle = { "", 0, 0 };

// Value to keep track of the number of existing vehicles, obtained when retrieving data from EEPROM
int vehicleMax;

// Value to keep track of the current display index, aka whats being displayed on screen
int displayIDX = 0;

// Custom Characters for up/down arrow (UDCHARS)
byte upArrow[8] = {
  0b00000,
  0b00100,
  0b01110,
  0b11111,
  0b00100,
  0b00100,
  0b00100,
  0b00000
};

byte downArrow[8] = {
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

// Update Display data:
void updateDisplay() {
  // Clear display prior to any updates
  lcd.clear();
  // If theres no data show nothing
  if (vehicleMax == 0) {
    lcd.setBacklight(blWhite);
    displayIDX = 0; // Reset display index
    lcd.print(F("    NO DATA"));
    lcd.setCursor(0, 1);
    lcd.print(F("    TO SHOW"));
    displayData = false; // Stop Scroll from attempting to work
    return;
  }

  // If theres no data to show in the left HCI state
  if (HCI_STATE == leftHCI && npdMax == 0) {
    lcd.setBacklight(blYellow);
    lcd.print(F("NO NPD VEHICLES"));
    displayData = false; // Stop Scroll from attempting to work
    return;
  }

  // If theres no data to show in the right HCI state
  if (HCI_STATE == rightHCI && pdMax == 0) {
    lcd.setBacklight(blGreen);
    lcd.print(F("NO PD VEHICLES"));
    displayData = false; // Stop Scroll from attempting to work
    return;
  }

  // If the function reaches this point data will be shown
  displayData = true; // Tell scroll to work on the data now

  // Set Arrows
  // If only one data point
  if (vehicleMax == 1) {
    // NO CURSOR
  }
  // Rest is dependent on what HCI you are in
  // If you are in the normal UI mode
  else if (HCI_STATE == centreHCI) {
    if (displayIDX == 0) {  // If at top of list
      lcd.setCursor(0, 1);
      lcd.write((uint8_t)1); // Up Arrow
    } else if (displayIDX == vehicleMax - 1) {  // If at bottom of list
      lcd.setCursor(0, 0);
      lcd.write((uint8_t)0); // Down Arrow
    } else {
      lcd.setCursor(0, 0);
      lcd.write((uint8_t)0); // Down Arrow
      lcd.setCursor(0, 1);
      lcd.write((uint8_t)1); // Up Arrow
    }
  } else if (HCI_STATE == leftHCI) { // If your in the display only NPD vehicles mode
    if (npdMax == 1) {
      // NO CURSOR
    } else if (displayIDX == npdVehicles[0]) {  // If at top of list
      lcd.setCursor(0, 1);
      lcd.write((uint8_t)1);
    } else if (displayIDX == npdVehicles[npdMax - 1]) {  // If at bottom of list
      lcd.setCursor(0, 0);
      lcd.write((uint8_t)0);
    } else {
      lcd.setCursor(0, 0);
      lcd.write((uint8_t)0);
      lcd.setCursor(0, 1);
      lcd.write((uint8_t)1);
    }
  } else if (HCI_STATE == rightHCI) { // if your in the display only PD vehicles mode
    if (pdMax == 1) {
      // NO CURSOR
    } else if (displayIDX == pdVehicles[0]) {  // If at top of list
      lcd.setCursor(0, 1);
      lcd.write((uint8_t)1);
    } else if (displayIDX == pdVehicles[pdMax - 1]) {  // If at bottom of list
      lcd.setCursor(0, 0);
      lcd.write((uint8_t)0);
    } else {
      lcd.setCursor(0, 0);
      lcd.write((uint8_t)0);
      lcd.setCursor(0, 1);
      lcd.write((uint8_t)1);
    }
  }
  // Display Reg
  lcd.setCursor(1, 0);
  for (int x = 0; x < 7; x++) {
    lcd.print(vehicles[displayIDX].regLoc[x]); // Print each reg character sequentially
  }
  // Display type
  int vtype;
  lcd.setCursor(1, 1);
  // depending on the value of typeEntry determine the character that needs to be put out
  // also get vtype var for getting entry time out easisly
  if (vehicles[displayIDX].typeEntry >= 50000) {
    lcd.print(F("B"));
    vtype = 50000;
  } else if (vehicles[displayIDX].typeEntry >= 40000) {
    lcd.print(F("L"));
    vtype = 40000;
  } else if (vehicles[displayIDX].typeEntry >= 30000) {
    lcd.print(F("V"));
    vtype = 30000;
  } else if (vehicles[displayIDX].typeEntry >= 20000) {
    lcd.print(F("M"));
    vtype = 20000;
  } else if (vehicles[displayIDX].typeEntry >= 10000) {
    lcd.print(F("C"));
    vtype = 10000;
  }

  // Display Entry time
  lcd.setCursor(7, 1);
  // If the data is mins only add 2 leading zeros
  if (vehicles[displayIDX].typeEntry - vtype < 100) {
    lcd.print(F("00"));
    // If data is less than 10hrs add 1 leading zero
  } else if (vehicles[displayIDX].typeEntry - vtype < 1000) {
    lcd.print(F("0"));
  }
  lcd.print(vehicles[displayIDX].typeEntry - vtype);


  // Display Payment
  lcd.setCursor(3, 1);
  // If vehicle has payed
  if (vehicles[displayIDX].PaidExit > 10000) {
    lcd.setBacklight(blGreen);
    lcd.print(F(" PD"));
    // Also display exit time
    char exitTime[6];
    itoa(vehicles[displayIDX].PaidExit, exitTime, 10);
    lcd.setCursor(12, 1);
    for (int x = 1; x < 5; x++) {
      lcd.print(exitTime[x]);
    }

  } else {  // IF NPD
    lcd.setBacklight(blYellow);
    lcd.print(F("NPD"));
  }

  // Display location
  lcd.setCursor(9, 0);
  // Go through the location string until hit the null terminator
  for (int x = 7; x < 15; x++) {
    if (vehicles[displayIDX].regLoc[x] != '\0') {
      lcd.print(vehicles[displayIDX].regLoc[x]);
    } else {
      break;
    }
  }
}

// Utility funtion to clear the serial buffer if neccessary. 
void serialFlush() {
  while (Serial.available() > 0) {  // while there is stuff in the buffer
    char x = Serial.read();
  }
}

// Function to process the A command
bool process_A() {
  /*
   *                    01234567890123456789012
   * Should be of form 'A-BC12DEF-A-AAAAAAAAAAA', first A is known by the fact it gets to this function
   */

  // Must have memory to add vehicle
  if (vehicleMax == 43) {
    Serial.println(F("ERROR: Not enough memory to add vehicle"));
    return false;
  }

  // A must be at least 13 char long
  if (msg.length() < 13) {
    Serial.println(F("ERROR: A command not long enough"));
    return false;
  }

  // Check first -
  if (msg.charAt(1) != '-') {
    Serial.println(F("ERROR: Missing first -"));
    return false;
  }

  // Check REG
  if (!isAlpha(msg.charAt(2)) || !isAlpha(msg.charAt(3))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }
  if (!isDigit(msg.charAt(4)) || !isDigit(msg.charAt(5))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }
  if (!isAlpha(msg.charAt(6)) || !isAlpha(msg.charAt(7)) || !isAlpha(msg.charAt(8))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }

  //Check second -
  if (msg.charAt(9) != '-') {
    Serial.println(F("ERROR: Missing Second -"));
    return false;
  }

  // Check Vehicle type is valid (C:1, M:2, V:3, L:4, B:5)
  if (toUpperCase(msg.charAt(10)) != 'C' && toUpperCase(msg.charAt(10)) != 'M' && toUpperCase(msg.charAt(10)) != 'V' && toUpperCase(msg.charAt(10)) != 'L' && toUpperCase(msg.charAt(10)) != 'B') {
    Serial.println(F("ERROR: Invalid Vehicle Type"));
    return false;
  }

  // Check third -
  if (msg.charAt(11) != '-') {
    Serial.println(F("ERROR: Missing third -"));
    return false;
  }

  // Check that the location string is valid
  for (int x = 12; x < msg.length(); x++) {
    if (!isAlphaNumeric(msg.charAt(x)) && msg.charAt(x) != '.') {
      Serial.println(F("ERROR: Invalid location"));
      return false;
    }
  }

  // Check if reg exists, record position if it does
  int idx = 99;  // init index value
  for (int i = 0; i < 44; i++) {
    bool vExists = true;
    for (int j = 0; j <= 6; j++) {
      if (vehicles[i].regLoc[j] != toUpperCase(msg.charAt(2 + j))) {
        vExists = false;
      }
    }
    if (vExists) {
      idx = i;
      break;
    }
  }
  if (idx == 99) {  // if car doesn't already exist:
    // Generate the data that will be inserted into the vehicle array
    // essentially a long concatination but using upper on the reg
    char reg[8];
    char loc[12];
    msg.substring(2, 9).toCharArray(reg, 8);
    msg.substring(12, msg.length()).toCharArray(loc, 12);
    char newRegLoc[19];
    for (int x = 0; x < 9; x++) { reg[x] = toUpperCase(reg[x]); }
    strcpy(newRegLoc, reg);
    strcat(newRegLoc, loc);

    strcpy(vehicles[vehicleMax].regLoc, newRegLoc);

    unsigned int typeTime = (timeNow / 1000 / 3600) * 100 + (((timeNow / 1000) % 3600) / 60); // calculate the entry time
    if (toUpperCase(msg.charAt(10)) == 'C') {
      typeTime += 10000;
    } else if (toUpperCase(msg.charAt(10)) == 'M') {
      typeTime += 20000;
    } else if (toUpperCase(msg.charAt(10)) == 'V') {
      typeTime += 30000;
    } else if (toUpperCase(msg.charAt(10)) == 'L') {
      typeTime += 40000;
    } else if (toUpperCase(msg.charAt(10)) == 'B') {
      typeTime += 50000;
    }

    vehicles[vehicleMax].typeEntry = typeTime;

    EEPROM.put(4 + vehicleMax * 23, vehicles[vehicleMax]);

    vehicleMax += 1;
  } else {
    // If car is NPD, can't change any of the details
    if (vehicles[idx].PaidExit < 10000) {
      Serial.println(F("ERROR: Vehicle NPD - Can't change details"));
      return false;
    }

    // Enter details into new data point
    char reg[8];
    char loc[12];
    msg.substring(2, 9).toCharArray(reg, 8);
    msg.substring(12, msg.length()).toCharArray(loc, 12);
    char newRegLoc[19];
    for (int x = 0; x < 9; x++) { reg[x] = toUpperCase(reg[x]); }
    strcpy(newRegLoc, toUpperCase(reg));
    strcat(newRegLoc, loc);

    unsigned int typeTime = (timeNow / 1000 / 3600) * 100 + (((timeNow / 1000) % 3600) / 60);
    if (toUpperCase(msg.charAt(10)) == 'C') {
      typeTime += 10000;
    } else if (toUpperCase(msg.charAt(10)) == 'M') {
      typeTime += 20000;
    } else if (toUpperCase(msg.charAt(10)) == 'V') {
      typeTime += 30000;
    } else if (toUpperCase(msg.charAt(10)) == 'L') {
      typeTime += 40000;
    } else if (toUpperCase(msg.charAt(10)) == 'B') {
      typeTime += 50000;
    }

    // Delete vehicle and sort list
    // Iterate over the list and pull each element 1 forward until the last element, and make that one null
    if (vehicleMax - 1 == idx) {
      vehicles[idx] = nullVehicle;
      EEPROM.put(4 + idx * 23, vehicles[idx]);
    } else {
      while (idx < vehicleMax - 1) {
        vehicles[idx] = vehicles[idx + 1];
        EEPROM.put(4 + idx * 23, vehicles[idx]);
        idx += 1;
      }
      vehicles[idx] = nullVehicle;
      EEPROM.put(4 + idx * 23, vehicles[idx]);
    }
    vehicleMax--;

    // Add new vehicle at end of list with saved details:
    strcpy(vehicles[vehicleMax].regLoc, newRegLoc);
    vehicles[vehicleMax].typeEntry = typeTime;
    EEPROM.put(4 + idx * 23, vehicles[vehicleMax]);
    vehicleMax += 1;
  }


  return true;
}

// Function to process the S command
bool process_S() {
  // WILL BE OF TYPE 'S-AA11AAA-PD' or 'S-AA11AAA-NPD', 12-13 char

  // A must be at least 12 char long, no more than 13
  if (msg.length() < 12 || msg.length() > 13) {
    Serial.println(F("ERROR: S command invalid length"));
    return false;
  }

  // Check first -
  if (msg.charAt(1) != '-') {
    Serial.println(F("ERROR: Missing first -"));
    return false;
  }

  // Check REG
  if (!isAlpha(msg.charAt(2)) || !isAlpha(msg.charAt(3))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }
  if (!isDigit(msg.charAt(4)) || !isDigit(msg.charAt(5))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }
  if (!isAlpha(msg.charAt(6)) || !isAlpha(msg.charAt(7)) || !isAlpha(msg.charAt(8))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }

  //Check second -
  if (msg.charAt(9) != '-') {
    Serial.println(F("ERROR: Missing second -"));
    return false;
  }

  // Check last section is either PD or NDP
  msg.toUpperCase();
  if (msg.length() == 12 && !(msg.substring(10, 12) == F("PD"))) {
    Serial.println(F("ERROR: Invalid payment type"));
    return false;
  }
  if (msg.length() == 13 && !(msg.substring(10, 13) == F("NPD"))) {
    Serial.println(F("ERROR: Invalid payment type"));
    return false;
  }

  // Check if reg exists, record position if it does
  int idx = 99;  // init index value
  for (int i = 0; i < 44; i++) {
    bool vExists = true;
    for (int j = 0; j <= 6; j++) {
      if (vehicles[i].regLoc[j] != toUpperCase(msg.charAt(2 + j))) {
        vExists = false;
      }
    }
    if (vExists) {
      idx = i;
      break;
    }
  }
  if (idx == 99) {
    Serial.println(F("ERROR: Non-Existent Reg"));
    return false;
  }  // if reg doesnt exist, error


  // What to do if request was for 'PD'
  if (msg.length() == 12) {
    // If vehicle is already PD
    if (vehicles[idx].PaidExit >= 10000) {
      Serial.println(F("ERROR: Can't chance PD status to PD"));
      return false;
    }
    // Set PD status and exit time
    else {
      vehicles[idx].PaidExit = 10000 + (timeNow / 1000 / 3600) * 100 + (((timeNow / 1000) % 3600) / 60);
      EEPROM.put(4 + idx * 23, vehicles[idx]);
    }
  }

  // What to do if request was for 'NPD'
  if (msg.length() == 13) {
    // If vehicle is already 'NPD'
    if (vehicles[idx].PaidExit < 10000) {
      Serial.println(F("ERROR: Can't change NPD status to NPD"));
      return false;
    } else {
      // Get old data that I need
      char oldRegLoc[19];
      strcpy(oldRegLoc, vehicles[idx].regLoc);
      char oldTypeEntry[5];
      itoa(vehicles[idx].typeEntry, oldTypeEntry, 10);
      unsigned int newTypeEntry = ((oldTypeEntry[0] - '0') * 10000) + (timeNow / 1000 / 3600) * 100 + (((timeNow / 1000) % 3600) / 60);

      // Delete vehicle and sort list
      if (vehicleMax - 1 == idx) {
        vehicles[idx] = nullVehicle;
        EEPROM.put(4 + idx * 23, vehicles[idx]);
      } else {
        while (idx < vehicleMax - 1) {
          vehicles[idx] = vehicles[idx + 1];
          EEPROM.put(4 + idx * 23, vehicles[idx]);
          idx += 1;
        }
        vehicles[idx] = nullVehicle;
        EEPROM.put(4 + idx * 23, vehicles[idx]);
      }
      vehicleMax--;

      // Add new vehicle at end of list with saved details:
      strcpy(vehicles[vehicleMax].regLoc, oldRegLoc);
      vehicles[vehicleMax].typeEntry = newTypeEntry;
      EEPROM.put(4 + idx * 23, vehicles[vehicleMax]);
      vehicleMax += 1;
    }
  }

  return true;
}

// Function to process the T command
bool process_T() {
  // Command will be in the form 'T-AA11AAA-C'

  msg.toUpperCase();

  // Command will only ever be 11 characters long
  if (msg.length() != 11) {
    Serial.println(F("ERROR: T command not correct length"));
    return false;
  }

  // Check first -
  if (msg.charAt(1) != '-') {
    Serial.println(F("ERROR: Missing first -"));
    return false;
  }

  // Check REG
  if (!isAlpha(msg.charAt(2)) || !isAlpha(msg.charAt(3))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }
  if (!isDigit(msg.charAt(4)) || !isDigit(msg.charAt(5))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }
  if (!isAlpha(msg.charAt(6)) || !isAlpha(msg.charAt(7)) || !isAlpha(msg.charAt(8))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }

  //Check second -
  if (msg.charAt(9) != '-') {
    Serial.println(F("ERROR: Missing second -"));
    return false;
  }

  // Check Vehicle type is valid (C:1, M:2, V:3, L:4, B:5)
  if (toUpperCase(msg.charAt(10)) != 'C' && toUpperCase(msg.charAt(10)) != 'M' && toUpperCase(msg.charAt(10)) != 'V' && toUpperCase(msg.charAt(10)) != 'L' && toUpperCase(msg.charAt(10)) != 'B') {
    Serial.println(F("ERROR: Invalid Vehicle Type"));
    return false;
  }

  // Check if reg exists, record position if it does
  int idx = 99;  // init index value
  for (int i = 0; i < 44; i++) {
    bool vExists = true;
    for (int j = 0; j <= 6; j++) {
      if (vehicles[i].regLoc[j] != toUpperCase(msg.charAt(2 + j))) {
        vExists = false;
      }
    }
    if (vExists) {
      idx = i;
      break;
    }
  }
  if (idx == 99) {
    Serial.println(F("ERROR: Non-Existent Reg"));
    return false;
  }  // if reg doesnt exist, error

  // If not paid can't change details
  if (vehicles[idx].PaidExit < 10000) {
    Serial.println(F("ERROR: Vehicle NPD - Can't update type"));
    return false;
  }

  // Calculate new type value
  unsigned int typeVal;
  if (toUpperCase(msg.charAt(10)) == 'C') {
    typeVal = 10000;
  } else if (toUpperCase(msg.charAt(10)) == 'M') {
    typeVal = 20000;
  } else if (toUpperCase(msg.charAt(10)) == 'V') {
    typeVal = 30000;
  } else if (toUpperCase(msg.charAt(10)) == 'L') {
    typeVal = 40000;
  } else if (toUpperCase(msg.charAt(10)) == 'B') {
    typeVal = 50000;
  }

  // Get old type value
  unsigned int oldTypeValue;
  if (vehicles[idx].typeEntry >= 50000) {
    oldTypeValue = 50000;
  } else if (vehicles[idx].typeEntry >= 40000) {
    oldTypeValue = 40000;
  } else if (vehicles[idx].typeEntry >= 30000) {
    oldTypeValue = 30000;
  } else if (vehicles[idx].typeEntry >= 20000) {
    oldTypeValue = 20000;
  } else if (vehicles[idx].typeEntry >= 10000) {
    oldTypeValue = 10000;
  }

  // If new and old type are the same
  if (typeVal == oldTypeValue) {
    Serial.println(F("ERROR: Can't set type to existing type"));
    return false;
  }

  // Update type value
  vehicles[idx].typeEntry -= oldTypeValue;
  vehicles[idx].typeEntry += typeVal;

  EEPROM.put(4 + idx * 23, vehicles[idx]);

  return true;
}

// Function to process the L command
bool process_L() {
  // Must be of type 'L-AA11AAA-a' to 'L-AA11AAA-aaaaaaaaaaa'

  // message must be between 11 and 21 characters
  if (msg.length() < 11 || msg.length() > 21) {
    Serial.println(F("ERROR: L command wrong length"));
    return false;
  }

  // Check first -
  if (msg.charAt(1) != '-') {
    Serial.println(F("ERROR: Missing first -"));
    return false;
  }

  // Check REG
  if (!isAlpha(msg.charAt(2)) || !isAlpha(msg.charAt(3))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }
  if (!isDigit(msg.charAt(4)) || !isDigit(msg.charAt(5))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }
  if (!isAlpha(msg.charAt(6)) || !isAlpha(msg.charAt(7)) || !isAlpha(msg.charAt(8))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }

  //Check second -
  if (msg.charAt(9) != '-') {
    Serial.println(F("ERROR: Missing second -"));
    return false;
  }

  // Check location string is valid
  for (int x = 10; x < msg.length(); x++) {
    if (!isAlphaNumeric(msg.charAt(x)) && msg.charAt(x) != '.') {
      Serial.println(F("ERROR: Invalid location"));
      return false;
    }
  }
  // Get the new location out of the message
  char newLoc[msg.length() - 9];
  msg.substring(10, msg.length()).toCharArray(newLoc, msg.length() - 9);

  // Check if reg exists, record position if it does
  int idx = 99;  // init index value
  for (int i = 0; i < 44; i++) {
    bool vExists = true;
    for (int j = 0; j <= 6; j++) {
      if (vehicles[i].regLoc[j] != toUpperCase(msg.charAt(2 + j))) {
        vExists = false;
      }
    }
    if (vExists) {
      idx = i;
      break;
    }
  }
  if (idx == 99) {
    Serial.println(F("ERROR: Non-Existent Reg"));
    return false;
  }  // if reg doesnt exist, error

  // If not paid can't change details
  if (vehicles[idx].PaidExit < 10000) {
    Serial.println(F("ERROR: Vehicle NPD - Can't update location"));
    return false;
  }

  // Get the old location out of the vehicles array
  char oldLoc[12];
  for (int i = 7; i < 19; i++) {
    oldLoc[i - 7] = vehicles[idx].regLoc[i];
  }

  // If the new and old location strings are equal then
  if (strlen(oldLoc) == strlen(newLoc)) {
    bool same = true;
    for (int i = 0; i < strlen(oldLoc); i++) {
      if (oldLoc[i] == newLoc[i]) {
        same = false;
      }
    }
    if (!same) {
      Serial.println(F("ERROR: Cant set location to be current location"));
      return false;
    }
  }
  char reg[8];
  char loc[12];
  msg.substring(2, 9).toCharArray(reg, 8);
  msg.substring(10, msg.length()).toCharArray(loc, 12);
  char newRegLoc[19];
  for (int x = 0; x < 9; x++) { reg[x] = toUpperCase(reg[x]); }
  strcpy(newRegLoc, reg);
  strcat(newRegLoc, loc);
  strcpy(vehicles[idx].regLoc, newRegLoc);

  //Update value in EEPROM
  EEPROM.put(4 + idx * 23, vehicles[idx]);
  return true;
}

// Function to process the R command
bool process_R() {
  // WIll always be of form 'R-AA11AAA'
  msg.toUpperCase();

  // message must be 9 characters
  if (msg.length() != 9) {
    Serial.println(F("ERROR: L command wrong length"));
    return false;
  }

  // Check first -
  if (msg.charAt(1) != '-') {
    Serial.println(F("ERROR: Missing first -"));
    return false;
  }

  // Check REG
  if (!isAlpha(msg.charAt(2)) || !isAlpha(msg.charAt(3))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }
  if (!isDigit(msg.charAt(4)) || !isDigit(msg.charAt(5))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }
  if (!isAlpha(msg.charAt(6)) || !isAlpha(msg.charAt(7)) || !isAlpha(msg.charAt(8))) {
    Serial.println(F("ERROR: Invalid Reg"));
    return false;
  }

  // Check if reg exists, record position if it does
  int idx = 99;  // init index value
  for (int i = 0; i < 44; i++) {
    bool vExists = true;
    for (int j = 0; j <= 6; j++) {
      if (vehicles[i].regLoc[j] != toUpperCase(msg.charAt(2 + j))) {
        vExists = false;
      }
    }
    if (vExists) {
      idx = i;
      break;
    }
  }
  if (idx == 99) {
    Serial.println(F("ERROR: Non-Existent Reg"));
    return false;
  }  // if reg doesnt exist, error

  // If not paid can't change details
  if (vehicles[idx].PaidExit < 10000) {
    Serial.println(F("ERROR: Vehicle NPD - Can't delete"));
    return false;
  }

  /*
   * To delete all the data from the vehicle, but to keep the memory in a sorted order, the memory will be cycled through
   * each element will be moved to the previous, with the last element being made blank. vehicleMax will also be
   * decremented to allow for more vehicles to be added
   */

  // If entry is last in dataset
  if (vehicleMax - 1 == idx) {
    vehicles[idx] = nullVehicle;
    EEPROM.put(4 + idx * 23, vehicles[idx]);
  } else {
    while (idx < vehicleMax - 1) {
      vehicles[idx] = vehicles[idx + 1];
      EEPROM.put(4 + idx * 23, vehicles[idx]);
      idx += 1;
    }
    vehicles[idx] = nullVehicle;
    EEPROM.put(4 + idx * 23, vehicles[idx]);
  }

  vehicleMax -= 1;
  displayIDX -= 1;

  if (vehicleMax == 1) {
    displayIDX += 1;
  }

  return true;
}

// master serial function, takes serial input and decides what to do with it
void processSerial() {
  // Read in serial command 
  msg = Serial.readString();

  Serial.print(F("DEBUG: "));
  Serial.println(msg);
  // Trip any leading/trailing \r or \n
  msg.trim();

/*
  if (msg.startsWith(F("rm*"))) {
    EEPROM.write(0, 5);
  }

  if (msg.startsWith(F("z"))) {               ////////
    for (int i = 0; i < 44; i++) {            ////////
      Serial.println(vehicles[i].regLoc);     ////////
      Serial.println(vehicles[i].typeEntry);  ////////
      Serial.println(vehicles[i].PaidExit);   ////////
      Serial.println("\n");                   ////////
    }
    Serial.println(vehicleMax);  ////////
  }

  if (msg.startsWith(F("="))) {
    displayIDX += 1;
    updateDisplay();
    Serial.println(displayIDX);
  }

  if (msg.startsWith(F("-"))) {
    displayIDX -= 1;
    updateDisplay();
    Serial.println(displayIDX);
  }

  if (msg.startsWith(F("x"))) {
    Serial.println(npdMax);
    Serial.println(" ");
    for (int x = 0; x < 44; x++) {
      Serial.println(npdVehicles[x]);
    }
    Serial.println(" ");
    Serial.println(pdMax);
    Serial.println(" ");
    for (int x = 0; x < 44; x++) {
      Serial.println(pdVehicles[x]);
    }
    Serial.println(" ");
  }
*/
  // All Serial messages/commands have to be in the range of 9 - 23 characters due to the strucuture of the commands
  if (msg.length() < 9 || msg.length() > 23) {
    Serial.println(F("ERROR: Invalid Serial Command (Too Long/Short)"));
    return;
  }
  // Run A command
  if (msg.startsWith(F("A"))) {
    if (!process_A()) { return; }
  }
  // Run S Command
  else if (msg.startsWith(F("S"))) {
    if (!process_S()) { return; }
  }
  // Run T Command
  else if (msg.startsWith(F("T"))) {
    if (!process_T()) { return; }
  }
  // Run L Command
  else if (msg.startsWith(F("L"))) {
    if (!process_L()) { return; }
  }
  // Run R Command
  else if (msg.startsWith(F("R"))) {
    if (!process_R()) { return; }
  }
  // If start is not above then it will error
  else {
    Serial.println(F("ERROR: Invalid Serial Command Prefix"));
    return;
  }

  // Refresh the NP and NPD lists to take into account all the different commands
  // This could be implemented into each function but its easier to modify when kept seperate
  for (int x = 0; x < 44; x++) { // zeroise the arrays
    npdVehicles[x] = 0;
    pdVehicles[x] = 0;
  }
  npdMax = 0;
  pdMax = 0;

  // same calculations that are used in setup
  for (int y = 0; y < 44; y++) {  
    if (vehicles[y].typeEntry != 0) {
      if (vehicles[y].PaidExit > 10000) {
        pdVehicles[pdMax] = y;
        pdMax += 1;
      } else {
        npdVehicles[npdMax] = y;
        npdMax += 1;
      }
    }
  }

  // Store Time
  EEPROM.put(1016, timeNow);
  updateDisplay();
  Serial.println("DONE!");
  serialFlush(); // Clear buffer of any excess data
}

// Initilse variable in global scope
unsigned long systemStartTime;  // Initilise variable to store the system time as saved to EEPROM

// Default setup function, runs when Arduino first starts
void setup() {
  Serial.begin(9600);  // initialise serial connection
  lcd.begin(16, 2);    // initialise the lcd display to be 16x2
  //lcd.cursor(); //debug line to show cursor position on LCD

  // Create custom characters (UDCHAR)
  lcd.createChar(0, upArrow);
  lcd.createChar(1, downArrow);

  // Set the LCD backlight to purple for the sync phase
  lcd.setBacklight(blPurple);

  bool startUp = true;                 // flag to control the start loop of 'Q's
  unsigned long startTime = millis();  // get the current time
  EEPROM.get(1016, systemStartTime);   // get system time as saved in EEPROM

  /*
   * Method for seeing if the data in EEPROM is from this program, or is random data. There is of course a extreamly small
   * chance that whatever program ran before produces the same values in the same places, but this is an increadiby small chance
   */
  unsigned long check1, check2;  // Initilise variables to store the check data
  EEPROM.get(0, check1);         // Retrieve check data
  EEPROM.get(1020, check2);

  /*
   * Both values of the check data are personal to me, making them non-random but also unlikeley to be guessed.
   * check1 is a concatination of my student ID '311453' and the year the program was written '2023',
   * check2 is a concatination of my date of birth '25/01/2004' and my age as of making this program '19'
   */
  if (check1 != 3114532023 || check2 != 2501200419) {
    //Serial.println(F("DEBUG: EEPROM DOES NOT MATCH EXPECTED DATA - RESETTING TO 0"));
    //Serial.println(F("DEBUG: WARNING... THIS MAY TAKE A MINUTE"));
    // If the check data isn't there, completely wipe the EEPROM and zero it all.
    for (int x = 0; x < EEPROM.length(); x++) {
      EEPROM.write(x, 0);
    }

    check1 = 3114532023;
    check2 = 2501200419;

    // Insert the check data for future runs of the progam
    EEPROM.put(0, check1);
    EEPROM.put(1020, check2);

    //Serial.println(F("DEBUG: DATA RESET COMPLETE"));
  } else {
    // Essentially do nothing
    //Serial.println(F("DEBUG: Data found"));
  }

  // every 23 bytes from address 0004 read in the data and assign it to an entry in the vehicles array
  // there is no need to sort the data as it will already be in order from the program putting it in that order
  for (int y = 0; y < 44; y++) {
    EEPROM.get(4 + y * 23, vehicles[y]);

    if (vehicles[y].typeEntry != 0) {
      vehicleMax += 1;

      if (vehicles[y].PaidExit > 10000) {
        pdVehicles[pdMax] = y;
        pdMax += 1;
      } else {
        npdVehicles[npdMax] = y;
        npdMax += 1;
      }
    }
  }
  //Serial.print(F("DEBUG: Found "));
  //Serial.print(vehicleMax);
  //Serial.println(F(" vehicles"));

  timeNow = millis();
  while (startUp) {
    // Print 'Q' every 1s
    if (millis() >= timeNow + 1000) {
      Serial.print('Q');
      timeNow = millis();
    }

    // If there is anything in the serial buffer:
    if (Serial.available() > 0) {
      msg = Serial.readString();

      // if ends with new line or carrage retrun 'treat as error'
      if (msg.endsWith(F("\n")) || msg.endsWith(F("\r"))) {
        //Serial.println(F("\nERROR: NOT EXPECTING \\n OR \\r"));
      }

      // if 'X' recieved break out of start up loop
      else if (msg == F("X")) {
        startUp = false;
      }

      // do nothing if anything else is recieved
      else {
        continue;
      }
    }
  }

  // I did all the extentions ^_^
  Serial.println(F("\nUDCHARS,FREERAM,HCI,EEPROM,SCROLL"));
  lcd.setBacklight(blWhite);
  updateDisplay();
}

// default loop function, runs forever (or until the arduino is unplugged \o/)
void loop() {
  // Initilise variables that are different for each iteration of the loop
  uint8_t buttons = lcd.readButtons();   // read button presses
  timeNow = millis() + systemStartTime;  // get current time

  // Initilise variables that are only initilised once (aka used between loops)
  static bool select_down = false;             // Boolean to keep track of whether the select button has been pressed or not
  static unsigned long time_select_down;       // time variable to keep track of when the select button was pressed
  static int SELECT_STATE = selectStateFalse;  // variable to control the select button FSM
  static unsigned long button_pressed;         // Variable to keep track of if a button has been pressed
  static bool locCalculated = false;           // Boolean to keep track of if the length of the location string has been calculated

  /*
   * Finite State Machine for controling the system state between being in "operation" mode ('selectStateFalse')
   * where the UI on the LCD will be active and working, or in "ID" mode ('selectStateTrue') where my ID (F311453)
   * is displayed along with the amount of free memory using the 'MemoryFree.h' library, completing the 'FREERAM'
   * extention.
   */
  switch (SELECT_STATE) {
    case selectStateFalse:
      {
        // If there is anything in the serial buffer process it
        if (Serial.available() > 0) {
          processSerial();
        }

        /*
         * Finite State machine for controlling the HCI aspect of the program
         * aka the left / right buttons for the interface to control what the up/down
         * buttons do in each state
         */
        switch (HCI_STATE) {
          case leftHCI:
            {
              // If the Up button is pressed
              if ((buttons & BUTTON_UP) && timeNow > button_pressed + 500) {
                // If not at top of the list
                if (displayIDX != npdVehicles[0]) {
                  // Index current displayIDX value against the npdVehicles to be able to find the next one
                  for (int x = 0; x < 44; x++) {
                    if (displayIDX == npdVehicles[x]) {
                      npdIDX = x;
                      break;
                    }
                  }
                  displayIDX = npdVehicles[npdIDX - 1];
                  locCalculated = false;
                  button_pressed = timeNow;
                  updateDisplay();
                }
              }

              // If the down button is pressed
              if ((buttons & BUTTON_DOWN) && timeNow > button_pressed + 500) {
                // If not at bottom of the list
                if (displayIDX != npdVehicles[npdMax - 1]) {
                  for (int x = 0; x < 44; x++) {
                    if (displayIDX == npdVehicles[x]) {
                      npdIDX = x;
                      break;
                    }
                  }
                  displayIDX = npdVehicles[npdIDX + 1];
                  locCalculated = false;
                  button_pressed = timeNow;
                  updateDisplay();
                }
              }

              // Only show NPD vehicles, else "NO NPD VEHICLES"
              if ((buttons & BUTTON_RIGHT) && timeNow > button_pressed + 500) {
                button_pressed = timeNow;
                HCI_STATE = centreHCI;
                locCalculated = false;
                displayIDX = 0;
                Serial.println(F("DEBUG: Switched to centreHCI"));
                updateDisplay();
              }
            }
            break;

          // Normal operation
          case centreHCI:
            {
              // If the Up button is pressed
              if ((buttons & BUTTON_UP) && timeNow > button_pressed + 500) {
                // If not at top of the list
                if (displayIDX != 0) {
                  button_pressed = timeNow;
                  displayIDX -= 1;
                  locCalculated = false;
                  updateDisplay();
                }
              }

              // If the down button is pressed
              if ((buttons & BUTTON_DOWN) && timeNow > button_pressed + 500) {
                // If not at bottom of the list
                if (displayIDX != vehicleMax - 1) {
                  button_pressed = timeNow;
                  displayIDX += 1;
                  locCalculated = false;
                  updateDisplay();
                }
              }

              // If the left button is pressed
              if ((buttons & BUTTON_LEFT) && timeNow > button_pressed + 500) {
                button_pressed = timeNow;
                HCI_STATE = leftHCI;
                locCalculated = false;
                Serial.println(F("DEBUG: Switched to leftHCI"));
                if (npdMax != 0) { displayIDX = npdVehicles[0]; }
                updateDisplay();
              }

              // If the right button is pressed
              if ((buttons & BUTTON_RIGHT) && timeNow > button_pressed + 500) {
                button_pressed = timeNow;
                HCI_STATE = rightHCI;
                locCalculated = false;
                Serial.println(F("DEBUG: Switched to rightHCI"));
                if (pdMax != 0) { displayIDX = pdVehicles[0]; }
                updateDisplay();
              }
            }
            break;

          case rightHCI:
            {
              // If the Up button is pressed
              if ((buttons & BUTTON_UP) && timeNow > button_pressed + 500) {
                // If not at top of the list
                if (displayIDX != pdVehicles[0]) {
                  for (int x = 0; x < 44; x++) {
                    if (displayIDX == pdVehicles[x]) {
                      pdIDX = x;
                      break;
                    }
                  }
                  displayIDX = pdVehicles[pdIDX - 1];
                  locCalculated = false;
                  button_pressed = timeNow;
                  updateDisplay();
                }
              }

              // If the down button is pressed
              if ((buttons & BUTTON_DOWN) && timeNow > button_pressed + 500) {
                // If not at bottom of the list
                if (displayIDX != pdVehicles[pdMax - 1]) {
                  for (int x = 0; x < 44; x++) {
                    if (displayIDX == pdVehicles[x]) {
                      pdIDX = x;
                      break;
                    }
                  }
                  displayIDX = pdVehicles[pdIDX + 1];
                  locCalculated = false;
                  button_pressed = timeNow;
                  updateDisplay();
                }
              }
              // Only show PD vehicles, else "NO PD VEHICLES"
              if ((buttons & BUTTON_LEFT) && timeNow > button_pressed + 500) {
                button_pressed = timeNow;
                HCI_STATE = centreHCI;
                locCalculated = false;
                Serial.println(F("DEBUG: Switched to centreHCI"));
                displayIDX = 0;
                updateDisplay();
              }
            }
        }

        // Scroll location string at 1 char per 0.5 seconds if longer than 7 char
        // If data is being shown and the location string is longer than 7 characters
        static int locStringLen;             // Store length of the location string
        static int locStringOffset;          // Store the current position of the location string
        static unsigned long locScrollTime;  // Timer value for the location string

        // If the scroll is turned on and its a new string
        if (displayData && !locCalculated) {
          locCalculated = true;
          locStringLen = 0;
          locStringOffset = 0;

          for (int x = 7; x < 19; x++) { // Calculate the length of the string stopping at the null terminator
            if (vehicles[displayIDX].regLoc[x] != '\0') {
              locStringLen += 1;
            } else {
              break;
            }
          }
        }

        if (displayData && locStringLen > 7) {
          if (timeNow > locScrollTime + 500) { // iterate every 0.5s
            locScrollTime = timeNow;

            lcd.setCursor(9, 0); // write over previous data
            for (int x = 7 + locStringOffset; x < 19; x++) { // use an ofset to display extra character
              if (vehicles[displayIDX].regLoc[x] != '\0') {
                lcd.print(vehicles[displayIDX].regLoc[x]);
              } else {
                break;
              }
            } // Reset the ofset, or increment it based on its current value
            if (locStringOffset >= locStringLen - 7) {
              locStringOffset = 0;
            } else {
              locStringOffset += 1;
            }
          }
        }


        // If the select button is pressed
        if (buttons & BUTTON_SELECT) {
          // If the system knows that its been pressed, and that at least 1s has passed
          if (select_down && timeNow > (time_select_down + 1000)) {
            // Switch to the other state, and do the prep work for it.
            SELECT_STATE = selectStateTrue;
            Serial.println(F("DEBUG: Switched to selectStateTrue"));
            lcd.setBacklight(blPurple);
            lcd.clear();
            lcd.print(F("F311453"));
            lcd.setCursor(0, 1);
            lcd.print(F("FREE SRAM: "));
          }
          // If this is the first loop of the button being down, record the time
          else if (!(select_down)) {
            select_down = true;
            time_select_down = timeNow;
          }
        }
        // If select isnt pressed, make sure to reset the flag for saying it is down
        else {
          select_down = false;
        }
      }
      break;

    case selectStateTrue:
      {
        // Update current amount of free memory
        lcd.setCursor(11, 1);
        lcd.print(freeMemory());
        lcd.print(F("B"));

        // If the select button is not pressed (aka you let go)
        if (!(buttons & BUTTON_SELECT)) {
          // Switch to other state, and prep work for it
          SELECT_STATE = selectStateFalse;
          Serial.println(F("DEBUG: Switched to selectStateFalse"));
          lcd.setBacklight(blWhite);
          lcd.clear();
          updateDisplay();
        }
      }
  }
}
