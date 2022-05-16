#!/bin/bash

cd dev_in_docker
docker-compose up -d
docker exec -it cocpp-dev bash
