# prv

## Downloading

> git clone https://github.com/magland/prv.git

> cd prv

## Installing web server using docker

Install docker (on ubuntu this would be apt-get install docker.io)

Then build the image from the Dockerfile

> sudo docker build -t prv .

(The first time you run this, it will take some time to build because it downloads ubuntu and qt5)

Then you can run the server in the container using:
> sudo docker run --net="host" -v $PWD/prv.json:/base/prv.json -v $PWD/data:/base/data -it prv nodejs prvfileserver/prvfileserver.js

or simply run

> ./start_prvfileserver.sh /absolute/path/to/data/directory

If instead you want to explore around the container you can replace the nodejs command at the end by /bin/bash.

Change "-it" to "-t" if you don't want it to stop when the terminal closes (or Ctr+C is pressed).
In that case you will need to stop the container via

> sudo docker ps (to find its name)

> sudo docker kill [its name]

## Testing the installation

Open a web browser and point it to

> http://localhost:8080/prv/hello_world.txt

> http://localhost:8080/prv/hello_world.txt?a=stat

> http://localhost:8080/prv/?a=locate&checksum=[fill in]&size=[fill in]

Replace localhost by the ip of your server if you are on a different machine.

## Configuring the listen port, data directory, etc

> cp prv.json.default prv.json

Then edit this file as needed. You should delete any fields for which you want to use the default value.

You should not change the value of "data_directory" since the data directory on the host will get mapped into /base/data in the container.

Rebuild the image, and run the container. Note that the option "-v $PWD/prv.json:/base/prv.json" moves this configuration to the container at run time, whereas prv.json.default is maintained in the git repo and is added to the container/image at build time.




