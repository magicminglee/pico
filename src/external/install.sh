#!/bin/bash

PLAFORM=$(uname)

ROOT_DIR_PATH=$(pwd)
INSTALL_ROOT_DIR_PATH=$(pwd)
OPENSSL_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/openssl/"
OPENSSL_LIB_PATH="${OPENSSL_INSTALL_PATH}""/lib"
EVENT_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/libevent/"
CXXOPT_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/cxxopt/"
YAML_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/yaml/"
JSON_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/json/"
PHR_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/phr/"
XLOG_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/xlog/"
PROTOBUF_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/protobuf/"
LUA_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/lua/"
LUA_PROTOBUF_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/luapb/"
HIREDIS_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/hiredis/"
REDISCXX_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/rediscxx/"
SASL2_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/sasl2/"
SASL2_INCLUDE_PATH="${INSTALL_ROOT_DIR_PATH}""/sasl2/include/"
SASL2_LIB_PATH="${INSTALL_ROOT_DIR_PATH}""/sasl2/lib/"
MONGOC_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/mongoc/"
MONGOCXX_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/mongocxx/"
FMT_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/fmt/"
CLICKHOUSE_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/clickhousecxx/"
JWTCPP_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/jwt-cpp/"
NGHTTP2_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/nghttp2/"
LLHTTP_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/llhttp/"
SPDLOG_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/spdlog/"
CATCH2_INSTALL_PATH="${INSTALL_ROOT_DIR_PATH}""/catch2/"

#openssl
rm -rf ${OPENSSL_INSTALL_PATH}
tar xvzf openssl-OpenSSL_1_1_1l.tar.gz
cd openssl-OpenSSL_1_1_1l
./config --prefix=${OPENSSL_INSTALL_PATH}
make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf openssl-OpenSSL_1_1_1l

#hiredis
rm -rf ${HIREDIS_INSTALL_PATH}
tar xvzf hiredis-1.0.0.tar.gz
cd hiredis-1.0.0
make PREFIX=${HIREDIS_INSTALL_PATH} -j4
make PREFIX=${HIREDIS_INSTALL_PATH} install
cd ${ROOT_DIR_PATH}
rm -rf hiredis-1.0.0

#sasl2
rm -rf ${SASL2_INSTALL_PATH}
tar xzvf cyrus-sasl-2.1.27.tar.gz
cd cyrus-sasl-2.1.27
./configure --prefix=${SASL2_INSTALL_PATH}
make && make install
cd ${ROOT_DIR_PATH}
rm -rf cyrus-sasl-2.1.27

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
rm -rf ${PROTOBUF_INSTALL_PATH}
tar xvzf protobuf-cpp-3.12.3.tar.gz
cd protobuf-3.12.3/cmake
cmake -DCMAKE_INSTALL_PREFIX=${PROTOBUF_INSTALL_PATH} -H. -Bbuild
cd build && make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf protobuf-3.12.3

#lua
rm -rf ${LUA_INSTALL_PATH}
mkdir -pv ${LUA_INSTALL_PATH}
tar xzvf lua-5.3.6.tar.gz
cd lua-5.3.6
if [[ $(uname) == 'Darwin' ]]; then
	make macosx install INSTALL_TOP=${LUA_INSTALL_PATH}
elif [[ $(uname) == 'Linux' ]]; then
	sed -i "44,9s#liblua.a#liblua.so#g" Makefile
	sed -i "61i\	\t\$(CC) -shared -ldl -Wl,-soname,liblua.so -o liblua.so \$? -lm \$(MYLDFLAGS)" src/Makefile
	make "MYCFLAGS=-fPIC" linux install INSTALL_TOP=${LUA_INSTALL_PATH}
fi
cd ${ROOT_DIR_PATH}
rm -rf lua-5.3.6

#lua-protobuf
rm -rf ${LUA_PROTOBUF_INSTALL_PATH}
tar xzvf lua-protobuf-0.3.4.tar.gz
cd lua-protobuf-0.3.4
cmake -DCMAKE_INSTALL_PREFIX=${INSTALL_ROOT_DIR_PATH} -H. -Bbuild
cd build && make -j4 && make install
ln -s ${LUA_PROTOBUF_INSTALL_PATH}lib/libpb.so ${LUA_PROTOBUF_INSTALL_PATH}lib/pb.so
cd ${ROOT_DIR_PATH}
rm -rf lua-protobuf-0.3.4

git clone https://github.com/rxi/json.lua.git
cp json.lua/json.lua ${LUA_PROTOBUF_INSTALL_PATH}lib
rm -rf json.lua

git clone https://github.com/Tieske/date.git
mv date/src/date.lua ${LUA_PROTOBUF_INSTALL_PATH}lib
rm -rf date
#export LUA_PATH_5_3=${LUA_PROTOBUF_INSTALL_PATH}lib/?.lua
#export LUA_CPATH_5_3=${LUA_PROTOBUF_INSTALL_PATH}lib/?.so

#rediscxx
rm -rf ${REDISCXX_INSTALL_PATH}
tar xvzf redis-plus-plus-1.3.3.tar.gz
cd redis-plus-plus-1.3.3
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=${HIREDIS_INSTALL_PATH} -DCMAKE_INSTALL_PREFIX=${REDISCXX_INSTALL_PATH} -DREDIS_PLUS_PLUS_CXX_STANDARD=17 -H. -Bbuild
cd build && make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf redis-plus-plus-1.3.3

#mongoc
rm -rf ${MONGOC_INSTALL_PATH}
tar xvzf mongo-c-driver-1.21.1.tar.gz
mkdir mongo-c-driver-1.21.1/cmake-build
cd mongo-c-driver-1.21.1/cmake-build
if [ "${PLAFORM}" = "Darwin" ]; then
	cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=ON -DENABLE_SASL=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${MONGOC_INSTALL_PATH} -DSASL_INCLUDE_DIRS=${SASL2_INCLUDE_PATH} -DSASL_LIBRARIES=${SASL2_LIB_PATH}"libsasl2.dylib" -DOPENSSL_ROOT_DIR=${OPENSSL_INSTALL_PATH} -Dpkgcfg_lib__OPENSSL_crypto=${OPENSSL_LIB_PATH}"libcrypto.dylib" -Dpkgcfg_lib__OPENSSL_ssl=${OPENSSL_LIB_PATH}"libssl.dylib" ..
else
	cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=ON -DENABLE_SASL=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${MONGOC_INSTALL_PATH} -DSASL_INCLUDE_DIRS=${SASL2_INCLUDE_PATH} -DSASL_LIBRARIES=${SASL2_LIB_PATH}"libsasl2.so" -DOPENSSL_ROOT_DIR=${OPENSSL_INSTALL_PATH} -Dpkgcfg_lib__OPENSSL_crypto=${OPENSSL_LIB_PATH}"libcrypto.so" -Dpkgcfg_lib__OPENSSL_ssl=${OPENSSL_LIB_PATH}"libssl.so" ..
fi
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

#phr
rm -rf ${PHR_INSTALL_PATH}
tar xvzf picohttpparser-1.0.tar.gz
cd picohttpparser-1.0
cmake -DCMAKE_INSTALL_PREFIX=${PHR_INSTALL_PATH} -H. -Bbuild
cd build
make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf picohttpparser-1.0

#jwt-cpp
rm -rf ${JWTCPP_INSTALL_PATH}
tar xvzf jwt-cpp-0.6.0.tar.gz
mkdir -pv ${JWTCPP_INSTALL_PATH}
cp jwt-cpp-0.6.0/include ${JWTCPP_INSTALL_PATH} -R
cd ${ROOT_DIR_PATH}
rm -rf jwt-cpp-0.6.0

rm -rf ${NGHTTP2_INSTALL_PATH}
tar xvzf nghttp2-1.48.0.tar.gz
cd nghttp2-1.48.0
./configure --enable-lib-only --prefix=${NGHTTP2_INSTALL_PATH} PKG_CONFIG_PATH=${OPENSSL_LIB_PATH}"pkgconfig"
make -j4
make install
cd ${ROOT_DIR_PATH}
rm -rf nghttp2-1.48.0

rm -rf ${LLHTTP_INSTALL_PATH}
tar xvzf llhttp-release-v6.0.5.tar.gz
cd llhttp-release-v6.0.5
cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=${LLHTTP_INSTALL_PATH} -H. -Bbuild
cd build
make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf llhttp-release-v6.0.5

#clickhouse
rm -rf ${CLICKHOUSE_INSTALL_PATH}
tar xvzf clickhouse-cpp-2.2.1.tar.gz
mkdir -pv ${CLICKHOUSE_INSTALL_PATH}
cp -R clickhouse-cpp-2.2.1/contrib ${CLICKHOUSE_INSTALL_PATH}
mkdir clickhouse-cpp-2.2.1/build
cd clickhouse-cpp-2.2.1/build
cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${CLICKHOUSE_INSTALL_PATH} ..
make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf clickhouse-cpp-2.2.1

#spdlog
rm -rf ${SPDLOG_INSTALL_PATH}
tar xvzf spdlog-1.10.0.tar.gz
cd spdlog-1.10.0
cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${SPDLOG_INSTALL_PATH} -DSPDLOG_BUILD_SHARED=ON -H. -Bbuild
cd build
make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf spdlog-1.10.0

#catch2
rm -rf ${CATCH2_INSTALL_PATH}
tar xvzf Catch2-3.1.0.tar.gz
cd Catch2-3.1.0
cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${CATCH2_INSTALL_PATH} -DBUILD_SHARED_LIBS=ON -H. -Bbuild
cd build
make -j4 && make install
cd ${ROOT_DIR_PATH}
rm -rf Catch2-3.1.0
