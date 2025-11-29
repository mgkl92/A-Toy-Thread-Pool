cmake_minimum_required(VERSION 3.15)

project(mgkl_thread_pool)

set(SRC_LIST
    main.cpp)

set(LIB_LIST
    pthread)

set(INCLUDE_LIST
    include)

set(CXX_COMPLIE_OPTIONS
    -Wall
    -g
    -ggdb)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_compile_options(${PROJECT_NAME} PUBLIC ${CXX_COMPLIE_OPTIONS})

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIB_LIST})

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_LIST})

