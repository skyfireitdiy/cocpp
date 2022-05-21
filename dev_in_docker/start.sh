#!/bin/bash
set -e
export USERNAME=$(id -nu) 
export USERID=$(id -u) 
export GROUPID=$(id -g)
export GROUPNAME=$(id -gn)
docker-compose up -d $*
docker exec -it cocpp-dev fish
