
/**
 * Caching Proxy Server in C++
 * February 18, 2019
 * Prathikshaa Rangarajan (pr109), Rijish Ganguly (rg239), David Laub (dgl9)
 */



#ifndef _CACHE_H
#define _CACHE_H

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
#include <unordered_map>
#include <vector>
#include <pthread.h>

using namespace std;

typedef std::unordered_map<string, std::vector<char> > map;

pthread_mutex_t my_lock = PTHREAD_MUTEX_INITIALIZER;

class Cache {

public:
  // vector <char> req;
  // vector <char> resource_name;
  map my_cache;
  map expiration_cache;
  string MRU;
  size_t size;
  size_t max_size;
  void print(void);
  ssize_t insert(string key, vector<char> val);
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
  pthread_mutex_lock(&my_lock);
  std::cout << "Cache:" << std::endl;
  for (auto it : this->my_cache) {
    std::cout << " " << it.first << ":";
    print_vec(it.second);
    std::cout << std::endl;
  }
  pthread_mutex_unlock(&my_lock);
  return;
}

ssize_t Cache::insert(string key, vector<char> val) {
  pthread_mutex_lock(&my_lock);
  if (this->size == this->max_size) {
    evictNMRU();
  }
  this->my_cache[key] = val;
  this->size++;
  this->MRU = key;
  pthread_mutex_unlock(&my_lock);
  return 0;
}

ssize_t Cache::evictNMRU(void) {
  pthread_mutex_lock(&my_lock);
  for (auto it : this->my_cache) {
    if (it.first != this->MRU) {
      this->my_cache.erase(it.first);
      break;
    }
  }
  this->size--;
  pthread_mutex_unlock(&my_lock);
  return 0;
}

ssize_t Cache::evict(string key) {
  pthread_mutex_lock(&my_lock);
  for (auto it : this->my_cache) {
    if (key == it.first) {
      this->my_cache.erase(it.first);
      break;
    }
  }
  if (this->MRU == key) {
    this->MRU = this->my_cache.begin()->first;
  }
  this->size--;
  pthread_mutex_unlock(&my_lock);
  return 0;
}

vector<char> Cache::lookup(string key) {
  pthread_mutex_lock(&my_lock);
  // return error if Cache Miss
  if (this->my_cache.find(key) == this->my_cache.end()) {
    vector<char> fail;
    pthread_mutex_unlock(&my_lock);
    return fail; // return ERROR
  }
  pthread_mutex_unlock(&my_lock);
  return this->my_cache[key];
}

ssize_t Cache::update() { return 1; }

#endif
