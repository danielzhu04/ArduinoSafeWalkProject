/**
 * Mock UDP Server for Arduino SafeWalk Communication
 * Simplified protocol: Send 0xFF to trigger alert, 0x00 to clear
 * This would run on Node.js backend or Electron main process
 */

import dgram from 'dgram';
import { DeviceInfo, ArduinoStatus, MessageTypes } from '../types/arduino.types';

export class ArduinoUDPServer {
  private server: dgram.Socket | null = null;
  private pairedDevice: DeviceInfo | null = null;
  private availableDevices: Map<string, DeviceInfo> = new Map();
  private statusCallback: ((status: ArduinoStatus) => void) | null = null;
  private alertActive: boolean = false;

  // Configuration
  private readonly LISTEN_PORT = 8888; // Frontend listens here for Arduino responses
  private readonly SEND_PORT = 8889;   // Frontend sends here to Arduino

  constructor() {
    this.server = dgram.createSocket('udp4');
    this.setupServer();
  }

  /**
   * Initialize UDP server
   */
  private setupServer(): void {
    if (!this.server) return;

    this.server.on('listening', () => {
      const address = this.server!.address();
      console.log(`UDP Server listening on ${address.address}:${address.port}`);
      console.log('Waiting for Arduino connection...\n');
    });

    this.server.on('message', (msg, rinfo) => {
      this.handleIncomingMessage(msg, rinfo);
    });

    this.server.on('error', (err) => {
      console.error('UDP Server error:', err);
      this.server?.close();
    });

    this.server.bind(this.LISTEN_PORT);
  }

  /**
   * Handle incoming UDP messages from Arduino
   */
  private handleIncomingMessage(msg: Buffer, rinfo: dgram.RemoteInfo): void {
    console.log(`\n<< Received ${msg.length} bytes from ${rinfo.address}:${rinfo.port}`);
    console.log(`   Message type: 0x${msg[0].toString(16).toUpperCase()}`);

    if (msg.length >= 1) {
      const messageByte = msg[0];

      // Handle different message types
      if (messageByte === MessageTypes.HELLO) {
        this.handleHelloMessage(rinfo);
      } else if (messageByte === MessageTypes.PAIR_CONFIRM) {
        this.handlePairConfirm(msg, rinfo);
      } else if (messageByte === MessageTypes.REQUEST_ALERT) {
        this.handleStatusResponse(MessageTypes.REQUEST_ALERT, rinfo);
      } else if (messageByte === MessageTypes.NO_ALERT) {
        this.handleStatusResponse(MessageTypes.NO_ALERT, rinfo);
      } else {
        console.log('Unknown message type: 0x' + messageByte.toString(16));
      }
    }
  }

  /**
   * Handle HELLO message from Arduino
   */
  private handleHelloMessage(rinfo: dgram.RemoteInfo): void {
    console.log('HELLO message received from Arduino');
    console.log(`   Device IP: ${rinfo.address}:${rinfo.port}`);
    console.log('Connection established\n');
  }

  /**
   * Handle pairing confirmation from Arduino
   */
  private handlePairConfirm(msg: Buffer, rinfo: dgram.RemoteInfo): void {
    if (msg.length >= 18) {
      const deviceID = msg.slice(1, 18).toString('utf8');
      console.log('Pairing confirmed');
      console.log(`   Device ID: ${deviceID}`);
      console.log(`   Device IP: ${rinfo.address}`);

      this.pairedDevice = {
        deviceID,
        ipAddress: rinfo.address,
        port: rinfo.port,
        status: 'paired',
        lastSeen: new Date(),
      };

      console.log('Device successfully paired\n');
    }
  }

  /**
   * Handle status response from Arduino
   */
  private handleStatusResponse(status: number, rinfo: dgram.RemoteInfo): void {
    const isAlert = status === MessageTypes.REQUEST_ALERT;
    
    console.log(`Status response received: ${isAlert ? 'ALERT ACTIVE' : 'ALERT CLEARED'}`);
    console.log(`   From: ${rinfo.address}:${rinfo.port}`);

    // Update last seen
    if (this.pairedDevice && this.pairedDevice.ipAddress === rinfo.address) {
      this.pairedDevice.lastSeen = new Date();
    }

    // Callback to frontend
    if (this.statusCallback) {
      this.statusCallback({
        isAlertActive: isAlert,
        lastResponseTime: new Date(),
      });
    }
  }

  /**
   * Send pairing request to Arduino
   */
  sendPairingRequest(arduinoIP: string): Promise<boolean> {
    return new Promise((resolve) => {
      console.log(`\nSending pairing request to ${arduinoIP}...`);
      console.log(`Frontend will listen on port ${this.LISTEN_PORT} for response`);
      console.log(`Sending to Arduino port ${this.SEND_PORT}`);

      const buffer = Buffer.from([MessageTypes.PAIR_REQUEST]);
      this.server?.send(buffer, this.SEND_PORT, arduinoIP, (err) => {
        if (err) {
          console.error('Error sending pairing request:', err);
          resolve(false);
        } else {
          console.log('Pairing request sent successfully');
          console.log('Waiting for Arduino response...');
          
          // Wait for confirmation (with timeout)
          setTimeout(() => {
            const paired = this.pairedDevice?.ipAddress === arduinoIP;
            if (paired) {
              console.log('SUCCESS: Pairing confirmed!');
            } else {
              console.log('TIMEOUT: No response from Arduino');
              console.log('Possible issues:');
              console.log('  1. Arduino not on same network');
              console.log('  2. Firewall blocking UDP port 8889');
              console.log('  3. Arduino not receiving packets');
              console.log(`  4. Check Arduino Serial Monitor for pairing message`);
            }
            resolve(paired);
          }, 5000);  // Increased to 5 seconds
        }
      });
    });
  }

  /**
   * Send alert request to Arduino (trigger speaker + LED)
   */
  sendAlertRequest(arduinoIP?: string): void {
    const targetIP = arduinoIP || this.pairedDevice?.ipAddress;
    
    if (!targetIP) {
      console.error('WARNING: No Arduino IP available. Pair a device first.');
      return;
    }

    console.log(`\nSending ALERT REQUEST (0xFF) to ${targetIP}...`);
    
    const buffer = Buffer.from([MessageTypes.REQUEST_ALERT]);
    this.server?.send(buffer, this.SEND_PORT, targetIP, (err) => {
      if (err) {
        console.error('Error sending alert request:', err);
      } else {
        this.alertActive = true;
        console.log('Alert request sent - Arduino should buzz and light up');
      }
    });
  }

  /**
   * Clear alert on Arduino (stop speaker + turn off LED)
   */
  clearAlert(arduinoIP?: string): void {
    const targetIP = arduinoIP || this.pairedDevice?.ipAddress;
    
    if (!targetIP) {
      console.error('WARNING: No Arduino IP available. Pair a device first.');
      return;
    }

    console.log(`\nSending CLEAR ALERT (0x00) to ${targetIP}...`);
    
    const buffer = Buffer.from([MessageTypes.NO_ALERT]);
    this.server?.send(buffer, this.SEND_PORT, targetIP, (err) => {
      if (err) {
        console.error('Error clearing alert:', err);
      } else {
        this.alertActive = false;
        console.log('Clear alert sent - Arduino should turn off');
      }
    });
  }

  /**
   * Register callback for status updates
   */
  onStatusUpdate(callback: (status: ArduinoStatus) => void): void {
    this.statusCallback = callback;
  }

  /**
   * Check if alert is currently active
   */
  isAlertActive(): boolean {
    return this.alertActive;
  }

  /**
   * Get currently paired device
   */
  getPairedDevice(): DeviceInfo | null {
    return this.pairedDevice;
  }

  /**
   * Get list of available devices (from backend)
   */
  getAvailableDevices(): DeviceInfo[] {
    return Array.from(this.availableDevices.values());
  }

  /**
   * Update available devices from backend
   */
  updateAvailableDevices(devices: DeviceInfo[]): void {
    this.availableDevices.clear();
    devices.forEach(device => {
      this.availableDevices.set(device.deviceID, device);
    });
  }

  /**
   * Unpair current device
   */
  unpair(): void {
    if (this.pairedDevice) {
      console.log(`Unpaired from device ${this.pairedDevice.deviceID}`);
      this.pairedDevice = null;
    }
  }

  /**
   * Close server
   */
  close(): void {
    this.server?.close();
    console.log('UDP Server closed');
  }
}

// Singleton instance
let serverInstance: ArduinoUDPServer | null = null;

export function getArduinoServer(): ArduinoUDPServer {
  if (!serverInstance) {
    serverInstance = new ArduinoUDPServer();
  }
  return serverInstance;
}
