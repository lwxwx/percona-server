cmake_minimum_required(VERSION 2.8.10)

project("id-increment")

list(APPEND CMAKE_INCLUDE_PATH $ENV{BRPC_LEVELDB_INCLUDE})
list(APPEND CMAKE_INCLUDE_PATH $ENV{BRPC_PROTOBUF_INCLUDE})
list(APPEND CMAKE_INCLUDE_PATH $ENV{BRPC_GFLAGS_INCLUDE})
list(APPEND CMAKE_INCLUDE_PATH $ENV{BRPC_SSL_INCLUDE})
list(APPEND CMAKE_INCLUDE_PATH $ENV{BRPC_INCLUDE})

list(APPEND CMAKE_LIBRARY_PATH $ENV{BRPC_LEVELDB_LIB})
list(APPEND CMAKE_LIBRARY_PATH $ENV{BRPC_PROTOBUF_LIB})
list(APPEND CMAKE_LIBRARY_PATH $ENV{BRPC_GFLAGS_LIB})
list(APPEND CMAKE_LIBRARY_PATH $ENV{BRPC_SSL_LIB})
list(APPEND CMAKE_LIBRARY_PATH $ENV{BRPC_LIB})

Message(${CMAKE_INCLUDE_PATH})

option(LINK_SO "Whether examples are linked dynamically" ON)

# CPP_FLAGS
set(CMAKE_CPP_FLAGS "${DEFINE_CLOCK_GETTIME} -DGFLAGS_NS=${GFLAGS_NS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -DNDEBUG -O2 -D__const__= -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer")
if(CMAKE_VERSION VERSION_LESS "3.1.3")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
else()
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

find_library(THRIFT_LIB NAMES thrift)
if (NOT THRIFT_LIB)
    set(THRIFT_LIB "")
endif()
find_library(THRIFTNB_LIB NAMES thriftnb)
if (NOT THRIFTNB_LIB)
    set(THRIFTNB_LIB "")
endif()

find_path(BRPC_INCLUDE_PATH NAMES brpc/server.h)
if(LINK_SO)
    find_library(BRPC_LIB NAMES brpc)
else()
    find_library(BRPC_LIB NAMES libbrpc.a brpc)
endif()
if((NOT BRPC_INCLUDE_PATH) OR (NOT BRPC_LIB))
    message(FATAL_ERROR "Fail to find brpc")
endif()
include_directories(${BRPC_INCLUDE_PATH})

SET(Protobuf_INCLUDE_DIR $ENV{BRPC_PROTOBUF_INCLUDE})
SET(Protobuf_LIBRARY $ENV{BRPC_PROTOBUF_LIB})
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER id.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_path(GFLAGS_INCLUDE_PATH gflags/gflags.h)
find_library(GFLAGS_LIBRARY NAMES gflags libgflags)
if((NOT GFLAGS_INCLUDE_PATH) OR (NOT GFLAGS_LIBRARY))
    message(FATAL_ERROR "Fail to find gflags")
endif()
include_directories(${GFLAGS_INCLUDE_PATH})

find_path(LEVELDB_INCLUDE_PATH NAMES leveldb/db.h)
find_library(LEVELDB_LIB NAMES leveldb)
if ((NOT LEVELDB_INCLUDE_PATH) OR (NOT LEVELDB_LIB))
    message(FATAL_ERROR "Fail to find leveldb")
endif()
include_directories(${LEVELDB_INCLUDE_PATH})

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(OPENSSL_ROOT_DIR
        "/usr/local/opt/openssl"    # Homebrew installed OpenSSL
        )
endif()
include_directories(${OPENSSL_INCLUDE_DIR})
find_package(OpenSSL)

# Message(STATUS ${PROTOBUF_LIBRARIES})

set(DYNAMIC_LIB
    pthread
    ${CMAKE_THREAD_LIBS_INIT}
    ${GFLAGS_LIBRARY}
    ${PROTOBUF_LIBRARIES}
    ${LEVELDB_LIB}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
    ${THRIFT_LIB}
    ${THRIFTNB_LIB}
    dl
    )

add_executable(client client.cpp ${PROTO_HEADER} ${PROTO_SRC})
add_executable(server server.cpp ${PROTO_HEADER} ${PROTO_SRC})

target_link_libraries(client  ${DYNAMIC_LIB} ${BRPC_LIB})
target_link_libraries(server  ${DYNAMIC_LIB} ${BRPC_LIB})