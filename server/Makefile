TARGETS=proxy_daemon
CC=g++
CCFLAGS=-ggdb3 -Wall -Werror -pedantic -std=c++11 -pthread

.PHONY: all clean clobber

all: $(TARGETS)

proxy_daemon: proxy_daemon.cpp cache.h HTTPrequest.h HTTPresponse.h proxy_daemon.h
	$(CC) $(CCFLAGS) -o $@ $<

cache: cache.cpp cache.h
	$(CC) $(CCFLAGS) -o $@ $<

clean:
	rm -rf *~ $(TARGETS) *.dSYM

clobber:
	rm -f *~
