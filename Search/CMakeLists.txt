cmake_minimum_required(VERSION 3.20)
project(SearchEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("C:/boost_1_89_0")

include_directories("C:/Users/smr00/libpqxx_install/include")

link_directories("C:/Users/smr00/libpqxx_install/lib")
link_directories("C:/Program Files/PostgreSQL/17/lib")

find_package(OpenSSL REQUIRED)

set(SHARED_SRC
    Config.cpp
    Config.h
    DBase.cpp
    DBase.h
    Spider.cpp
    Spider.h
    SearchServer.cpp
)

add_executable(spider
    ${SHARED_SRC}
    main_spider.cpp
)

add_executable(server
    ${SHARED_SRC}
    main_server.cpp
)

foreach(t spider server)
    target_link_libraries(${t} PRIVATE
        pqxx
        libpq
        ws2_32
        crypt32
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endforeach()
