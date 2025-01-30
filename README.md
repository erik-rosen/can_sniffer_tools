# CAN Bus Change Monitoring Tool

## Overview
This Arduino-based CAN Bus monitoring tool is very nifty to reverse engineer CAN messages. It observes changes in CAN message data across three phases:

1. **Baseline Phase**: Collects message bits for each CAN ID that change during the recording. A mask is produced to ignore these bits in subsequent phases.
2. **Modification Phase**: Applies the baseline mask to messages to ignore bits that changed in baseline phase. This phase identifies bits that change between messages and records them in a modification mask. It is in this phase where you would press buttons or change the state of your vehicle to find the corresponding CAN ID and bits. 
3. **Monitoring Phase**: Continuously monitors and reports changes to bits that were identified as modifiable during the modification phase, ignoring the bits that changed during the baseline phase.

This tool is designed for use with the Arduino Due and the `due_can` library.

## Features
- Uses bitwise masking to detect changes in individual bits rather than entire bytes.
- Serial prints provide real-time feedback on detected modifications.
- Summarizes all modified bits at the end of the modification phase.

## Hardware Requirements
- **Arduino Due** (or any board with two CAN controllers and compatible software support)
- **CAN Bus Transceiver Module**
- **CAN Bus Network**

## Software Requirements
- **Arduino IDE**
- **due_can** library (Install via Arduino Library Manager)

## Installation
1. Clone or download the source code.
2. Open the sketch in the Arduino IDE.
3. Install the `due_can` library if not already installed.
4. Connect the hardware components properly (if connecting to a car, the ODB2 port typically has CANL and CANH pins).
5. Compile and upload the sketch to the Arduino Due.

## How It Works
### 1. Baseline Phase
- The program collects data from all incoming CAN messages.
- It applies a bitwise AND across all received messages for each CAN ID.
- The resulting value represents stable bits.

### 2. Modification Phase
- The program tracks differences between new messages and the baseline.
- A modification mask is created for each CAN ID, capturing bits that change.
- At the end of this phase, the modification masks are printed to Serial.

### 3. Monitoring Phase
- The program monitors incoming messages and checks for changes in the bits marked in the modification mask.
- If a monitored bit changes, it is reported in real-time.

## Example Output
```
Entering Baseline Phase: Recording stable bits
Entering Modification Phase: Detecting changing bits
Modified bits during Modification Phase:
ID: 0x1 Byte[3] Bit[4]
Entering Monitoring Phase: Printing detected changes
ID: 0x1 Byte[3] Bit[4] changed
```

## Troubleshooting
- Ensure the correct baud rate is set for your CAN network.
- Verify proper connections between the Arduino Due and CAN transceiver.
- If no messages are detected, confirm that the CAN network is active.


