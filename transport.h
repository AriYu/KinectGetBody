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
		// まず本体をシリアル化する。しかしどのような大きさになるかわからない。
		std::ostringstream archive_stream;
		boost::archive::text_oarchive archive(archive_stream);
		archive << newmessage;
		outbound_data_ = archive_stream.str();

		// ヘッダーを作る
		std::ostringstream header_stream;
		header_stream << std::setw(header_length)
			<< std::hex << outbound_data_.size();
		if (!header_stream || header_stream.str().size() != header_length){
			std::cerr << "something error !" << std::endl;
			return 0;
		}
		outbound_header_ = header_stream.str();

		// ヘッダーと本体を一緒にして書き込む
		std::vector<boost::asio::const_buffer> buffers;
		buffers.push_back(boost::asio::buffer(outbound_header_));
		buffers.push_back(boost::asio::buffer(outbound_data_));
		//size_t bytes = boost::asio::write(socket_, boost::asio::buffer(newmessage, sizeof(newmessage)));
		/*size_t bytes = boost::asio::write(socket_, boost::asio::buffer(outbound_data_));*/
		size_t bytes = boost::asio::write(socket_, buffers);
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
	
	// 出力ヘッダー
	std::string outbound_header_;
	// 出力本体
	std::string outbound_data_;

	// ヘッダーの固定サイズ
	enum { header_length = 8 };

	// 入力ヘッダー
	char inbound_header_[header_length];


};

#endif