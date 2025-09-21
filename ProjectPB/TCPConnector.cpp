#include <boost/thread.hpp>
#include "TCPConnector.h"

void TCPConnector::OpenAsync()
{
	boost::thread(boost::bind(&boost::asio::io_service::run, m_pIOService.get()));
}

void TCPConnector::WriteAsync(const std::shared_ptr<boost::asio::const_buffers_1>& buffer, const std::function<void(boost::system::error_code, std::size_t)>& callback)
{
	boost::asio::async_write(
		*m_pSocket,
		*buffer,
		[buffer, callback](boost::system::error_code e, size_t length){ callback(e, length); }
	);
}

TCPConnector::TCPConnector() :
	m_bAccepted(false)
{
	m_pIOService = std::make_unique<boost::asio::io_service>();
	m_pIOServiceWork = std::make_unique<boost::asio::io_service::work>(*m_pIOService);
	m_pSocket = std::make_unique<boost::asio::ip::tcp::socket>(*m_pIOService);
}

void TCPConnector::Close()
{
	m_bAccepted = false;
	m_pIOServiceWork = nullptr;
	
	if (m_pIOService)
		m_pIOService->stop();

	m_pSocket = nullptr;
	m_pIOService = nullptr;
}