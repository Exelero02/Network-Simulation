#include <iostream>
#include <queue>
#include <thread>
#include <random>
#include <condition_variable>
#include <mutex>
#include <stdexcept>

using std::cout;
using std::endl;

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
    std::queue<Packet> buffer;
    std::mutex bufferMutex;
    std::condition_variable bufferCV;
    std::mt19937 &gen;
    bool running = true;

    static constexpr int MAX_RETRIES = 3;

public:
    explicit Node(std::string nodeName, std::mt19937 &genRef) :
            name(std::move(nodeName)), gen(genRef) {}

    void send(const Packet &packet, Node &receiver) {
        int retryCount = 0;

        while (retryCount < MAX_RETRIES) {
            {
                std::lock_guard<std::mutex> coutLock(coutMutex);
                cout << name << " is sending " << (packet.type == PacketType::DATA ? "DATA" : "ACK")
                     << " packet: "
                     << packet.data
                     << " (Seq: " << packet.sequenceNumber << ") to "
                     << receiver.getName() << " (Attempt " << retryCount + 1 << ")" << endl;
            }

            std::uniform_int_distribution<> delay(500, 1000);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay(gen)));

            try {
                receiver.receive(packet, *this);
                return; // Success, exit the retry loop
            } catch (const std::exception &e) {
                std::lock_guard<std::mutex> coutLock(coutMutex);
                cout << name << " failed to send packet to " << receiver.getName()
                     << ": " << e.what() << endl;
                retryCount++;
            }
        }

        std::lock_guard<std::mutex> coutLock(coutMutex);
        cout << name << " failed to send packet (Seq: " << packet.sequenceNumber
             << ") to " << receiver.getName() << " after " << MAX_RETRIES << " attempts." << endl;

        throw std::runtime_error("Failed to send packet after maximum retries");
    }

    void receive(const Packet &packet, Node &sender) {
        {
            std::unique_lock<std::mutex> lock(bufferMutex);
            buffer.push(packet);
        }
        bufferCV.notify_one();

        {
            std::lock_guard<std::mutex> coutLock(coutMutex);
            cout << name << " received " << (packet.type == PacketType::DATA ? "DATA" : "ACK")
                 << " packet: "
                 << packet.data
                 << " (Seq: " << packet.sequenceNumber << ")" << endl;
        }

        if (packet.type == PacketType::DATA) {
            Packet ackPacket{"ACK", packet.sequenceNumber, PacketType::ACK};
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
            running = false;
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