# SafeWalk Frontend - Arduino Communication

This directory contains the TypeScript/React frontend mock code for communicating with Arduino devices via UDP.

## Architecture

```
┌─────────────┐         UDP          ┌──────────────┐
│   Arduino   │ ◄─────────────────► │   Frontend   │
│  (Embedded) │  Port 8888/8889     │  (React/TS)  │
└─────────────┘                      └──────────────┘
       │                                     │
       │ HTTP Registration                   │ HTTP Query
       ▼                                     ▼
┌────────────────────────────────────────────────────┐
│                  Backend (Go)                       │
│        Device Registry & Pairing Service            │
└────────────────────────────────────────────────────┘
```

## Files

- **`types/arduino.types.ts`** - TypeScript types for Arduino packets and device info
- **`services/udpServer.ts`** - UDP server for receiving/sending Arduino data
- **`services/pairingService.ts`** - Service for device discovery and pairing
- **`components/PairingUI.tsx`** - React component for pairing UI
- **`test/testConnection.ts`** - Test script to verify Arduino connection

## Setup

### 1. Install Dependencies

```bash
cd frontend
npm install
```

### 2. Configuration

Update the backend URL in `pairingService.ts` if needed:

```typescript
const pairingService = new PairingService('http://YOUR_BACKEND_IP:3001');
```

### 3. Arduino Configuration

Make sure your Arduino `wifiConfig.h` has:

```cpp
const char BACKEND_IP[] = "YOUR_FRONTEND_IP";
const int BACKEND_PORT = 3001;
```

## Usage

### Test Connection Script

Run the test script to verify Arduino communication:

```bash
npm run test:connection
```

This will:
1. Start UDP server on port 8888
2. Listen for Arduino HELLO messages
3. Display received packets
4. Send ACK responses

### Expected Flow

1. **Arduino boots up:**
   - Connects to WiFi
   - Gets IP address (e.g., 192.168.1.100)
   - Sends registration to backend
   - Waits for pairing

2. **Frontend loads:**
   - Queries backend for available devices
   - Displays list in PairingUI component
   
3. **User pairs device:**
   - Selects device from list
   - Frontend sends PAIR_REQUEST to Arduino
   - Arduino confirms pairing
   - LED lights up + speaker alert
   
4. **Arduino sends HELLO:**
   - First communication after pairing
   - Frontend receives and logs packet
   - Frontend sends ACK back
   
5. **Ongoing communication:**
   - Arduino streams joystick/button data
   - Frontend receives and processes
   - Frontend sends game state ACKs

## Message Protocol (SIMPLIFIED)

### Operational Messages (Main Protocol)

**SafeWalk Alert Request (Frontend → Arduino):**
```
[0xFF]  // All 1s = trigger alert (LED ON, Speaker ON)
```

**Clear Alert (Frontend → Arduino):**
```
[0x00]  // All 0s = clear alert (LED OFF, Speaker OFF)
```

**Status Response (Arduino → Frontend):**
```
[0xFF]  // Alert is active
[0x00]  // Alert is cleared
```

### Pairing Messages (One-time Setup)

**Pair Request (Frontend → Arduino):**
```
[0xBB]
```

**Pair Confirm (Arduino → Frontend):**
```
[0xCC] + [device_id]
```

**Hello (Arduino → Frontend):**
```
[0xAA]
```

That's the entire protocol! Simple and effective. ✨

## Integration with React App

```typescript
import { getArduinoServer } from './services/udpServer';
import { PairingUI } from './components/PairingUI';

function App() {
  const arduinoServer = getArduinoServer();
  const [requestActive, setRequestActive] = useState(false);
  
  // Handle status updates from Arduino
  arduinoServer.onStatusUpdate((status) => {
    console.log('Alert active:', status.isAlertActive);
    setRequestActive(status.isAlertActive);
  });
  
  // Send SafeWalk request
  const handleSafeWalkRequest = () => {
    arduinoServer.sendAlertRequest();  // Sends 0xFF
  };
  
  // Clear alert when volunteer accepts
  const handleAcceptRequest = () => {
    arduinoServer.clearAlert();  // Sends 0x00
  };
  
  return (
    <div>
      <PairingUI />
      
      {!requestActive ? (
        <button onClick={handleSafeWalkRequest}>
          🚨 Request SafeWalk
        </button>
      ) : (
        <button onClick={handleAcceptRequest}>
          ✓ Accept Request
        </button>
      )}
    </div>
  );
}
```

## Troubleshooting

### Arduino not appearing in device list

- Check Arduino Serial Monitor for WiFi connection status
- Verify backend is receiving registration packets
- Check firewall settings (UDP ports 8888/8889)

### Pairing fails

- Verify Arduino and Frontend are on same network
- Check IP addresses in configuration
- Look for error messages in Arduino Serial Monitor

### No HELLO message received

- Check UDP ports are correct (8888/8889)
- Verify pairing completed successfully
- Check Arduino Serial output for sending confirmation

## Next Steps

1. **Implement Go backend** with device registry
2. **Add WebSocket support** for browser-based frontend
3. **Add reconnection logic** for dropped connections
4. **Implement heartbeat** to detect offline devices
5. **Add encryption** for secure communication
