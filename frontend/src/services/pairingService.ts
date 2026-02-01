/**
 * Mock Pairing Service
 * Handles device discovery and pairing with Arduino devices
 */

import { DeviceInfo } from '../types/arduino.types';

export class PairingService {
  private backendURL: string;

  constructor(backendURL: string = 'http://localhost:3001') {
    this.backendURL = backendURL;
  }

  /**
   * Fetch available Arduino devices from backend
   */
  async getAvailableDevices(): Promise<DeviceInfo[]> {
    try {
      console.log('Fetching available devices from backend...');
      
      const response = await fetch(`${this.backendURL}/api/devices/available`);
      
      if (!response.ok) {
        throw new Error(`Backend returned ${response.status}`);
      }

      const devices = await response.json() as DeviceInfo[];
      
      console.log(`Found ${devices.length} available device(s)`);
      devices.forEach(device => {
        console.log(`  - ${device.deviceID} at ${device.ipAddress}`);
      });

      return devices;
    } catch (error) {
      console.error('Error fetching devices:', error);
      
      // Return mock data for testing
      console.log('WARNING: Using mock device data for testing');
      return this.getMockDevices();
    }
  }

  /**
   * Register a device pairing with backend
   */
  async registerPairing(deviceID: string, frontendIP: string): Promise<boolean> {
    try {
      console.log(`\nRegistering pairing with backend...`);
      console.log(`   Device: ${deviceID}`);
      console.log(`   Frontend: ${frontendIP}`);

      const response = await fetch(`${this.backendURL}/api/devices/pair`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          deviceID,
          frontendIP,
          timestamp: new Date().toISOString(),
        }),
      });

      if (!response.ok) {
        throw new Error(`Backend returned ${response.status}`);
      }

      const result = await response.json() as { success: boolean };
      console.log('Pairing registered with backend');
      
      return result.success;
    } catch (error) {
      console.error('Error registering pairing:', error);
      // Continue anyway for testing
      return true;
    }
  }

  /**
   * Notify backend of unpairing
   */
  async unregisterPairing(deviceID: string): Promise<boolean> {
    try {
      const response = await fetch(`${this.backendURL}/api/devices/unpair`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ deviceID }),
      });

      return response.ok;
    } catch (error) {
      console.error('Error unregistering pairing:', error);
      return false;
    }
  }

  /**
   * Mock device data for testing
   */
  private getMockDevices(): DeviceInfo[] {
    // IMPORTANT: Update this IP to match your actual Arduino IP
    // Check Arduino Serial Monitor for the real IP address
    return [
      {
        deviceID: 'F4:12:FA:9F:A5:B0',  // Your actual Arduino MAC
        ipAddress: '128.148.140.215',  // [UPDATE THIS] to your Arduino's real IP
        port: 8889,
        status: 'available',
        lastSeen: new Date(),
      },
    ];
  }

  /**
   * Get local IP address (needs to be implemented based on environment)
   */
  async getLocalIP(): Promise<string> {
    // This would use Node.js 'os' module or browser WebRTC
    // For now, return placeholder
    return '192.168.1.50';
  }
}

// Singleton instance
let pairingServiceInstance: PairingService | null = null;

export function getPairingService(backendURL?: string): PairingService {
  if (!pairingServiceInstance) {
    pairingServiceInstance = new PairingService(backendURL);
  }
  return pairingServiceInstance;
}
