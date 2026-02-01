/**
 * TypeScript types for Arduino SafeWalk communication
 * Simplified protocol: 0xFF = trigger alert, 0x00 = no alert
 */

export interface DeviceInfo {
  deviceID: string;
  ipAddress: string;
  port: number;
  status: 'available' | 'paired' | 'offline';
  lastSeen?: Date;
}

export interface PairingRequest {
  type: 'pair';
  deviceID: string;
}

export interface PairingConfirm {
  type: 'confirm';
  deviceID: string;
  success: boolean;
}

// Message types (must match Arduino defines)
export const MessageTypes = {
  HELLO: 0xAA,
  PAIR_REQUEST: 0xBB,
  PAIR_CONFIRM: 0xCC,
  REQUEST_ALERT: 0xFF,  // All 1s - trigger alert on Arduino
  NO_ALERT: 0x00,       // All 0s - clear alert
  ACKNOWLEDGED: 0x01,   // Volunteer pressed button to acknowledge
} as const;

export interface ArduinoStatus {
  isAlertActive: boolean;
  lastResponseTime: Date;
}
