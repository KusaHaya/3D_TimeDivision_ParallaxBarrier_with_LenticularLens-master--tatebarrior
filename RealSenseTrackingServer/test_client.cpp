// Build with MSVC /EHsc /std:c++14 /DBOOST_ALL_NO_LIB /DBOOST_ERROR_CODE_HEADER_ONLY
// /I <boost root> test_client.cpp ws2_32.lib mswsock.lib
#define NOMINMAX
#include "../ProjectPB/RealSenseTrackingClient.h"
#include <iostream>
#include <limits>
#include <thread>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
int main() {
    try {
        boost::asio::io_service io;
        boost::asio::ip::tcp::acceptor listener(io,
            {boost::asio::ip::address::from_string("127.0.0.1"), 0});
        RealSenseTrackingClient client(listener.local_endpoint().port());
        boost::asio::ip::tcp::socket sender(io);
        listener.accept(sender);
        std::array<float, 6> out{}, valid{{0.03f, 0.02f, 1.f, -0.03f, 0.02f, 1.f}};
        auto pump = [&](int milliseconds) {
            auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
            do { client.Poll(out); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
            while (std::chrono::steady_clock::now() < until);
        };
        boost::asio::write(sender, boost::asio::buffer(valid.data(), 7));
        pump(20);
        require(!client.Poll(out), "Partial TCP packet was accepted");
        boost::asio::write(sender, boost::asio::buffer(reinterpret_cast<char*>(valid.data()) + 7, 17));
        pump(30);
        require(client.Poll(out) && out == valid, "Fragmented packet failed");
        pump(550);
        require(!client.Poll(out), "Stale sample was accepted");
        auto lost = valid;
        lost.fill(std::numeric_limits<float>::quiet_NaN());
        boost::asio::write(sender, boost::asio::buffer(lost));
        pump(20);
        require(!client.Poll(out), "Tracking loss packet was accepted");
        boost::asio::write(sender, boost::asio::buffer(valid));
        pump(20);
        require(client.Poll(out), "Reacquisition failed");
        sender.close();
        pump(1100);
        require(!client.Poll(out), "Disconnect retained valid coordinates");
        listener.accept(sender);
        boost::asio::write(sender, boost::asio::buffer(valid));
        pump(30);
        require(client.Poll(out), "Reconnect failed");
        std::cout << "PASS: fragmentation, timeout, invalid data, recovery, reconnect\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
