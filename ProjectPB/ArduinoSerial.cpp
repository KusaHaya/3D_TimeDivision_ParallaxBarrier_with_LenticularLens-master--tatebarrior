#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <boost/asio.hpp>
#include <chrono>
#include <stdio.h>
#include "ArduinoSerial.h"

boost::asio::io_service io;
boost::asio::serial_port arduinoSerial(io);
void SendArduinoCommand(int command) {
	if (arduinoSerial.is_open()) {
		unsigned char cmd = static_cast<unsigned char>(command);
		boost::system::error_code ec;
		size_t bytes_written = boost::asio::write(arduinoSerial, boost::asio::buffer(&cmd, 1), ec);
		if (ec || bytes_written != 1) {
			// Sends run every frame; report the first failure, then at most once per second.
			static unsigned failures = 0;
			static bool reported = false;
			static std::chrono::steady_clock::time_point lastReport;
			const auto now = std::chrono::steady_clock::now();
			++failures;
			if (!reported || now - lastReport >= std::chrono::seconds(1)) {
				printf("[ERROR] Arduino send failed: %s (failures: %u)\n", ec.message().c_str(), failures);
				lastReport = now;
				reported = true;
			}
		}
	}
	else {
		printf("[WARN] Arduino serial port is not open.\n");
	}
}

bool IsArduinoOpen()
{
	return arduinoSerial.is_open();
}

bool OpenArduinoSerial(const char* port, std::string& errorMessage)
{
    boost::system::error_code serialError;
    arduinoSerial.open(port, serialError);
    if (!serialError)
        arduinoSerial.set_option(boost::asio::serial_port_base::baud_rate(115200), serialError);
    if (serialError) {
        errorMessage = serialError.message();
        return false;
    }
    return true;
}
