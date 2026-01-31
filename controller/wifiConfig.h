/*
 * WiFi Configuration for Arduino UNO R4 WiFi
 * 
 * IMPORTANT: This file contains sensitive credentials and should NOT be committed to git
 */

#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

// const char WIFI_SSID[] = "DanieliPhone";
// const char WIFI_PASSWORD[] = "hzucdi167qiv";
const char WIFI_SSID[] = "RLAB";
const char WIFI_PASSWORD[] = "metropolis"; 

// Backend Server (for device registration)
const char BACKEND_IP[] = "128.148.140.151";  // Backend server IP
const int BACKEND_PORT = 3001;  // Backend registration port

// Dynamic Target Device (Frontend) - will be set during pairing
char UDP_TARGET_IP[16] = "0.0.0.0";  // Initially unpaired

// UDP Ports
const int UDP_TARGET_PORT = 8888;  // Send to frontend
const int UDP_LISTEN_PORT = 8889;  // Listen from frontend

// Hardware Pins
const int LED_PIN = 12;  // LED for visual feedback
const int ALERT_ENABLED = true;  // Enable/disable alerts

#endif

