#include <winsock2.h>
#include <ws2tcpip.h>
#include "ExternalEyeTracker.h"
#include "OscEyePacket.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>

struct ExternalEyeTracker::Impl {
    using Clock = std::chrono::steady_clock;
    std::atomic<bool> stop{false};
    std::thread worker;
    std::mutex mutex;
    SOCKET socket = INVALID_SOCKET;
    bool winsock = false, valid = false;
    std::array<float, 6> latest{};
    std::array<std::array<float, 3>, 2> eyes{};
    std::array<Clock::time_point, 2> times{};
    std::array<bool, 2> dirty{};
    float xSign = -1, ySign = -1;
    void Run() {
        unsigned char packet[65536];
        auto lastSample = Clock::now(), lastReport = Clock::now();
        unsigned samples = 0;
        bool announced = false, holding = false;
        while (!stop) {
            fd_set ready; FD_ZERO(&ready); FD_SET(socket, &ready);
            timeval timeout{0, 50000};
            const int result = select(0, &ready, nullptr, nullptr, &timeout);
            if (result == SOCKET_ERROR) { std::cerr << "OSC receive error: " << WSAGetLastError() << '\n'; break; }
            if (result > 0) {
                const int length = recv(socket, reinterpret_cast<char*>(packet), sizeof(packet), 0);
                if (length > 0) eyeosc::Parse(packet, size_t(length), [&](const std::string& address, const std::array<float, 3>& xyz) {
                    int eye = address == "/eye/right" ? 0 : address == "/eye/left" ? 1 : -1;
                    // centerは補助データ。左右の目が揃った測定だけを表示側に渡す。
                    if (eye < 0 || xyz[2] < .1f || xyz[2] > 10.f) return;
                    for (float value : xyz) if (std::abs(value) > 10.f) return;
                    const auto now = Clock::now();
                    eyes[eye] = xyz; times[eye] = now; dirty[eye] = true;
                    if (!dirty[0] || !dirty[1]) return;
                    if (std::abs(std::chrono::duration<double>(times[0] - times[1]).count()) > .1) return;
                    std::array<float, 6> coordinates;
                    for (int k = 0; k < 2; ++k) {
                        // RealSenseカメラ座標(X右/Y下/Z前、m)を表示側(X左/Y上/Z前)に変換。
                        coordinates[3*k] = xSign * eyes[k][0];
                        coordinates[3*k+1] = ySign * eyes[k][1];
                        coordinates[3*k+2] = eyes[k][2];
                    }
                    {
                        std::lock_guard<std::mutex> lock(mutex);
                        latest = coordinates; valid = true;
                    }
                    dirty = {{false, false}}; lastSample = now; ++samples; holding = false;
                    if (!announced) {
                        std::cout << "External eye tracking: receiving both eyes (metres). Face camera, then press h.\n";
                        announced = true;
                    }
                    if (now - lastReport > std::chrono::seconds(2)) {
                        std::cout << "OSC eye pairs/sec=" << samples / std::chrono::duration<double>(now-lastReport).count()
                                  << " center_xyz_m=" << (coordinates[0]+coordinates[3])*.5f << ','
                                  << (coordinates[1]+coordinates[4])*.5f << ',' << (coordinates[2]+coordinates[5])*.5f << '\n';
                        samples = 0; lastReport = now;
                    }
                });
            }
            if (!holding && Clock::now() - lastSample > std::chrono::seconds(1)) {
                std::cout << (announced ? "OSC gap: holding last eye position.\n" : "OSC waiting: start EyeTracker-RS with destination 127.0.0.1:8000.\n");
                holding = true;
            }
        }
    }
};
ExternalEyeTracker::ExternalEyeTracker() : impl_(new Impl) {}
ExternalEyeTracker::~ExternalEyeTracker() { Stop(); }
bool ExternalEyeTracker::Start() {
    Stop();
    unsigned port = 8000;
    if (const char* text = std::getenv("PROJECTPB_OSC_PORT")) {
        char* end; const long parsed = std::strtol(text, &end, 10);
        if (*text && !*end && parsed > 0 && parsed <= 65535) port = unsigned(parsed);
        else { std::cerr << "Invalid PROJECTPB_OSC_PORT.\n"; return false; }
    }
    if (const char* text = std::getenv("PROJECTPB_OSC_X_SIGN")) impl_->xSign = std::atoi(text) == 1 ? 1.f : -1.f;
    if (const char* text = std::getenv("PROJECTPB_OSC_Y_SIGN")) impl_->ySign = std::atoi(text) == 1 ? 1.f : -1.f;
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data)) return false;
    impl_->winsock = true;
    impl_->socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in local{}; local.sin_family = AF_INET; local.sin_addr.s_addr = htonl(INADDR_LOOPBACK); local.sin_port = htons(static_cast<u_short>(port));
    if (impl_->socket == INVALID_SOCKET || bind(impl_->socket, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == SOCKET_ERROR) {
        std::cerr << "Cannot bind OSC port " << port << ": " << WSAGetLastError() << ". Close another receiver using this port.\n";
        Stop(); return false;
    }
    impl_->stop = false; impl_->dirty = {{false, false}};
    try { impl_->worker = std::thread(&Impl::Run, impl_.get()); }
    catch (...) { Stop(); return false; }
    std::cout << "External OSC tracking listening on 127.0.0.1:" << port << " (/eye/right, /eye/left).\n";
    return true;
}
void ExternalEyeTracker::Stop() {
    impl_->stop = true;
    if (impl_->worker.joinable()) impl_->worker.join();
    if (impl_->socket != INVALID_SOCKET) { closesocket(impl_->socket); impl_->socket = INVALID_SOCKET; }
    if (impl_->winsock) { WSACleanup(); impl_->winsock = false; }
    std::lock_guard<std::mutex> lock(impl_->mutex); impl_->valid = false;
}
bool ExternalEyeTracker::Poll(std::array<float, 6>& coordinates) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid) return false;
    coordinates = impl_->latest; return true;
}
