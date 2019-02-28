
/**
 * Caching Proxy Server in C++
 * February 18, 2019
 * Prathikshaa Rangarajan (pr109), Rijish Ganguly (rj???), David Laub (dgl9)
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

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>


using namespace std;

Cache s_cache(1000);

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


int open_server_socket(char *hostname, char *port) {
  int fd;
  int status;
  struct addrinfo hints;
  struct addrinfo *addrlist, *rm_it;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &hints, &addrlist);
  if (status != 0) {
    perror("getaddrinfo:");
    return -1;
  }

  for (rm_it = addrlist; rm_it != NULL; rm_it = rm_it->ai_next) {
    fd = socket(rm_it->ai_family, rm_it->ai_socktype, rm_it->ai_protocol);

    if (fd == -1) {
      continue;
    }

    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt:");
      return -1;
    }

    // bind
    int status = ::bind(fd, rm_it->ai_addr, rm_it->ai_addrlen);

    if (status == 0) {
      std::cout << "Successfully bound to port " << port << std::endl; // log
      break;
    }

    // bind failed
    close(fd);
  }
  if (rm_it == NULL) {
    // bind failed
    fprintf(stderr, "Error: socket bind failed\n");
    return -1;
    // exit(EXIT_FAILURE);
  }

  if (listen(fd, LISTEN_BACKLOG) == -1) {
    perror("listen:");
    return -1;
  }

  freeaddrinfo(addrlist);

  return fd;
}

int open_client_socket(const char *hostname, const char *port) {
  cout << hostname << " " << port << endl;
  int fd;
  int status;
  struct addrinfo hints;
  struct addrinfo *addrlist, *rm_it;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &hints, &addrlist);
  if (status != 0) {
    perror("getaddrinfo:");
    return -1;
  }

  for (rm_it = addrlist; rm_it != NULL; rm_it = rm_it->ai_next) {
    fd = socket(rm_it->ai_family, rm_it->ai_socktype, rm_it->ai_protocol);
    // cout << rm_it->ai_addr << endl;
    if (fd == -1) {
      continue;
    }

    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt:");
      return -1;
    }

    if (connect(fd, rm_it->ai_addr, rm_it->ai_addrlen) != -1) {
      // close(fd);
      // perror("client: connect");
      break;
    }
    // bind failed
    close(fd);
  }

  if (rm_it == NULL) {
    // bind failed
    fprintf(stderr, "Error: connect failed\n");
    return -1;
    // exit(EXIT_FAILURE);
  }

  freeaddrinfo(addrlist);

  return fd;
}

std::vector<char> forward_request(const char *hostname, const char *port,
                                  const char *request) {
  int serverfd = open_client_socket(hostname, port);
  cout << "client connection successful attempting to send #bytes : "
       << strlen(request) << endl
       << request << endl;

  int num_to_send = strlen(request);
  while (num_to_send > 0) {
    cout << "bytes left to send : " << num_to_send << endl;
    int num_sent = send(serverfd, request, num_to_send,
                        0); // while loop to send everything
    request += num_sent;
    num_to_send -= num_sent;
  }

  // get HTTP header
  char buffer[1];
  std::vector<char> response;
  size_t line_break_count = 0;
  while (1) {
    // print_vec(response);
    recv(serverfd, buffer, 1, MSG_WAITALL); // while loop to receive everything
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
  //print_vec(response);
  HTTPresponse response_object;
  response_object.response_buffer = response;
  //print_vec(response_object.response_buffer); // remove
  
  int content_len = response_object.get_content_length();
  if(content_len >= 0){
    cout << "len = " << content_len << endl;
      
      std::vector<char> msg_body2(content_len);
      
      int recv_err = recv(serverfd, msg_body2.data(), content_len, MSG_WAITALL);
      if (recv_err == -1) {
	cerr << "recv failed" << endl;
	perror("recv");
      }
      cout << "printing msg" << endl;
    
      //print_vec(msg_body2);
    
      response_object.response_buffer.insert(response_object.response_buffer.end(),
					     msg_body2.begin(), msg_body2.end());
      cout << "printing msg with body" << endl;
      //print_vec(response_object.response_buffer);
      cout << "Successful Receive" << endl;
      // error checking
    // print_vec(response);
    // cout << response << endl;
      
  }
  return response_object.response_buffer;
   
}


int sendall(const char *buff, int fd, int size) {
  int num_to_send = size;
  while (num_to_send > 0) {
    int num_sent = send(fd, buff, num_to_send, 0);
    buff += num_sent;
    num_to_send -= num_sent;
  }
  return 1;
}

//OPEN A TUNNEL FOR CONNECT
void openTunnel(const char *hostname, const char *port, int user_fd) {
  int serverfd = open_client_socket(hostname, port);
  
  int fdmax = serverfd;
  fd_set master;
  fd_set read_fds;
  FD_ZERO(&master);
  FD_ZERO(&read_fds);
  FD_SET(serverfd, &master);
  FD_SET(user_fd, &master);
  
  //SEND ACK TO FIREFOX THAT A CONNECTION HAS BEEN ESTABLISHED WITH THE ORIGIN SERVER
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
      //cout << "from client" << endl;
      int rec_size = recv(user_fd, v_buffer.data(), 2048, 0);
      //cout << rec_size << endl;
      if (rec_size == -1 | rec_size == 0){
	break;
      }
      sendall(v_buffer.data(), serverfd, rec_size);
    }
    if (FD_ISSET(serverfd, &read_fds)) {
      //cout << "from server" << endl;
      std::vector<char> v_buffer(2048);
      int rec_size = recv(serverfd, v_buffer.data(), 2048, 0);
      //cout << rec_size << endl;
      if (rec_size == -1 | rec_size == 0){
	break;
      }
      sendall(v_buffer.data(), user_fd, rec_size);
    }
  }
  cout << "connect finished" << endl;
  close(user_fd); //when we have multithreading
  close(serverfd);
}









void *process_request(void *uid){
  cout << (long)uid << endl;
    
  long user_fd = (long)uid;
    if (user_fd == -1) {
      perror("accept");
    } 

    cout << "new connection" << endl;
    
    
    cout << "incoming" << endl;
    while(1){
    char http_req_buf[1];
    std::vector<char> request_header;
    size_t line_break_count = 0;

    //MARK : ABSTRACT THIS INTO RECEIVE REQUEST
    while (1) {
      int rec = recv(user_fd, http_req_buf, 1,
		     MSG_WAITALL); // while loop to receive everything
      if(rec == 0){
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
    //MARK END: ABSTRACT THIS INTO RECEIVE REQUEST
    
    //MARK : PUT THIS IN ABOVE FUNCTION 
    HTTPrequest request_obj;
    request_obj.request_buffer = request_header;
    request_obj.set_fields();
    int content_len = request_obj.get_content_length();
    if(content_len >= 0){
      cout << "len = " << content_len << endl;
      std::vector<char> msg_body(content_len);
      int recv_err = recv(user_fd, msg_body.data(), content_len, MSG_WAITALL);
      if (recv_err == -1) {
	cerr << "recv failed" << endl;
	perror("recv");
      }
      cout << "printing msg" << endl;
      request_obj.request_buffer.insert(request_obj.request_buffer.end(),
					msg_body.begin(), msg_body.end());
    }
    //MARK : PUT THIS IN ABOVE FUNCTION
    string port = "80";
    cout << request_obj.http_method << endl;
    //IF GET OR POST WE FORWARD ALONG
    string req = std::string(request_obj.request_buffer.data(), request_obj.request_buffer.size());
    if (request_obj.http_method == "GET"  ||  request_obj.http_method == "POST") {
      //FORWARD REQUEST MAYBE JUST TAKE THE REQUEST???
      vector<char> cache_hit = s_cache.lookup(req);
      if(cache_hit.size() > 0){
	cout << "CACHE HIT" << endl;
	sendall(cache_hit.data(), user_fd, cache_hit.size());
      }
      else{
	std::vector<char> response = 
	  forward_request(request_obj.server.c_str(), port.c_str(), request_obj.request_buffer.data());
	sendall(response.data(), user_fd, response.size());
	cout << "HANGING" << endl;
	s_cache.insert(req, response);
	cout << "DONE HANGING" << endl;
	cout << "Response has been cached." << endl;
      }
    } 
    else{
      //PERFECT
      openTunnel(request_obj.server.c_str(), "443", user_fd);
    }

    }
    pthread_exit(NULL);
    
}
  
















int main(void) {

  // listener socket -- bind to port (80)?
  char port[10] = HTTP_PORT;
  int listener_fd = open_server_socket(NULL, port);
  
  struct sockaddr_storage user_addr;
  socklen_t user_addr_len = sizeof(user_addr);
  long user_fd;
  for (;;) { // daemon loop
    
    user_fd =accept(listener_fd, (struct sockaddr *)&user_addr, &user_addr_len);
    pthread_t *thrd = (pthread_t *) malloc(sizeof(pthread_t));
    int err = pthread_create(thrd, NULL, &process_request, (void *)user_fd);
    if(err){
      cout << "error" << endl;
      exit(-1);
    }
  } // END for(;;)--and you thought it would never end!
  
  return EXIT_SUCCESS;
  // sleep(30); /* wait 30 seconds */
}
