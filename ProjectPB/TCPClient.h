#pragma once

#include "TCPConnector.h"

class TCPClient : public TCPConnector
{
protected:
	void ConnectAsync(const boost::asio::ip::tcp::endpoint& endpoint);

public:
	TCPClient(const std::string& iPAddress, uint16_t port);
	virtual ~TCPClient() { }
	virtual void Close() { TCPConnector::Close(); }
};