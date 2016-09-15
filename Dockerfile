############################################################
# Run a mountainfileserver file server
# Run this container using --net="host"
############################################################

# Set the base image to Ubuntu
FROM ubuntu:16.04

# Update the repository sources list
RUN apt-get update

# Install qt5
RUN apt-get install -y software-properties-common
RUN apt-add-repository ppa:ubuntu-sdk-team/ppa
RUN apt-get update
RUN apt-get install -y qtdeclarative5-dev
RUN apt-get install -y qt5-default qtbase5-dev qtscript5-dev make g++

# Install nodejs and npm
RUN apt-get install -y nodejs npm
RUN npm install ini extend

# Make the user
RUN mkdir /home/magland
RUN groupadd -r magland -g 433 && \
useradd -u 431 -r -g magland -d /home/magland -s /sbin/nologin -c "Docker image user" magland && \
chown -R magland:magland /home/magland
RUN apt-get install nano

USER magland
WORKDIR /home/magland
RUN echo "------------------------------------ $PWD"
RUN whoami

# Make the development directory
RUN mkdir -p dev/mountainfileserver
WORKDIR dev/mountainfileserver

# Add the source files
ADD src src
ADD sumit sumit
ADD config config
ADD test_data test_data
USER root
RUN chown -R magland:magland *
USER magland

# Compile sumit
WORKDIR sumit
RUN qmake
RUN make -j 4
WORKDIR ..

CMD ["nodejs","src/mountainfileserver.js"]
