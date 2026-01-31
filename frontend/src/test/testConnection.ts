/**
 * Test script to demonstrate Arduino-Frontend communication
 * Simplified SafeWalk protocol
 * Run this with: ts-node testConnection.ts
 */

import { getArduinoServer } from '../services/udpServer';
import { getPairingService } from '../services/pairingService';
import { ArduinoStatus } from '../types/arduino.types';

async function main() {
  console.log('========================================');
  console.log('Arduino SafeWalk Connection Test');
  console.log('========================================\n');

  // Initialize services
  const arduinoServer = getArduinoServer();
  const pairingService = getPairingService();

  // Set up status update handler
  arduinoServer.onStatusUpdate((status: ArduinoStatus) => {
    console.log('\nStatus update from Arduino:');
    console.log('   Alert Active:', status.isAlertActive);
    console.log('   Last Response:', status.lastResponseTime.toLocaleTimeString());
  });

  console.log('UDP Server initialized and listening...\n');

  // Simulate device discovery
  console.log('Step 1: Discovering available devices...');
  const devices = await pairingService.getAvailableDevices();
  
  if (devices.length === 0) {
    console.log('\nWARNING: No devices found.');
    console.log('Make sure:');
    console.log('1. Arduino is powered on');
    console.log('2. Arduino is connected to WiFi');
    console.log('3. Arduino has sent registration to backend');
    console.log('\nWaiting for Arduino connection...\n');
  } else {
    console.log(`\nFound ${devices.length} device(s). Ready to pair!\n`);
    
    // For testing, you can automatically pair with first device
    // Uncomment below to auto-pair and test:
    // /*
    const device = devices[0];
    console.log(`\nStep 2: Pairing with ${device.deviceID}...`);
    const success = await arduinoServer.sendPairingRequest(device.ipAddress);
    console.log(success);
    if (success) {
      console.log('\n✓ Pairing successful!');
      console.log('Waiting for HELLO message from Arduino...\n');
      
      // Wait a bit, then test alert
    //   setTimeout(() => {
    //     console.log('\n=== TESTING ALERT ===');
    //     arduinoServer.sendAlertRequest();
        
    //     // Clear alert after 5 seconds
    //     setTimeout(() => {
    //       console.log('\n=== CLEARING ALERT ===');
    //       arduinoServer.clearAlert();
    //     }, 5000);
    //   }, 2000);
    }
    // */
  }

  // Keep server running
  console.log('Test running. Press Ctrl+C to exit.\n');
  
  // Prevent process from exiting
  process.stdin.resume();
}

// Run test
main().catch(console.error);
