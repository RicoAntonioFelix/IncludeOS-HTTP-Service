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

#ifndef HTTP_REQUEST_LINE_HPP
#define HTTP_REQUEST_LINE_HPP

#include <regex>
#include <cctype>

#include "common.hpp"
#include "methods.hpp"
#include "version.hpp"

namespace http {

/**
 * @brief This class represents the {Request-Line}
 * of an HTTP request message
 */
class Request_line {
public:
  /**
   * @brief Default constructor
   */
  template<typename = void>
  explicit Request_line();

  /**
   * @brief Construct a {Request-Line} from the incoming
   * character stream of data
   *
   * @param request:
   * The character stream of data
   */
  explicit Request_line(std::experimental::string_view request);

  /**
   * @brief Default copy constructor
   */
  Request_line(Request_line&)  = default;

  /**
   * @brief Default move constructor
   */
  Request_line(Request_line&&) = default;

  /**
   * @brief Default destructor
   */
  ~Request_line() noexcept = default;

  /**
   * @brief Default copy assignment operator
   */
  Request_line& operator = (Request_line&)  = default;

  /**
   * @brief Default move assignment operator
   */
  Request_line& operator = (Request_line&&) = default;

  /**
   * @brief Get the method of the message
   *
   * @return The method of the message
   */
  template<typename = void>
  Method method() const noexcept;

  /**
   * @brief Set the method of the message
   *
   * @param method:
   * The method of the message
   */
  template<typename = void>
  void set_method(const Method method) noexcept;

  /**
   * @brief Get a reference to the URI object in
   * the message
   *
   * @return A reference to the URI object
   */
  template<typename = void>
  URI& uri() noexcept;

  /**
   * @brief Get a read-only reference to the URI object
   * in the message
   *
   * @return A read-only reference to the URI object
   */
  template<typename = void>
  const URI& uri() const noexcept;

  /**
   * @brief Set the URI of the message
   *
   * @param uri:
   * The URI of the message
   */
  template<typename = void>
  void set_uri(const URI& uri);

  /**
   * @brief Get the version of the message
   *
   * @return The version of the message
   */
  template<typename = void>
  Version version() const noexcept;

  /**
   * @brief Set the version of the message
   *
   * @param version:
   * The version of the message
   */
  template<typename = void>
  void set_version(const Version version) noexcept;

  /**
   * @brief Get a string representation of this
   * class
   *
   * @return A string representation
   */
  std::string to_string() const;

  /**
   * @brief Operator to transform this class
   * into string form
   */
  operator std::string () const;
  //-----------------------------------
private:
  //-----------------------------------
  // Class data members
  Method  method_  {GET};
  URI     uri_     {"/"};
  Version version_ {1U, 1U};
  //-----------------------------------
}; //< class Request_line

/**
 * @brief This class is used to represent an error that occurred
 * from within the operations of class Request_line
 */
class Request_line_error : public std::runtime_error {
  using runtime_error::runtime_error;
};

/**--v----------- Implementation Details -----------v--**/

template<typename>
inline Request_line::Request_line() {}

inline Request_line::Request_line(std::experimental::string_view request) {
  if (request.empty() or (request.length() < 15) /*<-(15) minimum request length */) {
    throw Request_line_error{"Invalid request: " + request.to_string()};
  }

  // Extract {Request-Line} from request
  std::size_t index;
  std::experimental::string_view request_line;

  bool is_canonical_line_ending {false};

  if ((index = request.find("\r\n")) not_eq std::experimental::string_view::npos) {
    request_line = request.substr(0, index);
    is_canonical_line_ending = true;
  } else if ((index = request.find('\n')) not_eq std::experimental::string_view::npos) {
    request_line = request.substr(0, index);
  } else {
    throw Request_line_error{"Invalid line-ending"};
  }

  const static std::regex request_line_pattern
  {
    "\\s*(GET|POST|PUT|DELETE|OPTIONS|HEAD|TRACE|CONNECT) " // Method
    "(\\S+) " // URI
    "HTTP/(\\d+)\\.(\\d+)" // Version Major.Minor
  };

  std::cmatch m;

  if (not std::regex_match(request_line.data(), request_line.data() + request_line.length(), m, request_line_pattern)) {
    throw Request_line_error{"Invalid request line: " + request_line.to_string()};
  }

  method_ = method::code(m[1].str());

  new (&uri_) URI(m[2]);

  unsigned maj = static_cast<unsigned>(std::stoul(m[3]));
  unsigned min = static_cast<unsigned>(std::stoul(m[4]));
  version_ = Version{maj, min};

  // Trim the request for further processing
  if (is_canonical_line_ending) {
    request = request.substr(index + 2);
  } else {
    request = request.substr(index + 1);
  }
}

template<typename>
inline Method Request_line::method() const noexcept {
  return method_;
}

template<typename>
inline void Request_line::set_method(const Method method) noexcept {
  method_ = method;
}

template<typename>
inline URI& Request_line::uri() noexcept {
  return uri_;
}

template<typename>
inline const URI& Request_line::uri() const noexcept {
  return uri_;
}

template<typename>
inline void Request_line::set_uri(const URI& uri) {
  new (&uri_) URI(uri);
}

template<typename>
inline Version Request_line::version() const noexcept {
  return version_;
}

template<typename>
inline void Request_line::set_version(const Version version) noexcept {
  version_ = version;
}

inline std::string Request_line::to_string() const {
  std::ostringstream req_line;
  //----------------------------
  req_line << method::str(method_) << " "
           << uri_                 << " "
           << version_             << "\r\n";
  //-----------------------------
  return req_line.str();
}

inline Request_line::operator std::string () const {
  return to_string();
}

inline std::ostream& operator << (std::ostream& output_device, const Request_line& req_line) {
  return output_device << req_line.to_string();
}

/**--^----------- Implementation Details -----------^--**/

} //< namespace http

#endif //< HTTP_REQUEST_LINE_HPP
