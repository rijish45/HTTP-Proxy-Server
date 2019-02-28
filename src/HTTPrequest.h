/**
 * Caching Proxy Server in C++
 * February 18, 2019
 * Prathikshaa Rangarajan (pr109), Rijish Ganguly (rg239), David Laub (dgl9)
 */

#ifndef _HTTPREQUEST_H
#define _HTTPREQUEST_H

#include <algorithm>
#include <iostream>
#include <istream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#define MAX_BUFFER 65536

using namespace std;

typedef std::unordered_map<std::string, std::string> stringmap;

class HTTPrequest {

public:
  vector<char> request_buffer;
  stringmap fv_map; // field value mapping
  string request_line;
  string server;
  string http_method;
  string http_type;

  int server_port_num;
  int total_length;
  int header_length;
  int content_length;

  int get_content_length();
  int build_fv_map();
  int set_fields();
  int set_header(string request);
  string return_etag();
  string return_header();
  string get_cache_control();
  bool header_receive_successful();
  string get_field_value(string field);

  HTTPrequest() {
    this->header_length = 0;
    this->content_length = 0;
    this->total_length = 0;
    request_buffer.resize(MAX_BUFFER);
  };
  ~HTTPrequest(){};
};

int HTTPrequest::build_fv_map(void) {
  string request(request_buffer.data());
  std::transform(request.begin(), request.end(), request.begin(),
                 std::ptr_fun<int, int>(std::toupper));

  // mark stackoverflow
  std::istringstream split(request);
  std::vector<std::string> lines;
  for (std::string each; std::getline(split, each, '\n'); lines.push_back(each))
    ;
  // mark end

  for (auto i : lines) {
    size_t pos = i.find(": ");
    if (pos != std::string::npos) {
      fv_map[i.substr(0, pos)] = i.substr(pos + 2);
      // string temp = get_field_value(i.substr(0,pos));
      // cout << fv_map[i.substr(0, pos)] << temp << endl;
    }
  }
  //cout << request << endl;
  return 1;
}

string HTTPrequest::get_field_value(string field) {
  if (fv_map.find(field) == fv_map.end()) {
    string fail;
    //cout << "FAIL" << endl;
    return fail;
  }
  return fv_map[field];
}

int HTTPrequest::set_header(string request) {
  this->request_buffer = vector<char>(request.begin(), request.end());
  return 1;
}

string HTTPrequest::return_header() {

  string temp(request_buffer.data());
  string header = temp.substr(0, temp.find("\r\n\r\n") + 4);
  return header;
}

bool HTTPrequest::header_receive_successful() {

  string temp(request_buffer.data());
  size_t end_position = temp.find("\r\n\r\n");
  this->header_length = end_position + 4;
  this->total_length = header_length;
  if (end_position != string::npos) {
    return true;
  } else
    return false;
}

string HTTPrequest::return_etag() {

  string header(request_buffer.data(), header_length);
  size_t find_position = header.find("ETag");
  if (find_position == string::npos) {
    return NULL;
  } else {
    size_t end_position = header.find("/r/n", find_position);
    string etag(header.substr(find_position, find_position - end_position));
    return etag;
  }
}

int HTTPrequest::get_content_length() {
  string temp(request_buffer.data());
  size_t content_len_pos =
      temp.find("content-length: "); // make case insensitive
  if (content_len_pos == string::npos) {
    return -1;
  } else {
    string parse(temp.erase(0, content_len_pos));
    content_len_pos = parse.find(": ");
    size_t position = parse.find("\r\n");
    if (position == string::npos) {
      cerr << "Strange request." << endl;
      return -1;
    } else {

      stringstream ss;
      ss << parse.substr(content_len_pos + 2, content_len_pos - position);
      ss >> this->content_length;
      this->total_length = this->header_length + this->content_length;
      return this->content_length;
    }
  }
}

int HTTPrequest::set_fields() {

  string original_request(request_buffer.data());
  string request;

  // get firstline
  size_t position = original_request.find("\r\n");
  if (!(position == string::npos)) {
    request = original_request.substr(0, position);
    this->request_line = request;
  } else {
    cerr << "Request format is incorrect." << endl;
    return -1;
  }

  // Get method
  string method;
  size_t position_two = request.find(" ");
  if (!(position_two == string::npos)) {
    method = request.substr(0, position_two);
    this->http_method = method;
  } else {
    cerr << "Request format is incorrect." << endl;
    return -1;
  }

  if (this->http_method != "GET" && this->http_method != "POST" &&
      this->http_method != "CONNECT") {
    cerr << "Method is not supported." << endl;
    return -1;
  }
  // Set the port numbers
  if ((this->http_method.compare("GET") == 0) ||
      (this->http_method.compare("POST") == 0))
    this->server_port_num = 80;
  else if (this->http_method.compare("CONNECT") == 0)
    this->server_port_num = 443;

  // Get http type
  request.erase(0, position_two + 1);
  position_two = request.find(" ");
  if (!(position_two == string::npos)) {
    this->http_type = request.substr(position_two + 1);
  } else {
    cerr << "Request format is incorrect." << endl;
    return -1;
  }

  if ((this->http_type.compare("HTTP/1.0") != 0) &&
      (this->http_type.compare("HTTP/1.1") != 0)) {
    cerr << "HTTP type is incorrect." << endl;
    return -1;
  }

  // Get server name
  string helper_request(request_buffer.data());
  string host;
  helper_request = helper_request.substr(helper_request.find("Host: ") + 6);
  host = helper_request.substr(0, helper_request.find("\r\n"));
  this->server = host;
  size_t position_three = host.find(":");
  if ((position_three != string::npos)) {
    host = host.substr(0, position_three);
    this->server = host;
  }

  // The case where port num is different than 443 or 80
  string actual_request(request_buffer.data());
  string temporary;
  actual_request = actual_request.substr(actual_request.find("Host:") + 6);
  size_t position_four = actual_request.find("\r\n");
  actual_request = actual_request.substr(0, position_four);

  if (actual_request.find(":") != string::npos) {
    actual_request = actual_request.substr(actual_request.find(":") + 1);
    temporary = actual_request.substr(0, actual_request.find("\r\n"));
    this->server_port_num = std::stoi(temporary);
  }

  return 0;
}

string HTTPrequest::get_cache_control() {
  string cache_control;
  string request(request_buffer.data());
  size_t position = request.find("Cache-Control: ");
  if (position != string::npos) {
    request = request.substr(position);
    int position_two = request.find("\r\n");
    string date = request.substr(0, position_two);
  }

  return cache_control;
}

#endif
