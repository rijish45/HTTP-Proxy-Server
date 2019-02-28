/**
 * Caching Proxy Server in C++
 * February 18, 2019
 * Prathikshaa Rangarajan (pr109), Rijish Ganguly (rg239), David Laub (dgl9)
 */

#ifndef _HTTPRESPONSE_H
#define _HTTPRESPONSE_H

#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#define MAX_BUFFER_SIZE 102400

using namespace std;

typedef std::unordered_map<std::string, std::string> stringmap;

class HTTPresponse {

public:
  stringmap fv_map; // field value mapping
  int header_length;
  int content_length;
  int total_length;

  vector<char> response_buffer;
  string first_line;
  string code;

  HTTPresponse() {
    this->header_length = 0;
    this->content_length = 0;
    this->total_length = 0;
    this->response_buffer.resize(MAX_BUFFER_SIZE);
  }

  ~HTTPresponse() {}
  string get_etag();
  bool receive_header_set_parameters();
  bool server_response_validate();
  int get_content_length();
  int get_age();

  string get_date();
  string get_last_modified();
  string get_cache_control();
  string get_expiry_time();
  string get_field_value(string field); // new
  int build_fv_map();                   // new
  bool is_valid_response();
  bool check_transfer_encoding();
  bool received_coded_content(int index);
};

int HTTPresponse::build_fv_map(void) {
  string request(response_buffer.data());
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

string HTTPresponse::get_field_value(string field) {
  if (fv_map.find(field) == fv_map.end()) {
    string fail;
    //cout << "FAIL" << endl;
    return fail;
  }
  return fv_map[field];
}

string HTTPresponse::get_etag() {
  string response(this->response_buffer.begin(), this->response_buffer.end());
  string etag;
  size_t position = response.find("ETag:");
  if (position != string::npos) {
    string temp = response.substr(position);
    etag = temp.substr(0, temp.find("\r\n"));
  }

  return etag;
}

bool HTTPresponse::receive_header_set_parameters() {

  size_t position;
  string response(this->response_buffer.data());
  position = response.find("\r\n\r\n");
  this->header_length = position + 4;
  this->total_length = header_length;

  if (position == string::npos) {
    return false;
  } else {

    this->first_line = response.substr(0, response.find("\r\n"));
    position = first_line.find(" ");
    string sub = first_line.substr(position + 1);
    position = sub.find(" ");
    this->code = sub.substr(0, position); // got the code

    return true;
  }
}

int HTTPresponse::get_content_length() {
  string temp(this->response_buffer.data());
  size_t content_len_pos = temp.find("Content-Length: ");
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
      if (content_length == 0) {
        return -1;
      }

      return this->content_length;
    }
  }
}

string HTTPresponse::get_cache_control() {
  string cache_control;
  string request(this->response_buffer.data());
  size_t position = request.find("Cache-Control: ");
  if (position != string::npos) {
    request = request.substr(position);
    int position_two = request.find("\r\n");
    string date = request.substr(0, position_two);
  }

  return cache_control;
}

string HTTPresponse::get_date() {
  // string date;
  string request(this->response_buffer.data());
  size_t position = request.find("Date:");
  string date;
  if (position != string::npos) {
    request = request.substr(position + 6);
    int position_two = request.find("\r\n");
    date = request.substr(0, position_two);
  }

  return date;
}

string HTTPresponse::get_expiry_time() {
  string request(this->response_buffer.data());
  string expire_time;
  size_t position = request.find("Expires:");
  if (position != string::npos) {
    request = request.substr(position + 9);
    int position_two = request.find("\r\n");
    expire_time = request.substr(0, position_two);
  }

  return expire_time;
}

string HTTPresponse::get_last_modified() {

  string request(this->response_buffer.data());
  string last;
  size_t position = request.find("Last-Modified:");
  if (position != string::npos) {
    request = request.substr(position + 15);
    int position_two = request.find("\r\n");
    last = request.substr(0, position_two);
  }

  return last;
}

/* bool HTTPresponse::is_valid_response() { */
/*   string begin = "HTTP/1"; */
/*   for (int i = 0; i < begin.length(); i++) { */
/*     if (this->response_buffer[i] != begin.at(i)) { */
/*       return false; */
/*     } */
/*   } */

/*   return true; */
/* } */

bool HTTPresponse::check_transfer_encoding() {
  string response(this->response_buffer.data());
  size_t position = response.find("Transfer-Encoding: ");
  if (position == string::npos) {
    return false;
  } else
    return true;
}

int HTTPresponse::get_age() {
  string response(this->response_buffer.data());
  size_t position = response.find("Age:");
  if (position != string::npos) {
    response = response.substr(position + 5);
    size_t position_two = response.find("\r\n");
    string age = response.substr(0, position_two);
    int num_age = stoi(age);
    return num_age;
  } else
    return 0;
}

bool HTTPresponse::received_coded_content(int index) {
  for (int i = index; i < this->total_length; i++) {
    if (i > 3 && this->response_buffer[i] == '\n' &&
        this->response_buffer[i - 1] == '\r' &&
        this->response_buffer[i - 2] == '\n' &&
        this->response_buffer[i - 3] == '\r' &&
        this->response_buffer[i - 4] == '0') {
      return true;
    }
  }
  return false;
}

#endif
