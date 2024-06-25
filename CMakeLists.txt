# set C project

project(chatSever)

# set CMake version
cmake_minimum_required(VERSION 3.0)

# set compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set C standart C99
set(CMAKE_C_STANDARD 99)

# set debug mode
set(CMAKE_BUILD_TYPE Debug)


# set include directory
include_directories(include)

# set source file list for server
set(SERVER_FILES
        src/server.c
        src/config.c 
        src/log.c
        src/list.c
        src/timestamp.c
        src/thread_pool.c
        )
        
set(CLIENT_FILES
        src/config.c
        src/client.c)        
        
add_executable(server ${SERVER_FILES})
add_executable(client ${CLIENT_FILES})

TARGET_LINK_LIBRARIES(server
pthread
)