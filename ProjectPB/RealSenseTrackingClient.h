#pragma once

#include <boost/asio.hpp>
#include <array>
#include <chrono>
#include <cmath>

// All callbacks run in Poll(), on the renderer's thread. No shared render state
// is written by a network thread. Wire format: six little-endian float32 metres.
class RealSenseTrackingClient {
    using Clock = std::chrono::steady_clock;
    boost::asio::io_service io_;
    boost::asio::ip::tcp::socket socket_{io_};
    boost::asio::steady_timer retry_{io_};
    boost::asio::ip::tcp::endpoint endpoint_;
    std::array<float, 6> incoming_{};
    std::array<float, 6> latest_{};
    Clock::time_point received_{};
    bool valid_ = false;

    void Connect() {
        socket_.async_connect(endpoint_, [this](boost::system::error_code error) {
            if (error) Reconnect();
            else Read();
        });
    }
    void Reconnect() {
        valid_ = false;
        boost::system::error_code ignored;
        socket_.close(ignored);
        retry_.expires_from_now(std::chrono::seconds(1));
        retry_.async_wait([this](boost::system::error_code error) {
            if (!error) Connect();
        });
    }
    void Read() {
        boost::asio::async_read(socket_, boost::asio::buffer(incoming_),
            [this](boost::system::error_code error, std::size_t length) {
                if (error || length != sizeof(incoming_)) { Reconnect(); return; }
                valid_ = true;
                for (float value : incoming_)
                    if (!std::isfinite(value) || std::abs(value) > 4.0f) valid_ = false;
                if (incoming_[2] <= 0.1f || incoming_[5] <= 0.1f) valid_ = false;
                latest_ = incoming_;
                received_ = Clock::now();
                Read();
            });
    }
public:
    explicit RealSenseTrackingClient(unsigned short port = 30000)
        : endpoint_(boost::asio::ip::address::from_string("127.0.0.1"), port) {
        static_assert(sizeof(float) == 4, "Protocol requires float32");
        Connect();
    }
    bool Poll(std::array<float, 6>& coordinates) {
        io_.reset();
        // Bound work per display frame, even if the sender misbehaves.
        for (int i = 0; i < 64 && io_.poll_one(); ++i) {}
        if (!valid_ || Clock::now() - received_ > std::chrono::milliseconds(500))
            return false;
        coordinates = latest_;
        return true;
    }
};
