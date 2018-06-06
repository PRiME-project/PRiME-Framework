/* This file is part of the PRiME Framework.
 * 
 * The PRiME Framework is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * The PRiME Framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with the PRiME Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by Graeme Bragg & James Bantock
 */

#ifndef _UDS_H
#define _UDS_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <queue>
#include <mutex>
#include <string>
#include <vector>
#include <iostream>
#include "args/args.hxx"

namespace prime
{
	class uds
	{
	public:
		
		enum class socket_layer_t
		{
			DEV_RTM,
			RTM_DEV,
			RTM_APP,
			APP_RTM,
			LOGGER,
			UI
		};
		
		struct socket_addrs_t
		{
			bool logger_en = true;
			bool logger_addr = false;			// Is the logger address present?
			bool remote_logger = false;			// Is the logger address remote?
			std::string logger_local_address;	// Local UDS filename.
			std::string logger_remote_address;	// Remote UDS filename or remote IP address.
			unsigned int logger_port;			// Remote logger port.
			bool api_addr = false;				// Is the API address present?
			bool remote_api = false;			// Is the API address remote?
			std::string api_local_address;		// Local UDS filename.
			std::string api_remote_address;		// Remote UDS filename or remote IP address.
			unsigned int api_port;				// API port.
			bool ui_addr = false;				// Is the UI address present?
			bool remote_ui = false;				// Is the UI address remote?
			std::string ui_local_address;		// Local UDS filename.
			std::string ui_remote_address;		// Remote UDS filename or remote IP address.
			unsigned int ui_port;				// API port.
		};
		
		uds(
			prime::uds::socket_layer_t socket_layer,
			prime::uds::socket_addrs_t *socket_addrs, 
			boost::function<void(std::vector<char>)> handler = NULL
		);
		
		uds(std::string local_filename, std::string remote_filename);
		
		uds(
			std::string local_filename,
			std::string remote_filename,
			boost::function<void(std::vector<char>)> handler
		);
		
		uds(
			std::string remote_address,
			unsigned int remote_port
		);
		
		uds(
			std::string remote_address,
			unsigned int remote_port,
			boost::function<void(std::vector<char>)> handler
		);
		
		uds(
			unsigned int local_port
		);
		
		uds(
			unsigned int local_port,
			boost::function<void(std::vector<char>)> handler
		);
		
		~uds();
		
		void set_remote_endpoint(std::string remote_filename);
		void send_message(std::vector<char> &message);
		void send_message_blocking(std::vector<char> &message);
		bool get_message(std::vector<char> &message);
		std::string get_endpoints(void);
		
		
	private:
		bool local;
		bool server;
		char layer;
		
		boost::function<void(std::vector<char>)> handler;

		std::queue<std::vector<char>> read_queue;
		std::mutex read_queue_mutex;

		boost::asio::io_service io_service;
		boost::asio::local::datagram_protocol::endpoint local_endpoint;
		boost::asio::local::datagram_protocol::endpoint remote_endpoint;
		boost::asio::local::datagram_protocol::endpoint sender_endpoint;
		boost::asio::local::datagram_protocol::socket uds_socket;
		
		boost::asio::ip::udp::resolver udp_resolver;
		boost::asio::ip::udp::resolver::query udp_query;
		boost::asio::ip::udp::endpoint udp_local_endpoint;
		boost::asio::ip::udp::endpoint udp_remote_endpoint;
		boost::asio::ip::udp::socket udp_socket;
		

		std::vector<char> receive_buffer;

		boost::thread io_service_thread;

		void async_receive();
		void handle_receive(
			const boost::system::error_code& error,
			std::size_t bytes_transferred
		);
		void async_receive_int();
		void handle_receive_int(
			const boost::system::error_code& error,
			std::size_t bytes_transferred
		);
		void async_write(std::vector<char> packet);
		void handle_write(
			const boost::system::error_code& error,
			std::size_t bytes_transferred
		);
		void sync_write(std::vector<char> packet);
	};
}

#endif
