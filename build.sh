#! /bin/bash 
cd iniparser
make -j2
cd -
rm -rf build 
mkdir build
cd build
cmake ../  
make -j2  


