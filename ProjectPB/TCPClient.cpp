#include "TCPClient.h"

void TCPClient::ConnectAsync(const boost::asio::ip::tcp::endpoint& endpoint)
{
	auto itr = boost::asio::ip::tcp::resolver(*m_pIOService).resolve(endpoint);

	boost::asio::async_connect(
		*m_pSocket,
		itr,
		[&](boost::system::error_code error, boost::asio::ip::tcp::endpoint  ep)
	{
		if (!error)
			m_bAccepted = true;
		else
			ConnectAsync(ep);
	});
}

TCPClient::TCPClient(const std::string& iPAddress, uint16_t port)
{
	const boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(iPAddress), port);

	ConnectAsync(endpoint);
	OpenAsync();
}