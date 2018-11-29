#!/bin/bash
make build
sudo make install
sudo make clean
sudo make build
sudo make install
cd ./examples
make
mkdir ./tmp_dir  

