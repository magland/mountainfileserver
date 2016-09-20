# prv

## Downloading

> git clone https://github.com/magland/prv.git

> cd prv

## Building and running web server using docker

Install docker. On ubuntu this would be

> apt-get install docker.io

Then build the image from the Dockerfile

> ./build_prvfileserver.sh

(The first time you run this, it will take some time to build because it downloads ubuntu and qt5)

Create a directory where the data will be stored. Or use an existing directory.

Then you can run the server in the container using:

> ./start_prvfileserver.sh /absolute/path/to/data/directory

> sudo docker ps (to find its name)

> sudo docker kill [its name]

## Testing the installation

First create a file inside your data directory called, say, hello_world.txt

Open a web browser and point it to

> http://localhost:8080/prv/[]

> http://localhost:8080/prv/hello_world.txt?a=stat

> http://localhost:8080/prv/?a=locate&checksum=[fill in]&size=[fill in]

Replace localhost/8080 by the ip/port of your server if you are on a different machine.

## Configuring the listen port, data directory, etc

> cp prv.json.default prv.json

Then edit this file as needed. You should delete any fields for which you want to use the default value.

Now start the container.

Note that prv.json is copied at run time (start_prvfileserver.sh), whereas prv.json.default is copied during build (build_prvfileserver.sh). So you should re-build if that file gets modified.

To stop the web server just use

> ./stop_prvfileserver.sh