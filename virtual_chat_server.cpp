#include <algorithm>
#include <boost/bind.hpp>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "udp_receiver.hpp"
#include "udp_sender.hpp"

class VirtualChatServer {
   public:
    VirtualChatServer(int localPort)
        : localEndpoint(boost::asio::ip::udp::v4(), localPort),
          udpReceiver(receiveService, localEndpoint) {}

    void start() {
        udpReceiver.async_receive(
            boost::bind(&VirtualChatServer::handleReceive, this,
                        boost::asio::placeholders::bytes_transferred));

        std::thread sender([this]() {
            boost::asio::io_service::work work(sendService);
            sendService.run();
        });

        std::thread receiver([this]() { receiveService.run(); });

        while (true) {
            std::string command;
            std::cin >> command;

            if (command == "stop") {
                sendService.stop();
                receiveService.stop();
                break;
            }
        }

        sender.join();
        receiver.join();
    }

   private:
    void handleReceive(size_t size) {
        auto it = find_if(
            udpSenders.begin(), udpSenders.end(), [&](const auto& udpSender) {
                return udpSender.remoteEndpoint == udpReceiver.remoteEndpoint;
            });

        if (it == udpSenders.end()) {
            udpSenders.emplace_back(sendService, localEndpoint,
                                    udpReceiver.remoteEndpoint);
        }

        std::string str(udpReceiver.buffer.data(), size);
        std::cout << str << std::endl;

        for (auto& udpSender : udpSenders) {
            udpSender.async_send([](...) {}, str);
        }

        udpReceiver.async_receive(
            boost::bind(&VirtualChatServer::handleReceive, this,
                        boost::asio::placeholders::bytes_transferred));
    }

    boost::asio::io_service sendService;
    boost::asio::io_service receiveService;

    boost::asio::ip::udp::endpoint localEndpoint;

    std::vector<UDPSender> udpSenders;
    UDPReceiver udpReceiver;
};

int main(int argc, char** argv) {
    std::vector<std::string> args(argv, argv + argc);

    VirtualChatServer virtualChatServer(std::stoi(args[1]));
    virtualChatServer.start();

    return 0;
}
