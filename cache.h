/**
 * Caching Proxy Server in C++
 * February 18, 2019
 * Prathikshaa Rangarajan (pr109), Rijish Ganguly (rg239), David Laub (dgl9)
 */

#ifndef _CACHE_H
#define _CACHE_H

#include "HTTPrequest.h"
#include "HTTPresponse.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <unordered_map>
#include <vector>

using namespace std;

typedef std::unordered_map<string, std::vector<char>> map;
typedef std::unordered_map<string, HTTPrequest> reqMap;
typedef std::unordered_map<string, HTTPresponse> respMap;
typedef std::unordered_map<string, time_t> timeMap;

class Cache {

public:
  // vector <char> req;
  // vector <char> resource_name;
  map my_cache;
  reqMap request_cache;
  respMap response_cache;
  timeMap time_cache;

  string MRU;
  size_t size;
  size_t max_size;
  void print(void);
  ssize_t insert(string key, vector<char> val, HTTPrequest request,
                 HTTPresponse response);
  ssize_t evictNMRU(void);
  ssize_t evict(string key);
  vector<char> lookup(string key);
  ssize_t update();

  Cache(int max_size) : size(0), max_size(max_size){};
  Cache() : size(0), max_size(4){};
  ~Cache(){};
};

void print_vec(const vector<char> &vec) {
  for (auto x : vec) {
    cout << x;
  }
  cout << endl;
  return;
}

void Cache::print(void) {
  std::cout << "Cache:" << std::endl;
  for (auto it : this->my_cache) {
    std::cout << " " << it.first << ":";
    print_vec(it.second);
    std::cout << std::endl;
  }
  return;
}

ssize_t Cache::insert(string key, vector<char> val, HTTPrequest request,
                      HTTPresponse response) {
  if (this->size == this->max_size) {
    evictNMRU();
  }
  this->my_cache[key] = val;
  request_cache[key] = request;
  time_cache[key] = time(NULL);

  //  request_cache[key].fv_map["REQ_TIME"] = time(NULL); // time

  response_cache[key] = response;

  this->size++;
  this->MRU = key;
  return 0;
}

ssize_t Cache::evictNMRU(void) {
  for (auto it : this->my_cache) {
    if (it.first != this->MRU) {
      this->my_cache.erase(it.first);
      this->response_cache.erase(it.first);
      this->request_cache.erase(it.first);
      this->time_cache.erase(it.first);
      break;
    }
  }

  this->size--;
  return 0;
}

ssize_t Cache::evict(string key) {
  for (auto it : this->my_cache) {
    if (key == it.first) {
      this->my_cache.erase(it.first);
      this->response_cache.erase(it.first);
      this->request_cache.erase(it.first);
      this->time_cache.erase(it.first);
      break;
    }
  }
  if (this->MRU == key) {
    this->MRU = this->my_cache.begin()->first;
  }
  this->size--;
  return 0;
}

vector<char> Cache::lookup(string key) {
  // return error if Cache Miss
  /* if (this->my_cache.find(key) == this->my_cache.end()) { */
  /*   vector<char> fail; */
  /*   return fail; // return ERROR */
  /* } */
  return this->my_cache[key];
}

ssize_t Cache::update() { return 1; }

#endif
