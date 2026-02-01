#include "controller.h"
#include <CapacitiveSensor.h>
#include <WiFiS3.h>

#define CTIME 8

// SafeWalk protocol message types
#define MSG_HELLO 0xAA
#define MSG_PAIR_REQUEST 0xBB
#define MSG_PAIR_CONFIRM 0xCC
#define MSG_REQUEST_ALERT 0xFF  // Frontend sends this to trigger alert
#define MSG_NO_ALERT 0x00       // Frontend sends this for no alert
#define MSG_ACKNOWLEDGED 0x01   // Arduino sends this when volunteer acknowledges

WiFiUDP udp;
DeviceInfo deviceInfo;

// Global alert state to prevent retriggering
volatile bool isAlertActive = false;


// buildMsg() removed - not needed 
/*
 * Initialize WiFi connection 
 * return true if successful, false otherwise
 */
 
/*
 * Get MAC address as unique device ID
 */
String getMACAddress() {
  byte mac[6];
  WiFi.macAddress(mac);
  String macStr = "";
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 16) macStr += "0";
    macStr += String(mac[i], HEX);
    if (i < 5) macStr += ":";
  }
  macStr.toUpperCase();
  return macStr;
}

/*
 * Set the target frontend IP address dynamically
 */
void setTargetIP(const char* newIP) {
  strncpy(UDP_TARGET_IP, newIP, 15);
  UDP_TARGET_IP[15] = '\0';
  Serial.print("Target IP updated to: ");
  Serial.println(UDP_TARGET_IP);
}

/*
 * Initialize WiFi and device info
 */
bool initWiFi() {
  Serial.print("Connecting to: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Wait for connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nERROR: Failed to connect to WiFi");
    return false;
  }
  
  Serial.println("\nWiFi Connected!");
  
  // Wait for DHCP to assign IP address
  IPAddress localIP = WiFi.localIP();
  attempts = 0;
  while (localIP[0] == 0 && attempts < 10) {
    delay(500);
    localIP = WiFi.localIP();
    Serial.print(".");
    attempts++;
  }
  
  if (localIP[0] == 0) {
    Serial.println("\nERROR: Failed to get IP address from DHCP");
    return false;
  }
  
  // Get device info
  String macAddr = getMACAddress();
  macAddr.toCharArray(deviceInfo.deviceID, 18);
  
  sprintf(deviceInfo.ipAddress, "%d.%d.%d.%d", 
          localIP[0], localIP[1], localIP[2], localIP[3]);
  
  deviceInfo.isPaired = false;
  strcpy(deviceInfo.pairedFrontendIP, "0.0.0.0");
  
  Serial.print("Device ID (MAC): ");
  Serial.println(deviceInfo.deviceID);
  Serial.print("Arduino IP: ");
  Serial.println(deviceInfo.ipAddress);
  Serial.print("Listening on port: ");
  Serial.println(UDP_LISTEN_PORT);
  
    // Initialize UDP socket
    if (udp.begin(UDP_LISTEN_PORT)) {
      Serial.print("UDP socket initialized on port ");
      Serial.println(UDP_LISTEN_PORT);
    } else {
      Serial.println("ERROR: Failed to initialize UDP socket");
      return false;
    }
    
    // Test UDP by trying to send/receive
    Serial.println("UDP ready to receive packets");
    
    // Register with backend
    sendRegistrationToBackend();
    
    return true;
}

/*
 * Send device registration to backend server
 */
void sendRegistrationToBackend() {
  Serial.println("\nRegistering device with backend...");
  
  // Create JSON-like registration message
  String regMsg = "{\"type\":\"register\",\"deviceID\":\"";
  regMsg += deviceInfo.deviceID;
  regMsg += "\",\"ip\":\"";
  regMsg += deviceInfo.ipAddress;
  regMsg += "\",\"port\":";
  regMsg += String(UDP_LISTEN_PORT);
  regMsg += ",\"status\":\"available\"}";
  
  udp.beginPacket(BACKEND_IP, BACKEND_PORT);
  udp.print(regMsg);
  udp.endPacket();
  
  Serial.print("Registration sent to backend: ");
  Serial.print(BACKEND_IP);
  Serial.print(":");
  Serial.println(BACKEND_PORT);
  Serial.println(regMsg);
}


/*
 * Send a simple status packet via UDP
 * status: 0xFF = ready/active, 0x00 = inactive/error
 */
void sendStatusPacket(uint8_t status) {
  if (!deviceInfo.isPaired) {
    Serial.println("WARNING: Not paired, cannot send status");
    return;
  }

  udp.beginPacket(UDP_TARGET_IP, UDP_TARGET_PORT);
  udp.write(status);
  udp.endPacket();

  Serial.print("Sent status: 0x");
  Serial.println(status, HEX);
}

/*
 * Handle incoming pairing request from frontend
 */
void handlePairingRequest() {
  int packetSize = udp.parsePacket();
  
  if (packetSize > 0) {
    Serial.print("Received UDP packet: ");
    Serial.print(packetSize);
    Serial.println(" bytes");
    
    uint8_t msgType = udp.read();
    Serial.print("Message type: 0x");
    Serial.println(msgType, HEX);
    
    // Check if it's a pairing request
    if (msgType == MSG_PAIR_REQUEST && !deviceInfo.isPaired) {
      IPAddress remoteIP = udp.remoteIP();
      sprintf(deviceInfo.pairedFrontendIP, "%d.%d.%d.%d", 
              remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3]);
      
      setTargetIP(deviceInfo.pairedFrontendIP);
      deviceInfo.isPaired = true;
      
      Serial.println("\nPAIRING REQUEST RECEIVED");
      Serial.print("Frontend IP: ");
      Serial.println(deviceInfo.pairedFrontendIP);
      
      // Send confirmation back
      udp.beginPacket(deviceInfo.pairedFrontendIP, UDP_TARGET_PORT);
      udp.write(MSG_PAIR_CONFIRM);
      udp.write((uint8_t*)deviceInfo.deviceID, 17);
      udp.endPacket();
      
      Serial.println("Pairing confirmed");
      
      // Trigger alert (LED + speaker)
      triggerRequestAlert();
      
      // Send hello packet
      delay(100);
      sendHelloPacket();
    }
  }
}

/*
 * Send initial hello packet after pairing
 */
void sendHelloPacket() {
  if (!deviceInfo.isPaired) {
    Serial.println("WARNING: Not paired yet, cannot send hello");
    return;
  }
  
  Serial.println("Sending HELLO packet to frontend...");
  
  udp.beginPacket(UDP_TARGET_IP, UDP_TARGET_PORT);
  udp.write(MSG_HELLO);
  udp.endPacket();
  
  Serial.println("HELLO packet sent");
}

/*
 * Receive request from frontend
 * 0xFF (all 1s) = trigger alert (safe walk request received)
 * 0x00 (all 0s) = no alert / clear alert
 */
void receiveRequestFromFrontend() {
  int packetSize = udp.parsePacket();
  
  if (packetSize >= 1) {
    uint8_t receivedByte = udp.read();
    
    // Check if it's a pairing request (only when not paired)
    if (receivedByte == MSG_PAIR_REQUEST && !deviceInfo.isPaired) {
      udp.flush();
      return; // Will be handled by handlePairingRequest
    }
    
    // Handle request messages (when paired)
    if (deviceInfo.isPaired) {
      
      // Only trigger alert if not already active (prevents retriggering)
      if (receivedByte == MSG_REQUEST_ALERT && !isAlertActive) {
        Serial.println("\nSAFE WALK REQUEST RECEIVED");
        triggerRequestAlert();
        
        // Send ACK back
        sendStatusPacket(0xFF);  // Confirm we received the request
        
      } else if (receivedByte == MSG_NO_ALERT && isAlertActive) {
        Serial.println("Request cleared from frontend");
        stopRequestAlert();
        
        // Send ACK back
        sendStatusPacket(0x00);
      }
    }
  }
}

/*
 * Initialize LED and speaker alerts
 */
void initAlerts() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.println("Alert system initialized");
}

/*
 * Trigger alert when request comes in (LED + speaker)
 */
void triggerRequestAlert() {
  if (!ALERT_ENABLED) return;
  
  Serial.println("Triggering request alert");
  
  // Set alert state
  isAlertActive = true;
  
  // Turn on LED
  digitalWrite(LED_PIN, HIGH);
  
  // Play alert tone (continuous or repeating)
  const String alertTone = "alert:d=8,o=6,b=200:c,e,g,c7,e7,g7";
  playToneSequenceISR(alertTone);
  
  Serial.println("LED ON | Speaker ACTIVE | Alert State: ACTIVE");
  // LED stays on until stopRequestAlert() is called
}

/*
 * Stop the alert (turn off LED and speaker)
 */
void stopRequestAlert() {
  if (!ALERT_ENABLED) return;
  
  Serial.println("Stopping alert");
  
  // Clear alert state
  isAlertActive = false;
  
  // Turn off LED
  digitalWrite(LED_PIN, LOW);
  
  // Stop any playing sound
  stopPlay();
  
  Serial.println("LED OFF | Speaker OFF | Alert State: INACTIVE");
}

/*
 * Set LED state
 */
void setLEDState(bool state) {
  digitalWrite(LED_PIN, state ? HIGH : LOW);
}