#!/bin/bash

# this script deploys a bulding processes 

container_name="arm-linux-gnueabihf"

mkdir -p build

sudo docker run --rm \
 -v ${PWD}/..:/home/dev/project:ro \
 -v ${PWD}/build:/home/dev/build:rw \
 -w /home/dev/build \
 ${container_name} \
 bash /home/dev/build_target.sh

