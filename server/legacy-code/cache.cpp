/**
 * Caching Proxy Server in C++
 * February 18, 2019
 * Prathikshaa Rangarajan (pr109), Rijish Ganguly (rg239), David Laub (dgl9)
 */




#include "cache.h"

int main(void) {
  Cache cache1; // init
  vector<char> str;
  str.push_back('a');
  cache1.insert("1", str);
  cache1.insert("2", str);
  cache1.insert("3", str);
  cache1.insert("4", str);
  cache1.insert("5", str);
  cache1.insert("6", str);
  cache1.print();
  cout << "---------------------" << endl;
  cache1.evict("6");
  cache1.print();
  cout << "---------------------" << endl;
  cache1.insert("7", str);
  cache1.print();
  cout << "---------------------" << endl;
  vector<char> response = cache1.lookup("hello");
  print_vec(response);
  if (response.size() == 0) {
    cout << "Cache Miss" << endl;
  }

  return EXIT_SUCCESS;
}
