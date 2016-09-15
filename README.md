# mountainfileserver

## Downloading

> git clone https://github.com/magland/mountainfileserver.git

> cd mountainfileserver

## Installing using docker

> sudo apt-get install docker.io

> sudo docker build -t mfs .

(It will take some time to build)

Then you can run the server in the container using:
> sudo docker run --net="host" -it mfs

Change "-it" to "-t" if you don't want it to stop when the terminal closes.
In that case you will need to stop the container via

> sudo docker ps

> sudo docker kill [name_of_container]

## Configuring the listen port

> cp config/mountainfileserver.ini.example config/mountainfileserver.ini

Then edit this .ini file appropriately.

Rebuild the image, and run the container

