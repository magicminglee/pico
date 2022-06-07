#!/bin/bash

ROOT_DIR_PATH=`pwd`
OPENSSL_INSTALL_PATH=`pwd`"/openssl/"
EVENT_INSTALL_PATH=`pwd`"/libevent/"
CXXOPT_INSTALL_PATH=`pwd`"/cxxopt/"
YAML_INSTALL_PATH=`pwd`"/yaml/"
JSON_INSTALL_PATH=`pwd`"/json/"
PHR_INSTALL_PATH=`pwd`"/phr/"
XLOG_INSTALL_PATH=`pwd`"/xlog/"
#PROTOBUF_INSTALL_PATH=`pwd`"/protobuf/"
HIREDIS_INSTALL_PATH=`pwd`"/hiredis/"
REDISCXX_INSTALL_PATH=`pwd`"/rediscxx/"
SASL2_INSTALL_PATH=`pwd`"/sasl2/"
SASL2_INCLUDE_PATH=`pwd`"/sasl2/include/"
SASL2_LIB_PATH=`pwd`"/sasl2/lib/"
MONGOC_INSTALL_PATH=`pwd`"/mongoc/"
MONGOCXX_INSTALL_PATH=`pwd`"/mongocxx/"
FMT_INSTALL_PATH=`pwd`"/fmt/"
#CLICKHOUSE_INSTALL_PATH=`pwd`"/clickhousecxx/"

<<comment
#openssl
tar xvzf openssl-OpenSSL_1_1_1l.tar.gz
cd openssl-OpenSSL_1_1_1l
./config --prefix=${OPENSSL_INSTALL_PATH}
make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf openssl-OpenSSL_1_1_1l

#libevent
rm -rf ${EVENT_INSTALL_PATH}
tar xvzf libevent-2.1.12-stable.tar.gz
cd libevent-2.1.12-stable
cmake -DOPENSSL_ROOT_DIR=${OPENSSL_INSTALL_PATH} -DCMAKE_INSTALL_PREFIX=${EVENT_INSTALL_PATH} -H. -Bbuild
cd build && make && make install
cd ${ROOT_DIR_PATH}
rm -rf libevent-2.1.12-stable

#cxxopt
rm -rf ${CXXOPT_INSTALL_PATH}
tar xvzf cxxopts-2.2.1.tar.gz
cd cxxopts-2.2.1
cmake -DCMAKE_INSTALL_PREFIX=${CXXOPT_INSTALL_PATH} -H. -Bbuild
cd build && make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf cxxopts-2.2.1

#yaml-cpp
rm -rf ${YAML_INSTALL_PATH}
tar xvzf yaml-cpp-yaml-cpp-0.6.3.tar.gz
cd yaml-cpp-yaml-cpp-0.6.3
cmake -DCMAKE_INSTALL_PREFIX=${YAML_INSTALL_PATH} -DYAML_BUILD_SHARED_LIBS=ON -H. -Bbuild
cd build && make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf yaml-cpp-yaml-cpp-0.6.3

#json
rm -rf ${JSON_INSTALL_PATH}
tar xvzf json-3.9.1.tar.gz
cd json-3.9.1
cmake -DCMAKE_INSTALL_PREFIX=${JSON_INSTALL_PATH} -H. -Bbuild
cd build && make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf json-3.9.1

#protobuf
#rm -rf ${PROTOBUF_INSTALL_PATH}
#tar xvzf protobuf-cpp-3.12.3.tar.gz
#cd protobuf-3.12.3/cmake
#cmake -DCMAKE_INSTALL_PREFIX=${PROTOBUF_INSTALL_PATH} -H. -Bbuild
#cd build && make -j4 && make install
#cd ${ROOT_DIR_PATH}
#rm -rf protobuf-3.12.3

#hiredis
rm -rf ${HIREDIS_INSTALL_PATH}
tar xvzf hiredis-1.0.0.tar.gz
cd hiredis-1.0.0
make PREFIX=${HIREDIS_INSTALL_PATH} -j4
make PREFIX=${HIREDIS_INSTALL_PATH} install
cd ${ROOT_DIR_PATH}
rm -rf hiredis-1.0.0

#rediscxx
rm -rf ${REDISCXX_INSTALL_PATH}
tar xvzf redis-plus-plus-1.3.3.tar.gz
cd redis-plus-plus-1.3.3
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=${HIREDIS_INSTALL_PATH} -DCMAKE_INSTALL_PREFIX=${REDISCXX_INSTALL_PATH} -DREDIS_PLUS_PLUS_CXX_STANDARD=17 -H. -Bbuild
cd build && make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf redis-plus-plus-1.3.3

#sasl2
rm -rf ${SASL2_INSTALL_PATH}
tar xzvf cyrus-sasl-2.1.27.tar.gz
cd cyrus-sasl-2.1.27
./configure --prefix=${SASL2_INSTALL_PATH}
make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf cyrus-sasl-2.1.27

#mongoc
rm -rf ${MONGOC_INSTALL_PATH}
tar xvzf mongo-c-driver-1.21.1.tar.gz 
mkdir mongo-c-driver-1.21.1/cmake-build
cd mongo-c-driver-1.21.1/cmake-build
cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=ON -DENABLE_SASL=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${MONGOC_INSTALL_PATH} -DSASL_INCLUDE_DIRS=${SASL2_INCLUDE_PATH} -DSASL_LIBRARIES=${SASL2_LIB_PATH}"libsasl2.so" -DOPENSSL_ROOT_DIR=${OPENSSL_INSTALL_PATH} -Dpkgcfg_lib__OPENSSL_crypto=${OPENSSL_LIB_PATH}"libcrypto.so" -Dpkgcfg_lib__OPENSSL_ssl=${OPENSSL_LIB_PATH}"libssl.so" ..
make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf mongo-c-driver-1.21.1

#mongocxx
rm -rf ${MONGOCXX_INSTALL_PATH}
tar xvzf mongo-cxx-driver-r3.6.7.tar.gz
cd mongo-cxx-driver-r3.6.7/build
cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_CXX_STANDARD=17 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${MONGOCXX_INSTALL_PATH} -DCMAKE_PREFIX_PATH=${MONGOC_INSTALL_PATH} ..
make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf mongo-cxx-driver-r3.6.7

#fmt
rm -rf ${FMT_INSTALL_PATH}
tar xvzf fmt-8.1.1.tar.gz
cd fmt-8.1.1
cmake -DBUILD_SHARED_LIBS=TRUE -DCMAKE_INSTALL_PREFIX=${FMT_INSTALL_PATH} -H. -Bbuild
cd build
make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf fmt-8.1.1
comment

#phr
rm -rf ${PHR_INSTALL_PATH}
tar xvzf picohttpparser-1.0.tar.gz
cd picohttpparser-1.0
cmake -DCMAKE_INSTALL_PREFIX=${PHR_INSTALL_PATH} -H. -Bbuild
cd build
make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf picohttpparser-1.0


#xlog
rm -rf ${XLOG_INSTALL_PATH}
tar xvzf xlogcxx-1.0.tar.gz
cd xlogcxx-1.0
cmake -DCMAKE_INSTALL_PREFIX=${XLOG_INSTALL_PATH} -H. -Bbuild
cd build
make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf xlogcxx-1.0


#clickhouse
#tar xvzf clickhouse-cpp-2.1.0.tar.gz
#mkdir clickhouse-cpp-2.1.0/build
#cd clickhouse-cpp-2.1.0/build
#cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${CLICKHOUSE_INSTALL_PATH} ..
#make -j4 && make install
#cd ${ROOT_DIR_PATH}
#rm -rf clickhouse-cpp-2.1.0
