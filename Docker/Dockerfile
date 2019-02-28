FROM ubuntu:18.04

RUN mkdir /code
RUN apt-get update && apt-get install -y make && apt-get install -y g++
WORKDIR /code
ADD . /code

