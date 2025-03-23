# Packet Transmission Simulation

## Overview
The **Packet Transmission Simulation** project simulates a basic communication protocol between two nodes, with error handling mechanisms such as packet loss, corruption, and retransmission. The simulation involves sending and receiving packets between nodes in a multithreaded environment, providing an insight into how network protocols might behave under conditions like packet loss and corruption.

---

## Features
- **Packet Loss Simulation**: Simulates 10% packet loss during transmission.
- **Packet Corruption Simulation**: Simulates 5% packet corruption during transmission.
- **Retransmission**: Retries sending the packet up to three times before failure.
- **Multithreading**: Utilizes threads to process packet transmission and reception concurrently.
- **Synchronization**: Implements thread synchronization using mutexes and condition variables to manage shared resources.
- **Error Handling**: Handles runtime errors during transmission and processing.

---

## System Architecture

### Key Components:
1. **Node Class**: Represents each communication endpoint.
   - Sends and receives packets.
   - Handles packet loss, corruption, and retries.
   - Processes incoming packets.

2. **Packet Class**: Represents a data packet.
   - Contains packet type (DATA or ACK), data, and sequence number.

3. **Main Program**: Initializes two nodes, simulates the sending and receiving of packets, and handles errors or retries if necessary.

---

## Classes and Methods

### Node Class
The `Node` class is responsible for sending and receiving packets between two nodes.

#### Methods:
- **send(const Packet &packet, Node &receiver)**: Sends a packet to another node. Retries up to 3 times if the transmission fails.
- **receive(const Packet &packet, Node &sender)**: Receives a packet, simulates packet loss or corruption, and processes it.
- **processPackets()**: Processes received packets, handling them as needed.
- **stop()**: Stops packet processing for the node.

### Packet Class
The `Packet` class represents a packet that is transmitted between nodes.

#### Attributes:
- `data`: The content of the packet.
- `sequenceNumber`: A unique identifier for each packet.
- `type`: The type of packet (DATA or ACK).

---

## Error Handling

The simulation includes error handling for the following scenarios:
- **Packet Loss**: A random probability (10%) is introduced to simulate packet loss during transmission.
- **Packet Corruption**: A random probability (5%) is used to simulate packet corruption.
- **Transmission Failure**: If a packet fails to be sent after 3 retries, an error message is displayed.

---

## Main Program Flow

1. **Initialization**: The main program initializes two nodes, `nodeA` and `nodeB`, each associated with a random number generator.
2. **Packet Creation**: Two packets (`packet1` and `packet2`) are created with sequence numbers and data.
3. **Sending Packets**: `nodeA` sends the packets to `nodeB`. Each packet is sent with retries if necessary.
4. **Packet Processing**: `nodeB` processes the received packets and sends acknowledgment packets back to `nodeA` upon successful receipt of data packets.
5. **Termination**: After processing the packets, `nodeB` stops its packet processing and the program terminates.

---

## Installation and Setup

To run the simulation, follow these steps:

```sh
g++ main.cpp -o packet_transmission -std=c++11 -pthread
```
```sh
./packet_transmission
```
