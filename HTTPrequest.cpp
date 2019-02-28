
/**
 * Caching Proxy Server in C++
 * February 18, 2019
 * Prathikshaa Rangarajan (pr109), Rijish Ganguly (rg239), David Laub (dgl9)
 */



#include "HTTPrequest.h"
#include "cache.h"

int main(void){
  HTTPrequest request;
  
  string req  = "GET http://www.rabihyounes.com/ HTTP/1.1\r\n" 
  "Host: www.rabihyounes.com\r\n"
    "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.13; rv:61.0) Gecko/20100101 Firefox/61.0\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
"Accept-Language: en-US,en;q=0.5\r\n"
"Accept-Encoding: gzip, deflate\r\n"
"Connection: keep-alive\r\n"
"Upgrade-Insecure-Requests: 1\r\n\r\n";
  
  request.set_header(req);
  print_vec(request.request_buffer);
  request.set_fields();
  return EXIT_SUCCESS;
}
