#!/bin/bash

# Define variables
FILE_NAME="Dockerfile.build_python_plugin"
IMAGE_NAME="build_python_binding"
CONTAINER_NAME="l15-build-python-plugin"
BUILD_DIR="/utxord-wallet/core/build/python_binding"
HOST_DIR="./build/python_binding_from_docker"

# Build the Docker image
docker rmi $IMAGE_NAME
docker build -t $IMAGE_NAME -f $FILE_NAME .

# Create and run a temporary container
docker create --name $CONTAINER_NAME $IMAGE_NAME

mkdir -p $HOST_DIR
# Copy the built files from the container to the host
docker cp $CONTAINER_NAME:$BUILD_DIR/share/libutxord_pybind.py $HOST_DIR/libutxord_pybind.py
docker cp $CONTAINER_NAME:$BUILD_DIR/lib/_libutxord_pybind.so $HOST_DIR/_libutxord_pybind.so

# Remove the temporary container
docker rm $CONTAINER_NAME
