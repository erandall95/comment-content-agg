cmake_minimum_required(VERSION 3.10)
project(CCA)

# Set C++ standard to C++17 (or any other version you prefer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# Add the executable target for your main project
add_executable(${PROJECT_NAME} main.cpp)

# Add source files from the ./src directory (for your main project)
file(GLOB_RECURSE SOURCES src/*.cpp src/*.c)

# Include the ./src directory for header files (for your main project)
include_directories(src)
include_directories(src/uWebsockets/src)
include_directories(src/uWebSockets/uSockets/src)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/uWebSockets/uSockets)
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/uWebSockets/uSockets/uSockets.a)

# Link any necessary libraries (e.g., if you need to link against OpenSSL)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB OpenSSL::SSL OpenSSL::Crypto)

# curl
target_include_directories(${PROJECT_NAME} PRIVATE /usr/local/opt/curl/include/)
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${CURL_LIBRARIES})

# mongodb
# Find the mongo-cxx-driver package
find_package(mongocxx REQUIRED)
# find_package(bsoncxx REQUIRED)
# Add include directories for mongocxx
target_include_directories(${PROJECT_NAME} PRIVATE ${MONGOCXX_INCLUDE_DIRS})
# Link your executable with the mongocxx libraries
target_link_libraries(${PROJECT_NAME} PRIVATE mongo::mongocxx_shared)

# Use ExternalProject to build /src/llama.cpp separately
include(ExternalProject)
ExternalProject_Add(
    llama_project
    PREFIX ${CMAKE_BINARY_DIR}/external/llama
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/llama.cpp
    BINARY_DIR ${CMAKE_BINARY_DIR}/external/llama-build
    INSTALL_DIR ${CMAKE_BINARY_DIR}/external/llama-install
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DBUILD_SHARED_LIBS=ON
    INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install
    BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/external/llama-install/lib/libllama.so
)
# Link your main project to the llama_cpp library
add_dependencies(${PROJECT_NAME} llama_project)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/llama.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/external/llama-install/lib/libembdinput.dylib)
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/external/llama-install/lib/libggml_shared.dylib)
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/external/llama-install/lib/libllama.dylib)
# copy metal and cuda (if applicable) to library location
add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/external/llama-install/bin/ggml-metal.metal ${CMAKE_BINARY_DIR}/external/llama-install/lib
    COMMENT "Copy metal build file"
)
