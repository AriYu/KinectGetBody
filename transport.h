#ifndef TRANSPORT_H_
#define TRANSPORT_H_

#include <iostream>
#include <string>
#include <deque>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "message.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>


using namespace boost::asio;

class transport
{
public:
	transport(boost::asio::io_service& io_service, std::string ip_address, std::string port)
		: io_service_(io_service), socket_(io_service), resolver_(io_service)
	{
		boost::asio::ip::tcp::endpoint
			endpoint(*resolver_.resolve({ ip_address.c_str(), port.c_str() }));
		socket_.connect(endpoint, error_);
		std::cout << "connected : " << ip_address.c_str() << " " << port.c_str() << std::endl;
	}
	size_t send(const message& newmessage)
	{
		// For serialization
		std::ostringstream archive_stream;
		boost::archive::text_oarchive archive(archive_stream);
		archive << newmessage;
		outbound_data_ = archive_stream.str();
		//size_t bytes = boost::asio::write(socket_, boost::asio::buffer(newmessage, sizeof(newmessage)));
		size_t bytes = boost::asio::write(socket_, boost::asio::buffer(outbound_data_));
		return bytes;
	}
	void close()
	{
		io_service_.post([this](){ socket_.close(); });
	}

	boost::asio::ip::tcp::socket socket_;
	boost::asio::io_service& io_service_;
	boost::asio::ip::tcp::resolver resolver_;
	boost::system::error_code error_;
	std::string outbound_data_;
};

#endif