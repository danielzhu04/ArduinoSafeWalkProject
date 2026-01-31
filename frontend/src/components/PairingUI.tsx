/**
 * Mock React Component for Device Pairing UI
 */

import React, { useState, useEffect } from 'react';
import { DeviceInfo } from '../types/arduino.types';
import { getPairingService } from '../services/pairingService';
import { getArduinoServer } from '../services/udpServer';

export const PairingUI: React.FC = () => {
  const [availableDevices, setAvailableDevices] = useState<DeviceInfo[]>([]);
  const [pairedDevice, setPairedDevice] = useState<DeviceInfo | null>(null);
  const [loading, setLoading] = useState(false);
  const [status, setStatus] = useState<string>('Ready to pair');
  const [alertActive, setAlertActive] = useState(false);

  const pairingService = getPairingService();
  const arduinoServer = getArduinoServer();

  /**
   * Load available devices on mount
   */
  useEffect(() => {
    loadAvailableDevices();
    
    // Set up status update handler
    arduinoServer.onStatusUpdate((arduinoStatus) => {
      setAlertActive(arduinoStatus.isAlertActive);
    });
    
    // Poll for devices every 5 seconds
    const interval = setInterval(loadAvailableDevices, 5000);
    
    return () => clearInterval(interval);
  }, []);

  /**
   * Fetch available devices from backend
   */
  const loadAvailableDevices = async () => {
    try {
      const devices = await pairingService.getAvailableDevices();
      setAvailableDevices(devices);
      arduinoServer.updateAvailableDevices(devices);
    } catch (error) {
      console.error('Failed to load devices:', error);
      setStatus('Error loading devices');
    }
  };

  /**
   * Handle pairing with a device
   */
  const handlePair = async (device: DeviceInfo) => {
    setLoading(true);
    setStatus(`Pairing with ${device.deviceID}...`);

    try {
      // Send pairing request to Arduino via UDP
      const success = await arduinoServer.sendPairingRequest(device.ipAddress);

      if (success) {
        const paired = arduinoServer.getPairedDevice();
        setPairedDevice(paired);
        setStatus('Successfully paired!');

        // Register pairing with backend
        const localIP = await pairingService.getLocalIP();
        await pairingService.registerPairing(device.deviceID, localIP);

        console.log('🎉 Pairing complete! Ready to receive data.');
      } else {
        setStatus('Pairing failed. Please try again.');
      }
    } catch (error) {
      console.error('Pairing error:', error);
      setStatus('Pairing error occurred');
    } finally {
      setLoading(false);
    }
  };

  /**
   * Handle unpairing
   */
  const handleUnpair = async () => {
    if (!pairedDevice) return;

    await pairingService.unregisterPairing(pairedDevice.deviceID);
    arduinoServer.unpair();
    setPairedDevice(null);
    setAlertActive(false);
    setStatus('Device unpaired');
    loadAvailableDevices();
  };

  /**
   * Trigger alert on Arduino (for testing SafeWalk requests)
   */
  const handleTriggerAlert = () => {
    arduinoServer.sendAlertRequest();
    setStatus('Alert sent to volunteer device');
  };

  /**
   * Clear alert on Arduino
   */
  const handleClearAlert = () => {
    arduinoServer.clearAlert();
    setStatus('Alert cleared');
  };

  return (
    <div className="pairing-ui" style={{ padding: '20px', fontFamily: 'sans-serif' }}>
      <h2>Arduino Device Pairing</h2>
      
      <div className="status" style={{ 
        padding: '10px', 
        margin: '10px 0', 
        backgroundColor: '#f0f0f0',
        borderRadius: '5px' 
      }}>
        Status: {status}
      </div>

      {pairedDevice ? (
        <div className="paired-device" style={{ 
          padding: '15px', 
          border: '2px solid #4CAF50',
          borderRadius: '5px',
          marginTop: '20px'
        }}>
          <h3>Paired Device</h3>
          <p><strong>Device ID:</strong> {pairedDevice.deviceID}</p>
          <p><strong>IP Address:</strong> {pairedDevice.ipAddress}</p>
          <p><strong>Status:</strong> {pairedDevice.status}</p>
          <p><strong>Alert Status:</strong> {alertActive ? 'Active' : 'Inactive'}</p>
          
          <div style={{ marginTop: '15px' }}>
            <button 
              onClick={handleTriggerAlert}
              disabled={alertActive}
              style={{
                padding: '10px 20px',
                backgroundColor: alertActive ? '#ccc' : '#FF9800',
                color: 'white',
                border: 'none',
                borderRadius: '5px',
                cursor: alertActive ? 'not-allowed' : 'pointer',
                marginRight: '10px'
              }}
            >
              Send Safe Walk Request
            </button>
            
            <button 
              onClick={handleClearAlert}
              disabled={!alertActive}
              style={{
                padding: '10px 20px',
                backgroundColor: !alertActive ? '#ccc' : '#4CAF50',
                color: 'white',
                border: 'none',
                borderRadius: '5px',
                cursor: !alertActive ? 'not-allowed' : 'pointer',
                marginRight: '10px'
              }}
            >
              Clear Request
            </button>
          </div>

          <button 
            onClick={handleUnpair}
            style={{
              padding: '10px 20px',
              backgroundColor: '#f44336',
              color: 'white',
              border: 'none',
              borderRadius: '5px',
              cursor: 'pointer',
              marginTop: '15px'
            }}
          >
            Unpair Device
          </button>
        </div>
      ) : (
        <div className="available-devices">
          <h3>Available Devices</h3>
          
          {availableDevices.length === 0 ? (
            <p style={{ color: '#666' }}>
              No devices found. Make sure Arduino is powered on and connected to WiFi.
            </p>
          ) : (
            <div style={{ marginTop: '15px' }}>
              {availableDevices.map((device) => (
                <div 
                  key={device.deviceID}
                  style={{
                    padding: '15px',
                    border: '1px solid #ddd',
                    borderRadius: '5px',
                    marginBottom: '10px',
                    display: 'flex',
                    justifyContent: 'space-between',
                    alignItems: 'center'
                  }}
                >
                  <div>
                    <div><strong>Device ID:</strong> {device.deviceID}</div>
                    <div><strong>IP:</strong> {device.ipAddress}</div>
                    <div><strong>Status:</strong> <span style={{ color: '#4CAF50' }}>{device.status}</span></div>
                  </div>
                  <button
                    onClick={() => handlePair(device)}
                    disabled={loading}
                    style={{
                      padding: '10px 20px',
                      backgroundColor: loading ? '#ccc' : '#2196F3',
                      color: 'white',
                      border: 'none',
                      borderRadius: '5px',
                      cursor: loading ? 'not-allowed' : 'pointer'
                    }}
                  >
                    {loading ? 'Pairing...' : 'Pair'}
                  </button>
                </div>
              ))}
            </div>
          )}

          <button
            onClick={loadAvailableDevices}
            disabled={loading}
            style={{
              padding: '10px 20px',
              backgroundColor: '#757575',
              color: 'white',
              border: 'none',
              borderRadius: '5px',
              cursor: 'pointer',
              marginTop: '15px'
            }}
          >
            Refresh Devices
          </button>
        </div>
      )}
    </div>
  );
};

export default PairingUI;
