#!/bin/bash

# Before running this command, set up the web server using the instructions in the README.md

# Pass the absolute data directory path in as the first argument. By default it is $PWD/data.
# This then gets mapped to the /base/data directory of the container, so you should NOT change data_directory in the prv.json

# To override the listen port, use --listen_port=8081

abs_data_directory=${1-$PWD/data}
shift #consume the first argument -- pass the rest to prvfileserver.js

sudo docker run --net="host" -v $PWD/prv.json:/base/prv.json -v $abs_data_directory:/base/data -it prv nodejs prvfileserver/prvfileserver.js "$@"
