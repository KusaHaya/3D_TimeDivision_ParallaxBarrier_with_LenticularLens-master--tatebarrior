#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <boost/asio.hpp>

class TCPConnector
{
protected:
	std::unique_ptr<boost::asio::ip::tcp::socket>	m_pSocket;
	std::unique_ptr<boost::asio::io_service>		m_pIOService;
	std::unique_ptr<boost::asio::io_service::work>	m_pIOServiceWork;
	bool											m_bAccepted;

	void OpenAsync();
	void WriteAsync(const std::shared_ptr<boost::asio::const_buffers_1>& buffer, const std::function<void(boost::system::error_code, std::size_t)>& callback);
	template<typename MutableBufferSequence>
	void ReadAsync(std::shared_ptr<MutableBufferSequence>& buffer, const std::function<void(boost::system::error_code, std::size_t)>& callback)
	{
		boost::asio::async_read(
			*m_pSocket,
			*buffer,
			boost::asio::transfer_exactly(sizeof(float)*6),
			[buffer, callback](boost::system::error_code error, std::size_t length){ callback(error, length); }
		);
	}
public:
	TCPConnector();
	virtual ~TCPConnector() { Close(); }
	void SendAsync(const std::shared_ptr<boost::asio::const_buffers_1>& buffer, const std::function<void(boost::system::error_code, std::size_t)>& callback) { if (m_bAccepted) WriteAsync(buffer, callback); }
	template<typename MutableBufferSequence>
	void RecieveAsync(std::shared_ptr<MutableBufferSequence>& buffer, const std::function<void(boost::system::error_code, std::size_t)>& callback) { if (m_bAccepted) ReadAsync(buffer, callback); }
	virtual void Close();
};