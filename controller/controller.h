#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <CapacitiveSensor.h>
#include <WiFiS3.h>  // WiFi library for Arduino UNO R4 WiFi
#include "wifiConfig.h"  // Git Ignored, must add your own CREDENTIALS

/*
  constants for gpt file
*/
#define CLOCKFREQUENCY 3000000 // from lab5
#define PIEZO_PORT 1 // we use oin 13 for speaker so port 1 and pin 5
#define PIEZO_PIN_NUM 2
#define TIMER_INT 13 // changed to use 13 and 14 because for some reason, 1 and 2 was not working. maybe its because theyre being used by something else?
#define NOTE_INT 14
#define WDT_INT 30

// watchdog stuff
void initWDT();
void petWDT();
void wdtISR();

// interrupt stuff
void initGPT();
void initNoteGPT();
void stopPlay();
void playToneSequenceISR(String song);
void gptISR();
void noteISR();

extern int noteFrequencies[100];
extern int noteDurations[100];
extern int songLen;
extern volatile int intcount;

/*
Declarations for capacitive sensing
*/
extern CapacitiveSensor s6;
extern CapacitiveSensor s7;
extern CapacitiveSensor s8;
extern CapacitiveSensor s9;
extern int capSensors[4];
extern int capSensThresholds[4];
extern int joystickThresholds[4]; // up, down, left, right

typedef enum {
  UP,
  RIGHT,
  DOWN,
  LEFT,
  NONE
} capSensDir;

typedef struct {
  int x;
  int y;
  bool buttonPressed;
} joystickInput;

/*
Declarations for SafeWalk messages
Simple protocol: 
  - Frontend sends 0xFF to trigger alert (request received)
  - Frontend sends 0x00 for no alert
*/
typedef struct{
    uint8_t data;  // 0xFF = trigger alert, 0x00 = no alert
} packet;
extern packet message;

/*
Game-related declarations (optional - not used in SafeWalk mode)
Uncomment #define ENABLE_GAME_MODE below to use these features
*/
// #define ENABLE_GAME_MODE

#ifdef ENABLE_GAME_MODE
/*
Declarations for stratagem logic
*/
extern const int cooldowns[3];
typedef enum {
  BOMB = 0,
  TURRET = 1,
  MINE = 2,
  EMPTY = 3
} stratEnum;

extern const capSensDir stratArray[3][4];

/*
Declarations for fsm logic
*/
typedef enum {
  s_INIT = 1,
  s_INPUT_WAIT = 2,
  s_INPUT_CALC = 3,
  s_DEPLOY = 4,
  s_DEPLOY_CONFIRMED = 5,
  s_GAME_OVER = 6,
  s_GAME_WIN = 7
} fsm_state;

typedef struct {
  capSensDir inputQueue[4];
  int inputCount;
  long timeSinceLastDeployed[3];
  stratEnum successfulStrat;
  fsm_state state;
} full_state;

extern const int MAX_INPUT_COUNT = 4;
#endif // ENABLE_GAME_MODE

/*
 * Pairing and Device Info
 */
typedef struct {
  char deviceID[18];        // MAC address as device ID
  char ipAddress[16];       // Arduino's IP address
  bool isPaired;            // Pairing status
  char pairedFrontendIP[16]; // Frontend IP address
} DeviceInfo;

extern DeviceInfo deviceInfo;

/*
 * Utilities for WiFi
 */
extern WiFiUDP udp;
bool initWiFi();
void sendStatusPacket(uint8_t status);
void receiveRequestFromFrontend();
void setTargetIP(const char* newIP);
String getMACAddress();
void sendRegistrationToBackend();
void sendHelloPacket();
void handlePairingRequest();

/*
 * Utilities for LED and Speaker alerts
 */
void initAlerts();
void triggerRequestAlert();
void stopRequestAlert();
void setLEDState(bool state);

/*
 * Utilities for capacitive sensor
 */
capSensDir getCapSensInput();
void calibrate();
void testCalibration();

/*
 * Game mode utilities (not used in SafeWalk)
 */
#ifdef ENABLE_GAME_MODE
full_state updateFSM(full_state currState, joystickInput joystick, capSensDir capSens, bool started, bool confirmed, bool dead, bool won, unsigned long clock);
stratEnum checkStrats(capSensDir inputs[]);
void buildMsg(fsm_state state, stratEnum successfulStrat, joystickInput joystick);
#endif

//watchdog stuffs
void initWatchdog();
void petWatchdog();

#endif