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
 *		written by Charles Leech, Graeme Bragg & James Bantock
 */

#include "uds.h"
// #define DEBUG_UDS
#define DEBUG

#ifdef DEBUG_UDS
#include "util.h"
#endif

namespace prime
{
	uds::uds(
		prime::uds::socket_layer_t socket_layer,
		prime::uds::socket_addrs_t *socket_addrs,
		boost::function<void(std::vector<char>)> handler
		) :
		handler(handler),
		read_queue(),
		read_queue_mutex(),
		io_service(),
		udp_resolver(io_service),
		udp_query("127.0.0.1", "5000", boost::asio::ip::udp::resolver::query::numeric_service),
		//udp_remote_endpoint(boost::asio::ip::address_v4::loopback(), 5000),
		//udp_local_endpoint(boost::asio::ip::address_v4::loopback(), 5000),
		local_endpoint(""),
		remote_endpoint(""),
		udp_socket(io_service),
		uds_socket(io_service)
	{
		boost::system::error_code ec;

		// Work out which layer & type of socket is needed.
		switch(socket_layer) {
			case prime::uds::socket_layer_t::RTM_APP:	// RTM > App. Server socket.
			case prime::uds::socket_layer_t::DEV_RTM:	// Device > RTM. Server socket.
					if(socket_addrs->remote_api) {
						// Remote API Address specified in the struct.
						local = false;

						udp_local_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v6(), socket_addrs->api_port);
						udp_socket.open(boost::asio::ip::udp::v6(), ec);
						if(!ec) {
							udp_socket.set_option(boost::asio::ip::v6_only(false), ec);
						} else {
							// Can't open a v6 socket and put it into dual-listen...
							udp_socket.close();
							udp_socket.open(boost::asio::ip::udp::v4());
							udp_local_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), socket_addrs->api_port);
						}
						udp_socket.bind(udp_local_endpoint);
					} else {
						// It's a local socket.
						local = true;

						local_endpoint = boost::asio::local::datagram_protocol::endpoint(socket_addrs->api_local_address);
						remote_endpoint = boost::asio::local::datagram_protocol::endpoint(socket_addrs->api_remote_address);
						::unlink(socket_addrs->api_local_address.c_str());

						uds_socket.open();
						uds_socket.bind(local_endpoint);
					}
					break;

			case prime::uds::socket_layer_t::APP_RTM:	// App > RTM. Client socket.
			case prime::uds::socket_layer_t::RTM_DEV:	// RTM > Device. Client socket.
					if(socket_addrs->remote_api) {
						// Remote API Address specified in the struct.
						local = false;

						udp_remote_endpoint = boost::asio::ip::udp::endpoint(
												*udp_resolver.resolve(
													boost::asio::ip::udp::resolver::query(
														socket_addrs->api_remote_address,
														std::to_string(socket_addrs->api_port),
														boost::asio::ip::udp::resolver::query::numeric_service)));
						udp_socket.open(udp_remote_endpoint.protocol());
					} else {
						// It's a local socket.
						local = true;

						local_endpoint = boost::asio::local::datagram_protocol::endpoint(socket_addrs->api_local_address);
						remote_endpoint = boost::asio::local::datagram_protocol::endpoint(socket_addrs->api_remote_address);
						::unlink(socket_addrs->api_local_address.c_str());

						uds_socket.open();
						uds_socket.bind(local_endpoint);
					}
					break;

			case prime::uds::socket_layer_t::LOGGER:
					// <X> > Logger. Client socket.
					if(socket_addrs->remote_logger) {
						// Remote Logger Address specified in the struct.
						local = false;

						udp_remote_endpoint = boost::asio::ip::udp::endpoint(
												*udp_resolver.resolve(
													boost::asio::ip::udp::resolver::query(
														socket_addrs->logger_remote_address,
														std::to_string(socket_addrs->logger_port),
														boost::asio::ip::udp::resolver::query::numeric_service)));
						udp_socket.open(udp_remote_endpoint.protocol());
					} else {
						// It's a local socket.
						local = true;

						local_endpoint = boost::asio::local::datagram_protocol::endpoint(socket_addrs->logger_local_address);
						remote_endpoint = boost::asio::local::datagram_protocol::endpoint(socket_addrs->logger_remote_address);
						::unlink(socket_addrs->logger_local_address.c_str());

						uds_socket.open();
						uds_socket.bind(local_endpoint);
					}
					break;

			case prime::uds::socket_layer_t::UI:
					// <X> > UI. Client socket.
					if(socket_addrs->remote_ui) {
						// Remote UI Address specified in the struct.
						local = false;
						udp_remote_endpoint = boost::asio::ip::udp::endpoint(
												*udp_resolver.resolve(
													boost::asio::ip::udp::resolver::query(
														socket_addrs->ui_remote_address,
														std::to_string(socket_addrs->ui_port),
														boost::asio::ip::udp::resolver::query::numeric_service)));
						udp_socket.open(udp_remote_endpoint.protocol());

					} else {
						// It's a local socket.
						local = true;
						local_endpoint = boost::asio::local::datagram_protocol::endpoint(socket_addrs->ui_local_address);
						remote_endpoint = boost::asio::local::datagram_protocol::endpoint(socket_addrs->ui_remote_address);
						::unlink(socket_addrs->ui_local_address.c_str());

						uds_socket.open();
						uds_socket.bind(local_endpoint);
					}
					break;

			default:
					// Should probably throw an error?
					break;

		}

		// Check if we have a handler
		if(handler == NULL) {
			async_receive();
		} else {
			async_receive_int();
		}

		io_service_thread = boost::thread(boost::bind(&boost::asio::io_service::run, &io_service));
	}



	uds::uds(
		std::string local_filename,
		std::string remote_filename
		) :
			local(true),
			read_queue(),
			read_queue_mutex(),
			io_service(),
			udp_resolver(io_service),
			udp_query("127.0.0.1", "5000", boost::asio::ip::udp::resolver::query::numeric_service),
			udp_remote_endpoint(boost::asio::ip::address_v4::loopback(), 5000),
			udp_local_endpoint(boost::asio::ip::address_v4::loopback(), 5000),
			udp_socket(io_service),
			local_endpoint(local_filename),
			remote_endpoint(remote_filename),
			uds_socket(io_service)
	{
		::unlink(local_filename.c_str());
		uds_socket.open();
		uds_socket.bind(local_endpoint);
		async_receive();
		io_service_thread = boost::thread(
		boost::bind(&boost::asio::io_service::run, &io_service));
	}

	uds::uds(
		std::string local_filename,
		std::string remote_filename,
		boost::function<void(std::vector<char>)> handler
		) :
			local(true),
			handler(handler),
			io_service(),
			udp_resolver(io_service),
			udp_query("127.0.0.1", "5000", boost::asio::ip::udp::resolver::query::numeric_service),
			udp_remote_endpoint(boost::asio::ip::address_v4::loopback(), 5000),
			udp_local_endpoint(boost::asio::ip::address_v4::loopback(), 5000),
			udp_socket(io_service),
			local_endpoint(local_filename),
			remote_endpoint(remote_filename),
			uds_socket(io_service)
	{
		::unlink(local_filename.c_str());
		uds_socket.open();
		uds_socket.bind(local_endpoint);
		async_receive_int();
		io_service_thread = boost::thread(
		boost::bind(&boost::asio::io_service::run, &io_service));
	}

	// Network Client: define only the destination address and port
	uds::uds(
		std::string remote_address,
		unsigned int remote_port
		) :
			local(false),
			layer(0),
			server(0),
			io_service(),
			udp_resolver(io_service),
			uds_socket(io_service),
			udp_query(remote_address, std::to_string(remote_port), boost::asio::ip::udp::resolver::query::numeric_service),
			//udp_remote_endpoint(*udp_resolver.resolve(udp_query)),
			udp_remote_endpoint(*udp_resolver.resolve(boost::asio::ip::udp::resolver::query(remote_address, std::to_string(remote_port), boost::asio::ip::udp::resolver::query::numeric_service))),
			udp_local_endpoint(boost::asio::ip::address_v4::loopback(), 5000),
			udp_socket(io_service)
	{
		// Accomodate IPv4 and IPv6.
		udp_socket.open(udp_remote_endpoint.protocol());
		async_receive();
		io_service_thread = boost::thread(
		boost::bind(&boost::asio::io_service::run, &io_service));
	}

	uds::uds(
		std::string remote_address,
		unsigned int remote_port,
		boost::function<void(std::vector<char>)> handler
		) :
			local(false),
			layer(0),
			server(0),
			handler(handler),
			io_service(),
			udp_resolver(io_service),
			uds_socket(io_service),
			udp_query(remote_address, std::to_string(remote_port), boost::asio::ip::udp::resolver::query::numeric_service),
			udp_remote_endpoint(*udp_resolver.resolve(udp_query)),
			udp_local_endpoint(boost::asio::ip::address_v4::loopback(), 5000),
			udp_socket(io_service)
	{
		// Accomodate IPv4 and IPv6.
		udp_socket.open(udp_remote_endpoint.protocol());
		async_receive_int();
		io_service_thread = boost::thread(
		boost::bind(&boost::asio::io_service::run, &io_service));
	}


	// Network Server: define listen port.
 	uds::uds(
		unsigned int local_port
		) :
			local(false),
			layer(0),
			server(1),
			io_service(),
			udp_resolver(io_service),
			uds_socket(io_service),
			udp_query("::0", std::to_string(5000), boost::asio::ip::udp::resolver::query::numeric_service),
			udp_remote_endpoint(boost::asio::ip::address_v6::loopback(), 5000),
			udp_local_endpoint(boost::asio::ip::udp::v6(), local_port),
			udp_socket(io_service)
	{
		// Accomodate IPv4 and IPv6.
		boost::system::error_code ec;
		udp_socket.open(boost::asio::ip::udp::v6(), ec);
		if(!ec) {
			udp_socket.set_option(boost::asio::ip::v6_only(false), ec);
		} else {
			// Can't open a v6 socket and put it into dual-listen...
			udp_socket.close();
			udp_socket.open(boost::asio::ip::udp::v4());
			udp_local_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), local_port);
		}
		udp_socket.bind(udp_local_endpoint);
		async_receive();
		io_service_thread = boost::thread(
		boost::bind(&boost::asio::io_service::run, &io_service));
	}

	uds::uds(
		unsigned int local_port,
		boost::function<void(std::vector<char>)> handler
		) :
			local(false),
			layer(0),
			server(1),
			handler(handler),
			io_service(),
			udp_resolver(io_service),
			uds_socket(io_service),
			udp_query("::0", std::to_string(5000), boost::asio::ip::udp::resolver::query::numeric_service),
			udp_remote_endpoint(boost::asio::ip::address_v6::loopback(), 5000),
			udp_local_endpoint(boost::asio::ip::udp::v6(), local_port),
			udp_socket(io_service)
	{
		// Accomodate IPv4 and IPv6.
		boost::system::error_code ec;
		udp_socket.open(boost::asio::ip::udp::v6(), ec);
		if(!ec) {
			udp_socket.set_option(boost::asio::ip::v6_only(false), ec);
		} else {
			// Can't open a v6 socket and put it into dual-listen...
			udp_socket.close();
			udp_socket.open(boost::asio::ip::udp::v4());
			udp_local_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), local_port);
		}
		udp_socket.bind(udp_local_endpoint);
		async_receive_int();
		io_service_thread = boost::thread(
		boost::bind(&boost::asio::io_service::run, &io_service));
	}

	uds::~uds()
	{
		if(local) {
			uds_socket.shutdown(boost::asio::local::datagram_protocol::socket::shutdown_both);
			uds_socket.close();
		} else {
			//udp_socket.shutdown(boost::asio::ip::udp::socket::shutdown_both);
			udp_socket.close();
		}
		io_service.stop();
		io_service_thread.join();
	}

	std::string uds::get_endpoints(void)
	{
		std::string ret;
		if (local) {
			// Return local socket details:
			ret = "UDS Local: " + uds_socket.local_endpoint().path() + " Remote: " + remote_endpoint.path();
		} else {
			// Return UDP socket details:
			ret = "UDP Local: " + udp_socket.local_endpoint().address().to_string() + ":" + std::to_string(udp_socket.local_endpoint().port())
					+ " Remote: " + udp_remote_endpoint.address().to_string() + ":" + std::to_string(udp_remote_endpoint.port());
		}
		return ret;
	}

	void uds::set_remote_endpoint(std::string remote_filename)
	{
		remote_endpoint.path(remote_filename);
	}

	void uds::send_message(std::vector<char> &message)
	{
#ifdef DEBUG_UDS
		unsigned int time = prime::util::get_timestamp();
#endif

		async_write(message);

#ifdef DEBUG_UDS
		if(sender_endpoint == "/tmp/dev.rtm.uds"){
			// std::cout << sender_endpoint;
			std::cout << "DEV:TIME SEND ASYNC: " << time << std::endl;
		}
#endif
	}

	void uds::send_message_blocking(std::vector<char> &message)
	{
		sync_write(message);
	}

	bool uds::get_message(std::vector<char> &message)
	{
		bool result;
		read_queue_mutex.lock();
		if(!read_queue.empty()) {
			message = read_queue.front();
			read_queue.pop();
			result = true;
		} else {
			result = false;
		}
		read_queue_mutex.unlock();

		return result;
	}

	void uds::async_receive()
	{
		// Set buffer to match IP stack
		receive_buffer = std::vector<char>(65535);
		if(local) {
			uds_socket.async_receive_from(
				boost::asio::buffer(receive_buffer),
				sender_endpoint,
				boost::bind(
					&uds::handle_receive,
					this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred
				)
			);
		} else {
			if(server) {
				udp_socket.async_receive_from(
					boost::asio::buffer(receive_buffer),
					udp_remote_endpoint,
					boost::bind(
						&uds::handle_receive,
						this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred
					)
				);

			} else {
				boost::asio::ip::udp::endpoint udp_endpoint;
				udp_socket.async_receive_from(
					boost::asio::buffer(receive_buffer),
					udp_endpoint,
					boost::bind(
						&uds::handle_receive,
						this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred
					)
				);
			}
		}
	}

	void uds::handle_receive(
		const boost::system::error_code& error,
		std::size_t bytes_transferred)
	{
		(void)bytes_transferred;

		std::vector<char> copied_buffer = receive_buffer;
		async_receive();

		if(error) {
//			std::cerr << "Error receiving packet: " << error << std::endl;
			return;
		}

		read_queue_mutex.lock();
		read_queue.push(copied_buffer);
		read_queue_mutex.unlock();
	}

	void uds::async_receive_int()
	{
#ifdef DEBUG_UDS
		unsigned int time = prime::util::get_timestamp();
#endif
		receive_buffer = std::vector<char>(65535); //Expanded to 10k to cope with Odroid architecture to deal with #31.
		//Further expanded to 65535 when IP added.

		if(local) {
			uds_socket.async_receive_from(
				boost::asio::buffer(receive_buffer),
				sender_endpoint,
				boost::bind(
					&uds::handle_receive_int,
					this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred
				)
			);
#ifdef DEBUG_UDS
			if(sender_endpoint == "/tmp/rtm.dev.uds"){
				// std::cout << sender_endpoint;
				std::cout << "DEV:TIME RECEIVE ASYNC: " << time << std::endl;
			}
#endif
		} else {
			if(server) {
				udp_socket.async_receive_from(
					boost::asio::buffer(receive_buffer),
					udp_remote_endpoint,
					boost::bind(
						&uds::handle_receive_int,
						this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred
					)
				);

			} else {
				boost::asio::ip::udp::endpoint udp_endpoint;
				udp_socket.async_receive_from(
					boost::asio::buffer(receive_buffer),
					udp_endpoint,
					boost::bind(
						&uds::handle_receive_int,
						this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred
					)
				);
#ifdef DEBUG
				if(udp_endpoint != udp_remote_endpoint) {
					std::cout << "\nUDS Client: remote endpoints do not match." << std::endl;
				}
#endif
			}
		}
	}

	void uds::handle_receive_int(
		const boost::system::error_code& error,
		std::size_t bytes_transferred)
	{
		(void)bytes_transferred;
		std::vector<char> copied_buffer = receive_buffer;
		async_receive_int();

		if(error) {
//			std::cerr << "Error receiving packet: " << error << std::endl;
			return;
		}

		handler(copied_buffer);
	}


	void uds::sync_write(std::vector<char> packet)
	{
		if(local) {
			uds_socket.send_to(
				boost::asio::buffer(packet),
				remote_endpoint
			);
		} else {
			udp_socket.send_to(
				boost::asio::buffer(packet),
				udp_remote_endpoint
			);
		}
	}

	void uds::async_write(std::vector<char> packet)
	{
		if(local) {
			io_service.post(
				[this, packet]()
				{
					uds_socket.async_send_to(
						boost::asio::buffer(packet),
						remote_endpoint,
						boost::bind(
							&uds::handle_write,
							this,
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred
						)
					);
				}
			);
		} else {
			io_service.post(
				[this, packet]()
				{
					udp_socket.async_send_to(
						boost::asio::buffer(packet),
						udp_remote_endpoint,
						boost::bind(
							&uds::handle_write,
							this,
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred
						)
					);
				}
			);
		}
	}

	void uds::handle_write(
		const boost::system::error_code& error,
		std::size_t bytes_transferred
		)
	{
		(void)bytes_transferred;
		if(error) {
//			std::cerr << "Error writing packet: " << error << std::endl;
		}
	}
}
