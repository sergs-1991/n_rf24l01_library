
# the minimal started image we're basing on
FROM debian

RUN apt-get update && apt-get install -y \
    cmake \
    pkg-config  \ 
    gcc-arm-linux-gnueabihf \    
    g++-arm-linux-gnueabihf \    
    && rm -rf /var/lib/apt/lists/*
 
# provide an uid (-u) as your main user on the host to be
# able to deal with files generated within the container 
# by the user created at this line;
# a primary group has to be created/specified, look at 
# the documentation for the RUN command
RUN groupadd -f -g 1000 dev && \
    useradd -g 1000 -m -N -s /bin/bash -u 1000 dev
    
WORKDIR /home/dev

# srcs are relative to the context directory
# dsts are relative to the working directory
COPY arm-linux-gnueabihf-cmake-toolchain.cmake .
COPY build_target.sh .

# set a default user you'll log under when you're
# going to run this image, /bin/bash will be executed
# unless otherwise specified
USER 1000

ENV TARGET_NAME arm-linux-gnueabihf
ENV CMAKE_TOOLCHAIN_FILE /home/dev/arm-linux-gnueabihf-cmake-toolchain.cmake
