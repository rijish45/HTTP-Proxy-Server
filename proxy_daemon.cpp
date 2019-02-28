/**
 * Caching Proxy Server in C++
 * February 18, 2019
 * Prathikshaa Rangarajan (pr109), Rijish Ganguly (rg239), David Laub (dgl9)
 */

#define HTTP_PORT "12345"
#define LISTEN_BACKLOG 1000

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <vector>

#include "HTTPrequest.h"
#include "HTTPresponse.h"
#include "cache.h"
#include "proxy_daemon.h"

#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

using namespace std;

Cache s_cache(10000000);
size_t ID = 0;
__thread size_t myID = 0;

static pthread_mutex_t id_lock;
static pthread_mutex_t cache_lock;

ofstream outfile;

// need to use boost libraries -- see how/why

/*
int forward_connect(int fd1, int fd2) {
  vector<char> data;
  char buffer[1];
  while (1) {
    int size =
        recv(fd1, buffer, 1, MSG_WAITALL); // while loop to receive everything
    data.push_back(buffer[0]);
    if (size == 0) {
      break;
    }
  }
  return 1;
}
*/

bool no_cache(HTTPrequest request_obj) {

  string cc = request_obj.get_field_value("CACHE-CONTROL");
  if ((cc.find("NO-STORE") != std::string::npos) ||
      (cc.find("PRIVATE") != std::string::npos)) {
    return true;
  }

  return false;
}

bool no_cache(HTTPresponse request_obj) { // response

  string cc = request_obj.get_field_value("CACHE-CONTROL");
  if ((cc.find("NO-STORE") != std::string::npos) ||
      (cc.find("PRIVATE") != std::string::npos)) {
    return true;
  }

  return false;
}

string getfromCC(string cc, string field) {
  size_t ptr;
  string val;
  if ((ptr = cc.find(field)) != std::string::npos) {
    val = cc.substr(ptr + field.length()); //, cc.find(","));
  }
  return val;
}

bool is_fresh(HTTPrequest request_obj) {
  //
  string cc = request_obj.get_field_value("CACHE-CONTROL"); // max_age

  //  time_t
  time_t req_time = s_cache.time_cache[request_obj.request_line];
  string max_age_str = getfromCC(cc, "MAX_AGE=");
  size_t max_age = str_to_num(max_age_str.c_str());

  if (difftime(time(NULL), req_time) < max_age) {
    return true;
  }
  cout << myID << ": in cache, but expired at "
       << request_obj.get_field_value("EXPIRES") << endl;

  return false; // change;
}

bool has_validate_tags(HTTPrequest request_obj) {

  string cc = request_obj.get_field_value("CACHE-CONTROL");
  if ((cc.find("NO-CACHE") != std::string::npos) ||
      (cc.find("MUST-REVALIDATE") != std::string::npos)) {
    return true;
  }

  return false;
}

HTTPresponse forward(HTTPrequest request_obj) {
  string port = "80"; // check HTTPrequest obj
  int server_fd = forward_request(request_obj.server.c_str(), port.c_str(),
                                  request_obj.request_buffer.data());

  return receive_response(server_fd);
}

HTTPresponse validate(HTTPrequest request_obj) {
  // cache
  string request_line = request_obj.request_line; // check?;

  // etag
  string etag = s_cache.response_cache[request_line].get_field_value("ETAG");

  // last modified
  string last_modified =
      s_cache.response_cache[request_line].get_field_value("LAST-MODIFIED");

  string req = build_validation_req(request_line, etag, last_modified);

  string port = "80"; // check HTTPrequest obj
  // make request to server
  int server_fd =
      forward_request(request_obj.server.c_str(), port.c_str(), req.c_str());

  HTTPresponse response_obj = receive_response(server_fd);

  // 304 ok -- return from cache

  // full response -- replace in cache
  cout << myID << ": in cache, valid" << endl;
  return s_cache.response_cache[request_line]; // todo: fix
}

string build_validation_req(string request_line, string etag,
                            string last_modified) {
  std::stringstream ss;
  ss << request_line << std::endl
     << "If-None-Match: " << etag << std::endl
     << "If-Modified-Since: " << last_modified;
  string req = ss.str();
  return req;
}

HTTPresponse deal_with_cache(HTTPrequest request) {

  if (no_cache(request)) { // no_cache boolean
    return forward(request);

  } else {
    // check cache
    std::vector<char> resp = s_cache.lookup(request.request_line);

    // if its not in the cache forward
    if (resp.size() == 0) {

      outfile << myID << ": not in cache" << endl;
      cout << myID << ": not in cache" << endl;

      // cout << "Cache\n" << endl;
      // s_cache.print();

      HTTPresponse response = forward(request);
      // if (no_cache(response)) {
      //   return response;
      // } else {
      pthread_mutex_lock(&cache_lock);
      s_cache.insert(request.request_line, resp, request, response);
      pthread_mutex_unlock(&cache_lock);
      return response;
      // }
    }
    // otherwise check freshness
    else {
      outfile << myID << ": HIT in cache" << endl;
      cout << myID << ": HIT in cache" << endl;

      // if not fresh validate
      if (!is_fresh(request)) {
        return validate(request);
      } else {

        // if we can check cache and its fresh and doesnt need validation send
        // back response
        if (!has_validate_tags(request)) {
          outfile << myID << ": in cache, valid" << endl;
          cout << myID << ": in cache, valid" << endl;
          HTTPresponse response = s_cache.response_cache[request.request_line];
          return response;
        } else {
          // use ETAG

          return validate(request);
        }
      }
    }
  }
}

HTTPrequest receive_request(int user_fd) {

  char http_req_buf[1];
  std::vector<char> request_header;
  size_t line_break_count = 0;

  while (1) {
    int rec = recv(user_fd, http_req_buf, 1,
                   MSG_WAITALL); // while loop to receive everything
    if (rec == 0) {
      pthread_exit(NULL);
    }
    request_header.push_back(http_req_buf[0]);
    if (http_req_buf[0] == '\n') {
      string str(request_header.end() - 4, request_header.end());
      if (str == "\r\n\r\n") {
        if (line_break_count == 0) {
          break;
        }
        line_break_count++;
      }
    }
  }

  HTTPrequest request_obj;
  request_obj.request_buffer = request_header;
  request_obj.build_fv_map();
  // cout << request_obj.get_field_value("PROXY-CONNECTION") << endl;
  // cout << request_obj.get_field_value("CONNECTION") << endl;
  // cout << request_obj.get_field_value("CACHE-CONTROL") << endl;

  // // DELETE
  // if (request_obj.set_fields() == -1) {
  //   return request_obj;
  // }
  // DELETE

  time_t now = time(0);

  outfile << myID << ": \"" << request_obj.request_line << "\" from "
          << request_obj.server << " @ " << ctime(&now) << endl;
  int content_len = request_obj.get_content_length();
  if (content_len >= 0) {
    // cout << "len = " << content_len << endl;
    std::vector<char> msg_body(content_len);
    int recv_err = recv(user_fd, msg_body.data(), content_len, MSG_WAITALL);
    if (recv_err == -1) {
      cerr << "recv failed" << endl;
      perror("recv");
    }
    // cout << "printing msg" << endl;
    request_obj.request_buffer.insert(request_obj.request_buffer.end(),
                                      msg_body.begin(), msg_body.end());
  }
  return request_obj;
}

int forward_request(const char *hostname, const char *port,
                    const char *request) {

  int serverfd = open_client_socket(hostname, port);
  // cout << "client connection successful attempting to send #bytes : "
  //     << strlen(request) << endl
  //     << request << endl;

  int num_to_send = strlen(request);
  while (num_to_send > 0) {
    // cout << "bytes left to send : " << num_to_send << endl;
    int num_sent = send(serverfd, request, num_to_send,
                        0); // while loop to send everything
    request += num_sent;
    num_to_send -= num_sent;
  }

  return serverfd;
}

HTTPresponse receive_response(int server_fd) {
  // get HTTP header
  char buffer[1];
  std::vector<char> response;
  size_t line_break_count = 0;
  while (1) {
    // print_vec(response);
    recv(server_fd, buffer, 1,
         MSG_WAITALL); // while loop to receive everything
    response.push_back(buffer[0]);
    if (buffer[0] == '\n') {
      // cout << "newline" << endl;
      // print_vec(response);
      string str(response.end() - 4, response.end());
      // cout << "\"" << str << "\"" << endl;
      // break;
      if (str == "\r\n\r\n") {
        if (line_break_count == 0) {
          break;
        }
        line_break_count++;
      }
    }
  }

  std::string resp_str(response.begin(), response.end());
  // print_vec(response);
  HTTPresponse response_object;
  response_object.response_buffer = response;
  response_object.build_fv_map();
  // cout << resp_str << endl;
  // cout << "CACHE CONTROL" << response_object.get_field_value("CACHE-CONTROL")
  //     << endl;
  // print_vec(response_object.response_buffer); // remove

  int content_len = response_object.get_content_length();
  if (content_len >= 0) {
    // cout << "len = " << content_len << endl;

    std::vector<char> msg_body2(content_len);

    int recv_err = recv(server_fd, msg_body2.data(), content_len, MSG_WAITALL);
    if (recv_err == -1) {
      cerr << "recv failed" << endl;
      perror("recv");
    }
    // cout << "printing msg" << endl;

    // print_vec(msg_body2);

    response_object.response_buffer.insert(
        response_object.response_buffer.end(), msg_body2.begin(),
        msg_body2.end());
    // cout << "printing msg with body" << endl;
    // print_vec(response_object.response_buffer);
    // cout << "Successful Receive" << endl;
    // error checking
    // print_vec(response);
    // cout << response << endl;
  }
  return response_object;
}

// OPEN A TUNNEL FOR CONNECT
void openTunnel(const char *hostname, const char *port, int user_fd) {
  int serverfd = open_client_socket(hostname, port);

  int fdmax = serverfd;
  fd_set master;
  fd_set read_fds;
  FD_ZERO(&master);
  FD_ZERO(&read_fds);
  FD_SET(serverfd, &master);
  FD_SET(user_fd, &master);

  // SEND ACK TO FIREFOX THAT A CONNECTION HAS BEEN ESTABLISHED WITH THE
  // ORIGIN SERVER
  string OK =
      "HTTP/1.1 200 Connection Established\r\nConnection: close\r\n\r\n";
  sendall(OK.c_str(), user_fd, OK.length());

  while (1) {
    read_fds = master;
    if ((select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)) {
      perror("select");
      exit(4);
    }
    if (FD_ISSET(user_fd, &read_fds)) {
      std::vector<char> v_buffer(2048);
      // cout << "from client" << endl;
      int rec_size = recv(user_fd, v_buffer.data(), 2048, 0);
      // cout << rec_size << endl;
      if (rec_size == -1 || rec_size == 0) {
        break;
      }
      sendall(v_buffer.data(), serverfd, rec_size);
    }
    if (FD_ISSET(serverfd, &read_fds)) {
      // cout << "from server" << endl;
      std::vector<char> v_buffer(2048);
      int rec_size = recv(serverfd, v_buffer.data(), 2048, 0);
      // cout << rec_size << endl;
      if (rec_size == -1 || rec_size == 0) {
        break;
      }
      sendall(v_buffer.data(), user_fd, rec_size);
    }
  }
  // cout << "connect finished" << endl;
  close(user_fd); // when we have multithreading
  close(serverfd);
}

void *process_request(void *uid) {
  // cout << (long)uid << endl;
  long user_fd = (long)uid;
  if (user_fd == -1) {
    perror("accept");
  }

  pthread_mutex_lock(&id_lock);
  myID = ID;
  ID++;
  pthread_mutex_unlock(&id_lock);

  cout << "Request ID:" << myID << endl;

  // cout << "new connection" << endl;

  // cout << "incoming" << endl;
  while (1) {
    HTTPrequest request_obj = receive_request(user_fd);

    // DELETE
    if (request_obj.set_fields() == -1) {
      continue;
    }
    // DELETE

    string port = "80";
    // cout << request_obj.http_method << endl;
    // IF GET OR POST WE FORWARD ALONG
    if (request_obj.http_method == "GET" || request_obj.http_method == "POST") {
      // FORWARD REQUEST MAYBE JUST TAKE THE REQUEST???

      HTTPresponse response_obj = deal_with_cache(request_obj);

      /*
      int server_fd = forward_request(request_obj.server.c_str(), port.c_str(),
                                      request_obj.request_buffer.data());

      HTTPresponse response_obj = receive_response(server_fd);
      std::vector<char> response = response_obj.response_buffer;
      */

      sendall(response_obj.response_buffer.data(), user_fd,
              response_obj.response_buffer.size());

    } else {
      // PERFECT
      openTunnel(request_obj.server.c_str(), "443", user_fd);
    }
  }
  pthread_exit(NULL);
}

int main(void) {
  Cache s_cache;
  // listener socket -- bind to port (80)?
  char port[10] = HTTP_PORT;
  int listener_fd = open_server_socket(NULL, port);
  outfile.open("proxy.log");
  /*-----------------------become daemon-----------------------*/
  // BEGIN_REF - http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html

  pid_t pid, sid;

  /* Fork off the parent process */
  pid = fork();
  if (pid < 0) {
    outfile << "exiting in pid < 0" << endl;
    exit(EXIT_FAILURE);
  }
  /* If we got a good PID, then
     we can exit the parent process. */
  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  /* Change the file mode mask */
  umask(0);

  /* Open any logs here */

  /* Create a new SID for the child process */
  sid = setsid();
  if (sid < 0) {
    /* Log any failures here */
    exit(EXIT_FAILURE);
  }

  /* Change the current working directory */
  // if ((chdir("/")) < 0) {
  /* Log any failures here */
  //  exit(EXIT_FAILURE);
  //}

  /* Close out the standard file descriptors */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  /* Daemon-specific initialization goes here */

  // END_REF
  /*-----------------------------------------------------------*/

  struct sockaddr_storage user_addr;
  socklen_t user_addr_len = sizeof(user_addr);
  long user_fd;

  for (;;) { // daemon loop

    user_fd =
        accept(listener_fd, (struct sockaddr *)&user_addr, &user_addr_len);
    pthread_t *thrd = (pthread_t *)malloc(sizeof(pthread_t));
    int err = pthread_create(thrd, NULL, &process_request, (void *)user_fd);
    if (err) {
      cout << "error" << endl;
      exit(-1);
    }
  } // END for(;;)--and you thought it would never end!
  outfile.close();
  return EXIT_SUCCESS;
  // sleep(30); /* wait 30 seconds */
}
