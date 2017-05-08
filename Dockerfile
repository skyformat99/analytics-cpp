FROM debian:jessie

RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y build-essential
RUN apt-get install -y libcurl4-gnutls-dev
RUN apt-get install -y valgrind

ADD . /src
WORKDIR /src

ENTRYPOINT [ "/usr/bin/make" ]
