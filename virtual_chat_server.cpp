#include <thread>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include "udp_sender.hpp"
#include "udp_receiver.hpp"

boost::asio::io_service sendService;
boost::asio::io_service receiveService;
boost::asio::ip::udp::endpoint localEndpoint(boost::asio::ip::udp::v4(), 11999);

std::vector<std::unique_ptr<UDPSender>> udpSenders;
auto udpReceiver = std::make_unique<UDPReceiver>(receiveService, localEndpoint);

void handler(const boost::system::error_code& error, size_t size)
{
    auto it = find_if(udpSenders.begin(), udpSenders.end(), [&](const auto& udpSender)
    {
        return udpSender->remoteEndpoint == udpReceiver->remoteEndpoint;
    });

    if (it == udpSenders.end())
    {
        udpSenders.emplace_back(std::make_unique<UDPSender>(sendService, localEndpoint, udpReceiver->remoteEndpoint));
    }

    std::string str(udpReceiver->buffer.data(), size);
    std::cout << str << std::endl;

    for (auto& udpSender : udpSenders)
    {
        udpSender->async_send([](...){}, str);
    }

    udpReceiver->async_receive(handler);
}

int main()
{
    udpReceiver->async_receive(handler);

    std::thread sender([]() 
    { 
        boost::asio::io_service::work work(sendService);
        sendService.run(); 
    });

    std::thread receiver([]() 
    { 
        receiveService.run();
    });

    sender.join();
    receiver.join();

    return 0;
}
