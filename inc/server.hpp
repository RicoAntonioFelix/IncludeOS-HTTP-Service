// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <cstdio>
#include <cstdint>

#include <net/inet4>
#include <net/dhcp/dh4client.hpp>

#include "router.hpp"

#define MTU 1500

namespace http {

//-------------------------------
// This class is a simple dumb
// HTTP server for service testing
//-------------------------------
class Server {
private:
  //-------------------------------
  // Internal class type aliases
  //-------------------------------
  using Port     = const uint16_t;
  using IP_Stack = std::unique_ptr<net::Inet4<VirtioNet>>;
  using Callback = std::function<void()>;
  //-------------------------------
public:
  //-------------------------------
  // Default constructor to set up
  // the server
  //-------------------------------
  explicit Server();

  //-------------------------------
  // Default destructor
  //-------------------------------
  ~Server() noexcept = default;

  //-------------------------------
  // Get the underlying router
  // which contain route resolution
  // configuration
  //-------------------------------
  Router& router() noexcept;

  //-------------------------------
  // Install a new route table for
  // route resolutions
  //
  // @tparam (http::Router) new_routes - The new route table
  //                                     to install
  //
  // @return - The object that invoked this method
  //-------------------------------
  template <typename Route_Table>
  Server& set_routes(Route_Table&& routes);

  //-------------------------------
  // Start the server to listen for
  // incoming connections on the
  // specified port
  //-------------------------------
  void listen(Port port, Callback callback);

private:
  //-------------------------------
  // Class data members
  //-------------------------------
  IP_Stack inet_;
  Router   router_;

  //-----------------------------------
  // Deleted move and copy operations
  //-----------------------------------
  Server(const Server&) = delete;
  Server(Server&&) = delete;

  //-----------------------------------
  // Deleted move and copy assignment operations
  //-----------------------------------
  Server& operator = (const Server&) = delete;
  Server& operator = (Server&&) = delete;

  //-------------------------------
  // Set up the network stack
  //-------------------------------
  void initialize();
}; //< class Server

/**--v----------- Implementation Details -----------v--**/

// #define DEBUG

inline Server::Server() {
  initialize();
}

inline Router& Server::router() noexcept {
  return router_;
}

template <typename Route_Table>
inline Server& Server::set_routes(Route_Table&& routes) {
  router_.install_new_configuration(std::forward<Route_Table>(routes));
  return *this;
}

inline void Server::listen(Port port, Callback callback = []{ printf("%s\n", "Server started"); }) {
  Server& server = *this;

  inet_->tcp()
       .bind(port)
       .onConnect([&server](auto conn) {
          conn->read(MTU, [conn, &server](net::TCP::buffer_t buf, size_t n) {
            auto data = std::string{static_cast<char*>(buf.get()), n};
            debug("Received data: %s\n", data.c_str());

	          // Create request / response objects for callback
	          Request  req {std::move(data)};
	          Response res;

	          // Get and call the callback
	          server.router_[{req.method(), req.uri()}](req, res);
            //-------------------------------

	          auto str = res.to_string();
	          conn->write(str.data(), str.size(), [conn, &str](size_t) {
		          debug("Wrote %i bytes back\n", str.size());
		          conn->close();
	          }); //< conn->write
	        }); //< conn->read
       }); //< onConnect

  callback();
}

void Server::initialize() {
  auto& eth0 = hw::Dev::eth<0,VirtioNet>();
  //-------------------------------
  inet_ = std::make_unique<net::Inet4<VirtioNet>>(eth0);
  //-------------------------------
  inet_->network_config(
      { 10,0,0,42 },     // IP
		  { 255,255,255,0 }, // Netmask
		  { 10,0,0,1 },      // Gateway
		  { 8,8,8,8 }        // DNS
  );
  //-------------------------------
}

/**--^----------- Implementation Details -----------^--**/

} // namespace http

#endif //< HTTP_SERVER_HPP
