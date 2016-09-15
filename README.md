# mountainfileserver

## Downloading

> git clone https://github.com/magland/mountainfileserver.git

> cd mountainfileserver

## Installing using docker

Install docker (on ubuntu this would be apt-get install docker.io)

Then build the image from the Dockerfile

> sudo docker build -t mfs .

(It will take some time to build)

Then you can run the server in the container using:
> sudo docker run --net="host" -v $PWD/config:/base/config -v $PWD/data:/base/data -it mfs

If instead you want to explore around the container you can add /bin/bash to the end of this command.

Change "-it" to "-t" if you don't want it to stop when the terminal closes.
In that case you will need to stop the container via

> sudo docker ps

> sudo docker kill [name_of_container]

## Testing the installation

Open a web browser and point it to

> http://localhost:8080/hello_world.txt

> http://localhost:8080/hello_world.txt?a=stat

You can replace localhost by the ip of your server if you are on a different machine.

## Configuring the listen port

> cp config/mountainfileserver.ini.example config/mountainfileserver.ini

Then edit this .ini file appropriately.

Rebuild the image, and run the container

## Configuring the data directory

To point the file server to a different base data directory, simply replace
"$PWD/data" in your run command by the absolute path of your data directory.
For example

> -v /path/to/data:/base/data



