#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "OscReceiver.h"
#include "../../ProjectPB/OscEyePacket.h"
#include <vector>

namespace projectpb {
int64_t MonotonicNs() {
    LARGE_INTEGER tick, frequency;
    QueryPerformanceCounter(&tick); QueryPerformanceFrequency(&frequency);
    // Match the PINNED runtime's os_time.h, including its integer ns/tick
    // conversion (not a different floating-point/chrono conversion).
    return tick.QuadPart * (1000000000LL / frequency.QuadPart);
}
OscReceiver::OscReceiver(uint16_t port, CameraTransform transform) : buffer_(transform) {
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2,2), &data)) return;
    winsock_ = true;
    socket_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET) return;
    BOOL exclusive = TRUE;
    setsockopt(socket_, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive), sizeof(exclusive));
    sockaddr_in local{}; local.sin_family = AF_INET; local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    local.sin_port = htons(port);
    if (bind(socket_, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == SOCKET_ERROR) return;
    ready_ = true;
    try { worker_ = std::thread(&OscReceiver::Run, this); }
    catch (...) { ready_ = false; }
}
OscReceiver::~OscReceiver() {
    stop_ = true;
    if (worker_.joinable()) worker_.join();
    if (socket_ != INVALID_SOCKET) closesocket(socket_);
    if (winsock_) WSACleanup();
}
void OscReceiver::Run() {
    unsigned char bytes[65536];
    while (!stop_) {
        fd_set set; FD_ZERO(&set); FD_SET(socket_, &set);
        timeval timeout{0, 50000};
        const int selected = select(0, &set, nullptr, nullptr, &timeout);
        if (selected == SOCKET_ERROR) break;
        if (!selected) continue;
        const int n = recv(socket_, reinterpret_cast<char*>(bytes), sizeof(bytes), 0);
        if (n <= 0) continue;
        const auto received = MonotonicNs();
        // Transactional parse: malformed trailing bundle elements must not
        // publish an otherwise valid-looking eye pair from its prefix.
        std::vector<std::pair<std::string, Point>> messages;
        if (!eyeosc::Parse(bytes, static_cast<size_t>(n), [&](const std::string& a, const Point& p) {
                messages.emplace_back(a, p);
            })) continue;
        for (const auto& message : messages) buffer_.Accept(message.first, message.second, received);
    }
    ready_ = false;
}
} // namespace projectpb
