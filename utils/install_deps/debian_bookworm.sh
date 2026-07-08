#! /bin/bash

echo "running installlation of dependencies"

# mkdir
mkdir -p abseil-build abseil-install deps-install protobuf-build boost-build 

#install boost
wget https://archives.boost.io/release/1.82.0/source/boost_1_82_0.tar.gz
tar -xf boost_1_82_0.tar.gz  
cd boost_1_82_0
./bootstrap.sh --with-libraries="thread" --prefix=../deps-install
./b2 link=static -a
./b2 install
cd ..

echo "boost finished"

# install abseil:
git clone -b 20240722.0 https://github.com/abseil/abseil-cpp.git
cmake -B ./abseil-build -S abseil-cpp -DCMAKE_INSTALL_PREFIX=./abseil-install -DABSL_PROPAGATE_CXX_STD=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=20
cmake --build ./abseil-build --target install -- -j2


#install protobuf
git clone -b v29.0-rc1 https://github.com/protocolbuffers/protobuf.git
cd protobuf && git submodule update --init --recursive && cd ..
cmake -B ./protobuf-build -S protobuf -DCMAKE_INSTALL_PREFIX=./deps-install -DCMAKE_RPEFIX_PATH=~/software/abseil -Dprotobuf_LOCAL_DEPENDENCIES_ONLY=ON -DABSL_PROPAGATE_CXX_STD=ON -Dprotobuf_WITH_ZLIB=ON
cmake --build ./protobuf-build --target install -- -j2
