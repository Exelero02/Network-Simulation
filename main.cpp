#include <iostream>
#include <queue>
#include <thread>
#include <random>
#include <condition_variable>
#include <mutex>
#include <stdexcept>

using std::cout;
using std::endl;

// Mutex to prevent simultaneous output from multiple threads
std::mutex coutMutex;

enum class PacketType {
    DATA,
    ACK
};

struct Packet {
    std::string data;
    int sequenceNumber;
    PacketType type;
};


class Node {
private:
    std::string name;
    std::queue<Packet> buffer;    // Buffer for storing received packets
    std::mutex bufferMutex;       // Mutex for synchronizing access to the buffer
    std::condition_variable bufferCV;  // Condition variable to notify packet processing
    std::mt19937 &gen;            // Random number generator
    bool running = true;          // Flag to control the node's operation

    static constexpr int MAX_RETRIES = 3;  // Maximum number of retries for sending a packet

public:
    explicit Node(std::string nodeName, std::mt19937 &genRef) :
            name(std::move(nodeName)), gen(genRef) {}

    void send(const Packet &packet, Node &receiver) {
        int retryCount = 0;

        while (retryCount < MAX_RETRIES) {
            {
                std::lock_guard<std::mutex> coutLock(coutMutex);
                cout << name << " is sending " << (packet.type == PacketType::DATA ? "DATA" : "ACK")
                     << " packet: " << packet.data
                     << " (Seq: " << packet.sequenceNumber << ") to "
                     << receiver.getName() << " (Attempt " << retryCount + 1 << ")" << endl;
            }

            // Simulate random network delay between 500ms and 1000ms
            std::uniform_int_distribution<> delay(500, 1000);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay(gen)));

            try {
                receiver.receive(packet, *this);
                return;  // Exit the retry loop
            } catch (const std::exception &e) {
                // Log the failure and increment retry count if an error occurs
                std::lock_guard<std::mutex> coutLock(coutMutex);
                cout << name << " failed to send packet to " << receiver.getName()
                     << ": " << e.what() << endl;
                retryCount++;
            }
        }

        // Log failure after MAX_RETRIES attempts
        std::lock_guard<std::mutex> coutLock(coutMutex);
        cout << name << " failed to send packet (Seq: " << packet.sequenceNumber
             << ") to " << receiver.getName() << " after " << MAX_RETRIES << " attempts." << endl;

        throw std::runtime_error("Failed to send packet after maximum retries");
    }

    void receive(const Packet &packet, Node &sender) {

        // Simulate packet loss with a 10% probability
        std::uniform_real_distribution<> lossProbability(0.0, 1.0);
        if (lossProbability(gen) < 0.1) {

            // Log and drop the packet if it's lost
            std::lock_guard<std::mutex> coutLock(coutMutex);
            cout << name << " dropped packet: " << packet.data
                 << " (Seq: " << packet.sequenceNumber << ")" << endl;
            return;  // Packet is lost, return without processing
        }

        Packet receivedPacket = packet;

        // Simulate packet corruption with a 5% probability
        std::uniform_real_distribution<> corruptionProbability(0.0, 1.0);
        if (corruptionProbability(gen) < 0.05) {
            receivedPacket.data = "CORRUPTED";  // Simulate packet corruption

            // Log the corrupted packet
            std::lock_guard<std::mutex> coutLock(coutMutex);
            cout << name << " received corrupted packet: " << receivedPacket.data
                 << " (Seq: " << receivedPacket.sequenceNumber << ")" << endl;
        }

        {
            std::unique_lock<std::mutex> lock(bufferMutex);
            buffer.push(receivedPacket);
        }
        bufferCV.notify_one();

        {
            std::lock_guard<std::mutex> coutLock(coutMutex);
            cout << name << " received " << (receivedPacket.type == PacketType::DATA ? "DATA" : "ACK")
                 << " packet: " << receivedPacket.data
                 << " (Seq: " << receivedPacket.sequenceNumber << ")" << endl;
        }

        if (receivedPacket.type == PacketType::DATA) {
            Packet ackPacket{"ACK", receivedPacket.sequenceNumber, PacketType::ACK};
            send(ackPacket, sender);
        }
    }

    void processPackets() {
        while (running) {
            try {
                std::unique_lock<std::mutex> lock(bufferMutex);
                bufferCV.wait(lock, [this] { return !buffer.empty() || !running; });

                if (!running) break;

                while (!buffer.empty()) {
                    Packet packet = buffer.front();
                    buffer.pop();

                    lock.unlock();
                    {
                        std::lock_guard<std::mutex> coutLock(coutMutex);
                        cout << name << " is processing packet: " << packet.data
                             << " (Seq: " << packet.sequenceNumber << ")" << endl;
                    }
                    lock.lock();
                }
            } catch (const std::exception &e) {
                std::lock_guard<std::mutex> coutLock(coutMutex);
                cout << name << " encountered an error while processing packets: " << e.what() << endl;
                if (std::string(e.what()) == "Fatal error") {
                    running = false;
                }
            }
        }

        {
            std::lock_guard<std::mutex> coutLock(coutMutex);
            cout << name << " is stopping packet processing." << endl;
        }
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(bufferMutex);
            running = false;  // Set running flag to false
        }
        bufferCV.notify_all();
    }

    [[nodiscard]] std::string getName() const {
        return name;
    }
};

int main() {
    try {
        std::random_device randomDevice;
        std::mt19937 gen(randomDevice());

        Node nodeA("Node A", gen);
        Node nodeB("Node B", gen);

        std::thread nodeBThread(&Node::processPackets, &nodeB);

        Packet packet1{"Test", 1, PacketType::DATA};
        Packet packet2{"Packet", 2, PacketType::DATA};

        try {
            nodeA.send(packet1, nodeB);
            nodeA.send(packet2, nodeB);
        } catch (const std::exception &e) {
            std::cerr << "Failed to send packet: " << e.what() << endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));

        nodeB.stop();
        nodeBThread.join();
    } catch (const std::exception &e) {
        std::cerr << "An error occurred in the main function: " << e.what() << endl;
        return 1;
    }

    return 0;
}