#include "controller.h"
#include <CapacitiveSensor.h>


// REMOVE THIS WHEN NOT TESTING
// #define TESTING

#ifdef TESTING
  #include "test_mcu.h"
#endif

#define PIEZO_PIN 13

#define CTIME 8  // Capacitive sensing sample time
#define X A4
#define Y A5
#define P 4
#define S6 6 // Capacitive sensing connected to pins 6 - 9
#define S7 7
#define S8 8
#define S9 9

/*
Define the thresholds and sensors
*/
int capSensors[4];
int capSensThresholds[4];
int joystickThresholds[4];

// Game mode cooldowns (not used in SafeWalk)
// #ifdef ENABLE_GAME_MODE
// /*
// Initialize cooldowns in ms
//  */
// const int cooldowns[3] = {
//   15000, // BOMB since BOMB = 0
//   15000, // TURRET since TURRET = 1
//   30000 // MINE since MINE = 2
// };
// #endif

/*
Initialize SafeWalk message packet
*/
packet message = { 0x00 };  // Simple: 0xFF = alert, 0x00 = no alert

// Sound effects
// #ifdef ENABLE_GAME_MODE
// const String correctTone = "ding:d=16,o=6,b=200:e,g";
// const String incorrectTone = "ding:d=16,o=6,b=200:g,e";
// const String beepTone = "ding:d=32,o=6,b=200:c";
// #endif

// This is the notes buffer and duration buffer.
int notesBuf[256];
int dursBuf[256];

/*
setup() sets up everything needed to run the game or test the mcu code functionality.
It initializes the pinmodes for audio and capacitive sensing, sets thresholds for capacitive sensors
and joystick inputs, and initializing interrupts and the watchdog. There are no inputs or outputs.
*/
void setup() {
    // Mouse.begin(); // Start mouse control

  // Pin modes
  pinMode(PIEZO_PIN, OUTPUT);
  R_PFS->PORT[PIEZO_PORT].PIN[PIEZO_PIN_NUM].PmnPFS_b.PDR = 1;

  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Acknowledgment button with internal pull-up

  pinMode(S6, OUTPUT);
  pinMode(S7, INPUT);
  pinMode(S8, INPUT);
  pinMode(S9, INPUT);

  Serial.begin(9600);
  while (!Serial);



  #ifdef TESTING
    Serial.println("Entering TESTING mode...");
    delay(200);

    // Run all generated test cases
    runMCUTests();

    // Stop here — do not run gameplay while testing
    Serial.println("Testing complete. Halting.");

  #else

  if (R_SYSTEM->RSTSR1_b.WDTRF) {
    Serial.println("dog didnt not pet need reset!"); // resetting the watch dog
    R_SYSTEM->RSTSR1_b.WDTRF = 0;
  } else {
    Serial.println("Power-on or normal reset.");
  }

  if (!initWiFi()) {
    Serial.println("WiFi initialization failed!");
  }
 
  // init alerts (LED + speaker)
  initAlerts();
 
  // init interrupts
  initGPT();

  Serial.println("\n========================================");
  Serial.println("Setup complete");
  Serial.println("Status: WAITING FOR PAIRING");
  Serial.println("Device ready for frontend connection");
  Serial.print("Listening for UDP on port: ");
  Serial.println(UDP_LISTEN_PORT);
  Serial.print("Will send UDP to port: ");
  Serial.println(UDP_TARGET_PORT);
  Serial.println("========================================\n");

  //WATCH DAWG
  initWDT();
  Serial.println("initializing the watchdog please work.");

  #endif
}

/*
loop() sets up the main SafeWalk loop logic.
It handles pairing requests and listens for alert messages from the frontend.
Optional: Can also monitor capacitive sensors for local volunteer input.
*/
void loop() {
  #ifndef TESTING
  
  // Optional: CAPACITIVE SENSOR INPUT
  // Uncomment if you want volunteers to use touch sensors for acknowledgment
  /*
  long v6 = s6.capacitiveSensorRaw(CTIME);
  long v7 = s7.capacitiveSensorRaw(CTIME);
  long v8 = s8.capacitiveSensorRaw(CTIME);
  long v9 = s9.capacitiveSensorRaw(CTIME);

  // Check if any sensor exceeds its threshold
  bool pressed = (v6 > capSensThresholds[UP]) ||
                 (v7 > capSensThresholds[RIGHT]) ||
                 (v8 > capSensThresholds[DOWN]) ||
                 (v9 > capSensThresholds[LEFT]);

  static bool prevPressed = false;
  if (pressed && !prevPressed) {
    // Volunteer touched sensor - could clear alert or send acknowledgment
    Serial.println("Capacitive sensor pressed");
    // sendStatusPacket(0x00);  // Clear alert
  }
  prevPressed = pressed;
  */
  
  // Debug: Print status every 5 seconds
  static unsigned long lastStatusPrint = 0;
  static int loopCount = 0;
  loopCount++;
  
  if (millis() - lastStatusPrint >= 5000) {
    if (!deviceInfo.isPaired) {
      Serial.print("Still waiting for pairing... (loop iterations: ");
      Serial.print(loopCount);
      Serial.println(")");
      
      // Try to read any waiting packets
      int available = udp.parsePacket();
      Serial.print("UDP packets available: ");
      Serial.println(available);
    }
    loopCount = 0;
    lastStatusPrint = millis();
  }
  
  // Handle pairing requests (check before regular logic)
  if (!deviceInfo.isPaired) {
    handlePairingRequest();
  } else {
    // When paired, listen for requests from frontend
    receiveRequestFromFrontend();
    
    // Check for button press (volunteer acknowledging request)
    static bool lastButtonState = HIGH;
    int buttonState = digitalRead(BUTTON_PIN);
    
    // Button pressed (LOW because of pull-up resistor)
    if (buttonState == LOW && lastButtonState == HIGH && isAlertActive) {
      Serial.println("Button pressed - volunteer acknowledging request");
      
      // Use the proper stop function to clear alert state
      stopRequestAlert();
      
      // Play acceptance sound
      const String acceptTone = "accept:d=4,o=5,b=240:e,g,c6,e6";
      playToneSequenceISR(acceptTone);
      
      // Send acknowledgment to frontend (0x01 = acknowledged)
      uint8_t ackMsg = 0x01;
      udp.beginPacket(UDP_TARGET_IP, UDP_TARGET_PORT);
      udp.write(&ackMsg, 1);
      udp.endPacket();
      
      Serial.println("Acknowledgment sent to frontend | Alert cleared");
      
      delay(50);  // Debounce delay
    }
    lastButtonState = buttonState;
  }

  // Watchdog timer - pet regularly to prevent reset
  static unsigned long lastPet = 0;
  unsigned long now = millis();
  
  if (now - lastPet >= 2500) {   // Pet every 2.5 seconds
    petWDT();
    lastPet = now;
  }

  delay(10); // Small delay to prevent overload
  #endif
}


/*
Game FSM functions below are kept for reference but not used in SafeWalk mode.
You can safely remove everything below this line if not using game features.
*/

// GAME FSM CODE - NOT USED IN SAFEWALK MODE
// Kept for reference only
// #ifdef ENABLE_GAME_MODE
// full_state updateFSM(full_state currState, joystickInput joystick, capSensDir capSens, bool started, bool confirmed, bool dead, bool won, unsigned long clock) {
//   fsm_state state = currState.state;
//   full_state ret = currState;  

//   switch (state) {

//       //--------------------1.INIT-----------------------------------------------------------
//     case s_INIT:
//       // Serial.println("1.INIT state");
//       if (started == true) {  // t 1-2
//         // initialize everything
//         for (int i = 0; i < 4; i++) {
//           ret.inputQueue[i] = NONE;
//         }
//         ret.inputCount = 0;
//         ret.timeSinceLastDeployed[0] = -cooldowns[0];
//         ret.timeSinceLastDeployed[1] = -cooldowns[1];
//         ret.timeSinceLastDeployed[2] = -cooldowns[2];
//         ret.successfulStrat = EMPTY;

//         buildMsg(state, ret.successfulStrat, joystick);
//         ret.state = s_INPUT_WAIT;
//         Serial.println("TRANSITION 1-2 init --> input wait");
//       }
//       break;

//       //--------------------2.INPUT WAIT-----------------------------------------------------------
//     case s_INPUT_WAIT:
//       // Serial.println("2.INPUT WAIT state");
//       if (capSens != NONE && !dead && !won) {  // t 2-3
//         ret.inputQueue[ret.inputCount] = capSens;
//         ret.inputCount++;
//         ret.successfulStrat = checkStrats(ret.inputQueue);

//         ret.state = s_INPUT_CALC;
//         Serial.println("TRANSITION 2-3 input wait --> input calc");
//       }

//       else if (capSens == NONE && !dead && !won) {  // t 2-2
//         ret.state = s_INPUT_WAIT;                   // overkill but just for readability
//         Serial.println("TRANSITION 2-2 input wait --> input wait");
//       }

//       else if (won && !dead) {  // t 2-7
//         Serial.println("You won");
//         // reset vars
//         for (int i = 0; i < 4; i++) {
//           ret.inputQueue[i] = NONE;
//         }
//         ret.inputCount = 0;
//         ret.successfulStrat = EMPTY;

//         ret.state = s_GAME_WIN;
//         Serial.println("TRANSITION 2-7 input wait --> game win");
//       }

//       else if (dead) {  // t 2-6
//         Serial.println("u suck");
//         // reset vars
//         for (int i = 0; i < 4; i++) {
//           ret.inputQueue[i] = NONE;
//         } 
//         ret.inputCount = 0;
//         ret.successfulStrat = EMPTY;

//         ret.state = s_GAME_OVER;
//         Serial.println("TRANSITION 2-6 input wait --> game over");
//       }

//       buildMsg(state, ret.successfulStrat, joystick);
//       break;

//     //--------------------3.INPUT CALC-----------------------------------------------------------
//     case s_INPUT_CALC:
//       Serial.println("3.INPUT CALC state");

//       if (ret.inputCount < MAX_INPUT_COUNT) {  // t 3-2b

//         ret.state = s_INPUT_WAIT;
//         Serial.println("TRANSITION 3-2b input calc --> input wait");
//       }

//       else if (ret.inputCount == MAX_INPUT_COUNT && ret.successfulStrat != EMPTY && clock - ret.timeSinceLastDeployed[ret.successfulStrat] >= cooldowns[ret.successfulStrat]) { // t 3-4
//           // beep success
//           playToneSequenceISR(correctTone);


//           ret.state = s_DEPLOY;
//           Serial.println("TRANSITION 3-4 input calc --> deploy");
//         }

//       else if (ret.inputCount == MAX_INPUT_COUNT && (ret.successfulStrat == EMPTY || clock - ret.timeSinceLastDeployed[ret.successfulStrat] < cooldowns[ret.successfulStrat])) {  // t 3-2a
//         // reset inputQueue
//         for (int i = 0; i < 4; i++) {
//           ret.inputQueue[i] = NONE;
//         }
//         ret.inputCount = 0;
//         // beep failure
//         // tone(PIEZO_PIN, 1000);
//         playToneSequenceISR(incorrectTone);

//         ret.state = s_INPUT_WAIT;
//         Serial.println("TRANSITION 3-2a input calc --> input wait");
//       }

//       buildMsg(state, ret.successfulStrat, joystick);
//       break;

//     //--------------------4.DEPLOY-----------------------------------------------------------
//     case s_DEPLOY:
//       Serial.println("4.DEPLOY state");

//       if (joystick.buttonPressed && !won && !dead) { // t 4-5
//           // update cooldown
//           ret.timeSinceLastDeployed[ret.successfulStrat] = clock;

//           ret.state = s_DEPLOY_CONFIRMED;
//           Serial.println("TRANSITION 4-5 deploy --> deploy confirmed");
//         }

//       else if (!joystick.buttonPressed && !won && !dead) {  // t 4-4

//         ret.state = s_DEPLOY;  // overkill but just for readability
//         Serial.println("TRANSITION 4-4 deploy --> deploy");
//       }

//       else if (won && !dead) {  // t 4-7
//         Serial.println("You won");
//         // reset vars
//         for (int i = 0; i < 4; i++) {
//           ret.inputQueue[i] = NONE;
//         }
//         ret.inputCount = 0;
//         ret.successfulStrat = EMPTY;

//         ret.state = s_GAME_WIN;
//         Serial.println("TRANSITION 4-7 deploy --> game win");
//       }

//       else if (dead) {  // t 4-6
//         Serial.println("u suck");
//         // reset vars
//         for (int i = 0; i < 4; i++) {
//           ret.inputQueue[i] = NONE;
//         }
//         ret.inputCount = 0;
//         ret.successfulStrat = EMPTY;

//         ret.state = s_GAME_OVER;
//         Serial.println("TRANSITION 4-6 deploy --> game over");
//       }

//       buildMsg(state, ret.successfulStrat, joystick);
//       break;

//     //--------------------5.DEPLOY_CONFIRMED-----------------------------------------------------------
//     case s_DEPLOY_CONFIRMED:
//       Serial.println("5.DEPLOY_CONFIRMED state");

//       if (!confirmed && !won && !dead) {  // t 5-5
//         ret.state = s_DEPLOY_CONFIRMED;     // overkill but just for readability
//         Serial.println("TRANSITION 5-5 deploy confirmed --> deploy confirmed");
//       }

//       else if (confirmed && !won && !dead) {  // t 5-2
//         // beep deploy confirmed
//         playToneSequenceISR(correctTone);

//         // reset vars
//         for (int i = 0; i < 4; i++) {
//           ret.inputQueue[i] = NONE;
//         }
//         ret.inputCount = 0;
//         ret.successfulStrat = EMPTY;

//         ret.state = s_INPUT_WAIT;
//       }

//       else if (won && !dead) {  // t 5-7
//         Serial.println("You won");
//         // reset vars
//         for (int i = 0; i < 4; i++) {
//           ret.inputQueue[i] = NONE;
//         }
//         ret.inputCount = 0;
//         ret.successfulStrat = EMPTY;

//         ret.state = s_GAME_WIN;
//         Serial.println("TRANSITION 5-7 deploy confirmed --> game win");
//       }

//       else if (dead) {  // t 5-6
//         Serial.println("u suck");
//         // reset vars
//         for (int i = 0; i < 4; i++) {
//           ret.inputQueue[i] = NONE;
//         }
//         ret.inputCount = 0;
//         ret.successfulStrat = EMPTY;

//         ret.state = s_GAME_OVER;
//         Serial.println("TRANSITION 5-6 deploy confirmed --> game over");
//       }

//       buildMsg(state, ret.successfulStrat, joystick);
//       break;

//     //--------------------6.GAME_OVER-----------------------------------------------------------
//     case s_GAME_OVER:
//       // terminal state: no outgoing transitions
//       Serial.println("u suck balls");
//       break;

//       //--------------------7.GAME_WIN-----------------------------------------------------------
//     case s_GAME_WIN:      
//       // terminal state: no outgoing transitions
//       Serial.println("ur pretty good chump");
//       break;

//     default:
//       Serial.println("Invalid state");
//       while (true)
//         ;
//     }
//   return ret;
// }
// #endif // ENABLE_GAME_MODE