
/**
 * Caching Proxy Server in C++
 * February 18, 2019
 * Prathikshaa Rangarajan (pr109), Rijish Ganguly (rg239), David Laub (dgl9)
 */


#ifndef __PROXY_H_
#define __PROXY_H_

#include "HTTPrequest.h"
#include "HTTPresponse.h"
#include "cache.h"

#include <errno.h>
#include <fcntl.h>
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

bool no_cache(HTTPrequest request_obj);
bool no_cache(HTTPresponse request_obj);

bool is_fresh(HTTPrequest request_obj);

HTTPresponse forward(HTTPrequest request_obj);

HTTPresponse validate(HTTPrequest request_obj);

string build_validation_req(string request_line, string etag,
                            string last_modified);

HTTPresponse deal_with_cache(HTTPrequest request);

int forward_request(const char *hostname, const char *port,
                    const char *request);
HTTPresponse receive_response(int server_fd);

int open_server_socket(char *hostname, char *port);
int open_client_socket(const char *hostname, const char *port);
int sendall(const char *buff, int fd, int size);

/*----------------------implementations-------------------*/

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
      //std::cout << "Successfully bound to port " << port << std::endl; // log
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
  //cout << hostname << " " << port << endl;
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

int sendall(const char *buff, int fd, int size) {
  int num_to_send = size;
  while (num_to_send > 0) {
    int num_sent = send(fd, buff, num_to_send, 0);
    buff += num_sent;
    num_to_send -= num_sent;
  }
  return 1;
}

size_t str_to_num(const char *str) {

  //  printf("string: %s\n", str);

  // use strtoul
  char *endptr;
  // check for -ve nos.
  if (str[0] == '-') {
    printf("Invalid Input:\t%s\n", str);
    exit(EXIT_FAILURE);
  }
  errno = 0;
  size_t val = strtoul(str, &endptr, 10);

  if (errno) {
    perror("Invalid Input: ");
    exit(EXIT_FAILURE);
  }

  if (endptr == str) {
    fprintf(stderr, "Invalid Input:\t%s\nNo digits were found.\n", str);
    exit(EXIT_FAILURE);
  }

  return val;
}

#endif
