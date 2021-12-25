#!/bin/bash
./build_docker_image.sh
echo docker run -it --rm -u $(id -u):$(id -g) -v $(pwd):$(pwd) -w $(pwd) cocpp_build:lastest 
docker run -it --rm -u $(id -u):$(id -g) -v $(pwd):$(pwd) -w $(pwd) cocpp_build:lastest 