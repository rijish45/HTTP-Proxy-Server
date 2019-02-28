Caching Web Proxy Server
========================

**Language**: C++  

**Developers**:
---------------
 1. Prathikshaa Rangarajan  
 2. Rijish Ganguly   
 3. David Laub    


## Current Progress:
* Listen on a socket for incoming requests
* print message to command line
* send() the message back to the user -- echo


## To Do:

* Put the incoming requests into a log file.

* Use vector<char> instead of strings as HTTP responses may contain any type of content-encoding, e.g. g-zip which uses '\0' as part of data and not as a terminator.

* Branch 1: Uncomment Daemon code and multi-threading
--  place accept inside the loop -- span a new thread for every new fd

* Branch 2: A simple proxy server -- caching aside
-- Parse the input request and forward the request and just pass back the response -- no caching yet.

* Step 3:
- Combine both of the above and check it works

* Branch 3:
-- Use the Branch 2 proxy server to build the cache model:
   Step 1: Just save the reponses as files pointed to by a Hash map

## Response Parsing
*Parser should search for below and let us know if a message body exists.*

Yes, the proxy should handle chunked transfers in any case where the transfer encoding field indicates "chunked"
Here's a good overview of chunked transfers (why they're very useful, and an example):
https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Transfer-Encoding
Also, section 4.1 of RFC 7230 describes how to handle chunked transfers (including pseudo-code for decoding a chunked transfer).
_Source:_ https://piazza.com/class/jqkchrj2t002u4?cid=156

## References
https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa  
https://jameshfisher.com/2016/12/20/http-hello-world.html  
https://www.i-programmer.info/programming/cc/9993-c-sockets-no-need-for-a-web-server.html  
https://stackoverflow.com/questions/176409/build-a-simple-http-server-in-c  
https://github.com/LambdaSchool/C-Web-Server/blob/master/guides/lrucache.md  
