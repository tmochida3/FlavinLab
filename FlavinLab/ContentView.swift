import SwiftUI
import CoreBluetooth
import Combine  // Add this import for ObservableObject

#if canImport(UIKit)
import UIKit
#endif

#if canImport(AppKit)
import AppKit
#endif

// Nordic UART Service UUIDs
let NUS_SERVICE_UUID = CBUUID(string: "6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
let NUS_RX_CHAR_UUID = CBUUID(string: "6E400002-B5A3-F393-E0A9-E50E24DCCA9E")
let NUS_TX_CHAR_UUID = CBUUID(string: "6E400003-B5A3-F393-E0A9-E50E24DCCA9E")

// Bluetooth Manager
class BluetoothManager: NSObject, ObservableObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    @Published var isConnected = false
    @Published var statusMessage = "Not Connected"
    @Published var isScanning = false
    
    private var centralManager: CBCentralManager!
    private var peripheral: CBPeripheral?
    private var rxCharacteristic: CBCharacteristic?
    
    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }
    
    func startScanning() {
        if centralManager.state == .poweredOn {
            centralManager.scanForPeripherals(
                withServices: [NUS_SERVICE_UUID],
                options: [CBCentralManagerScanOptionAllowDuplicatesKey: false]
            )
            statusMessage = "Scanning..."
            isScanning = true
        }
    }
    
    func vibrate() {
        sendCommand("ON")
        // Auto turn off after 500ms
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) { [weak self] in
            self?.sendCommand("OFF")
        }
    }
    
    private func sendCommand(_ command: String) {
        guard let peripheral = peripheral,
              let characteristic = rxCharacteristic,
              let data = command.data(using: .utf8) else {
            print("Not connected or characteristic not found")
            return
        }
        
        peripheral.writeValue(data, for: characteristic, type: .withoutResponse)
        print("Sent command: \(command)")
    }
    
    // MARK: - CBCentralManagerDelegate
    
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        switch central.state {
        case .poweredOn:
            statusMessage = "Bluetooth Ready"
            startScanning()
        case .poweredOff:
            statusMessage = "Bluetooth Off"
            isConnected = false
        case .unsupported:
            statusMessage = "Bluetooth Not Supported"
        default:
            statusMessage = "Bluetooth Unavailable"
        }
    }
    
    func centralManager(_ central: CBCentralManager,
                       didDiscover peripheral: CBPeripheral,
                       advertisementData: [String : Any],
                       rssi RSSI: NSNumber) {
        // Look for device with "VibMotor" name
        if let name = peripheral.name, name.contains("VibMotor") {
            self.peripheral = peripheral
            centralManager.stopScan()
            isScanning = false
            centralManager.connect(peripheral, options: nil)
            statusMessage = "Connecting..."
        }
    }
    
    func centralManager(_ central: CBCentralManager,
                       didConnect peripheral: CBPeripheral) {
        statusMessage = "Connected"
        isConnected = true
        peripheral.delegate = self
        peripheral.discoverServices([NUS_SERVICE_UUID])
    }
    
    func centralManager(_ central: CBCentralManager,
                       didDisconnectPeripheral peripheral: CBPeripheral,
                       error: Error?) {
        statusMessage = "Disconnected"
        isConnected = false
        self.peripheral = nil
        rxCharacteristic = nil
        // Auto-reconnect
        startScanning()
    }
    
    // MARK: - CBPeripheralDelegate
    
    func peripheral(_ peripheral: CBPeripheral,
                   didDiscoverServices error: Error?) {
        guard let services = peripheral.services else { return }
        
        for service in services {
            if service.uuid == NUS_SERVICE_UUID {
                peripheral.discoverCharacteristics([NUS_RX_CHAR_UUID, NUS_TX_CHAR_UUID],
                                                  for: service)
            }
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral,
                   didDiscoverCharacteristicsFor service: CBService,
                   error: Error?) {
        guard let characteristics = service.characteristics else { return }
        
        for characteristic in characteristics {
            if characteristic.uuid == NUS_RX_CHAR_UUID {
                rxCharacteristic = characteristic
                print("Found RX characteristic")
            } else if characteristic.uuid == NUS_TX_CHAR_UUID {
                peripheral.setNotifyValue(true, for: characteristic)
            }
        }
    }
}

// Main App View
struct ContentView: View {
    @StateObject private var bluetoothManager = BluetoothManager()
    @State private var isButtonPressed = false
    
    var body: some View {
        VStack(spacing: 30) {
            // Status Header
            VStack(spacing: 8) {
                Image(systemName: bluetoothManager.isConnected ? "antenna.radiowaves.left.and.right" : "antenna.radiowaves.left.and.right.slash")
                    .font(.system(size: 50))
                    .foregroundColor(bluetoothManager.isConnected ? .green : .gray)
                
                Text(bluetoothManager.statusMessage)
                    .font(.headline)
                    .foregroundColor(.secondary)
            }
            .padding(.top, 40)
            
            Spacer()
            
            // Main Vibrate Button
            Button(action: {
                if bluetoothManager.isConnected {
                    // Haptic feedback on device
                    #if canImport(UIKit)
                    let impactFeedback = UIImpactFeedbackGenerator(style: .medium)
                    impactFeedback.prepare()
                    impactFeedback.impactOccurred()
                    #elseif canImport(AppKit)
                    NSHapticFeedbackManager.defaultPerformer.perform(.alignment, performanceTime: .now)
                    #endif
                    
                    // Trigger motor vibration
                    bluetoothManager.vibrate()
                    
                    // Visual feedback
                    withAnimation(.easeInOut(duration: 0.1)) {
                        isButtonPressed = true
                    }
                    DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                        withAnimation(.easeInOut(duration: 0.1)) {
                            isButtonPressed = false
                        }
                    }
                }
            }) {
                ZStack {
                    Circle()
                        .fill(bluetoothManager.isConnected ? Color.blue : Color.gray)
                        .frame(width: 200, height: 200)
                        .scaleEffect(isButtonPressed ? 0.95 : 1.0)
                        .shadow(color: bluetoothManager.isConnected ? Color.blue.opacity(0.3) : Color.clear,
                               radius: isButtonPressed ? 20 : 10)
                    
                    VStack {
                        Image(systemName: "hand.tap.fill")
                            .font(.system(size: 50))
                            .foregroundColor(.white)
                        
                        Text("VIBRATE")
                            .font(.title2)
                            .fontWeight(.bold)
                            .foregroundColor(.white)
                    }
                }
            }
            .disabled(!bluetoothManager.isConnected)
            
            Spacer()
            
            // Connection Button
            if !bluetoothManager.isConnected && !bluetoothManager.isScanning {
                Button(action: {
                    bluetoothManager.startScanning()
                }) {
                    Text("Scan for Device")
                        .font(.headline)
                        .foregroundColor(.white)
                        .padding()
                        .frame(maxWidth: 200)
                        .background(Color.blue)
                        .cornerRadius(10)
                }
                .padding(.bottom, 30)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        #if canImport(UIKit)
        .background(Color(UIColor.systemBackground))
        #elseif canImport(AppKit)
        .background(Color(NSColor.windowBackgroundColor))
        #else
        .background(Color.black)
        #endif
    }
}
