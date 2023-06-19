cmake_minimum_required(VERSION 3.0)
project(app-plugin)

set(SRC
    "main.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC})
