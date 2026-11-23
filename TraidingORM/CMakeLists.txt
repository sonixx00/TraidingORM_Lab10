cmake_minimum_required(VERSION 3.20)

project(TraidingORM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(SqliteOrm CONFIG REQUIRED)

add_executable(TraidingORM
    TraidingORM.cpp
    TraidingORM.h
 "Patterns.h")

target_link_libraries(TraidingORM PRIVATE
    unofficial::sqlite3::sqlite3
    sqlite_orm::sqlite_orm
)
