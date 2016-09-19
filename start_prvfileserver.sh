#!/bin/bash

# Before running this command, set up the web server using the instructions in the README.md

# Pass the absolute data directory path in as the first argument. By default it is $PWD/data.
# This then gets mapped to the /base/data directory of the container, so you should NOT change data_directory in the prv.json

# To override the listen port, use --listen_port=8081

abs_data_directory=$1

if [ -z "$abs_data_directory" ];
then
  echo "You must specify the absolute data directory path as the first argument."
  exit -1
fi

shift #consume the first argument -- pass the rest to prvfileserver.js

sudo docker kill prv_container
sudo docker rm prv_container

cmd="nodejs prvfileserver/prvfileserver.js"

args="--name=prv_container --net=host -v $abs_data_directory:/base/data"
if [ -f "$PWD/prv.json" ];
then
  args="$args -v $PWD/prv.json:/base/prv.json"
fi

echo sudo docker run $args -t prv $cmd "$@"
sudo docker run $args -t prv $cmd "$@"
