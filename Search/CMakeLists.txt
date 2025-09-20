cmake_minimum_required(VERSION 3.20)
project(Search)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Пути к Boost (если ставил вручную)
include_directories("C:/boost_1_89_0")

# Пути к установленному libpqxx
include_directories("C:/Users/smr00/libpqxx_install/include")

# Пути к библиотекам libpqxx и PostgreSQL
link_directories("C:/Users/smr00/libpqxx_install/lib")
link_directories("C:/Program Files/PostgreSQL/17/lib")

# Подключаем OpenSSL через vcpkg
find_package(OpenSSL REQUIRED)

# Исходники проекта
add_executable(Search
    main.cpp
    Config.cpp
    DBase.cpp
    DBase.h
    Spider.cpp
    Spider.h
)

# Линковка
target_link_libraries(Search PRIVATE
    pqxx
    libpq
    ws2_32
    crypt32        # нужно для OpenSSL на Windows
    OpenSSL::SSL
    OpenSSL::Crypto
)
