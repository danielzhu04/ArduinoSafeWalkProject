/*
 * WiFi Example Configuration for Arduino UNO R4 WiFi
 * 
 */

 #ifndef WIFI_CONFIG_H
 #define WIFI_CONFIG_H

 const char WIFI_SSID[] = "WIFI NAME";
 const char WIFI_PASSWORD[] = "XXXXX"; 
 
 // Target Device (your Mac/PC)
 const char UDP_TARGET_IP[] = "xxx.xxx.xxx.xxx";
 
 // UDP Ports
 const int UDP_TARGET_PORT = 8888;  // Send
 const int UDP_LISTEN_PORT = 8889;  // Listen
 
 #endif
 
 