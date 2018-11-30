#!/bin/bash
make build
MABAIN_INSTALL_DIR=~/mabain make install
cd ./examples
MABAIN_INSTALL_DIR=~/mabain make
mkdir ./tmp_dir  

