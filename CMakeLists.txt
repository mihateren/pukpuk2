cmake_minimum_required(VERSION 3.10)

project(ManageMainAndNode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Добавим флаги компиляции
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall")

# Если ZeroMQ в системных репозиториях, можно просто:
# find_package(cppzmq REQUIRED)
# А если вы используете локальную сборку zeromq-4.2.1, указывайте пути:
set(ZEROMQ_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/zeromq-4.2.1/include")
set(ZEROMQ_LIB_DIR     "${CMAKE_SOURCE_DIR}/zeromq-4.2.1/lib")

# Подключим папку include/ и ZeroMQ
include_directories(
    ${ZEROMQ_INCLUDE_DIR}
    "${CMAKE_SOURCE_DIR}/include"
)
link_directories(${ZEROMQ_LIB_DIR})

# Исходники для "manage_main"
set(SOURCES_MAIN
    src/tree.cpp          # код "дерева общего вида"
    src/manage_node.cpp   # класс, управляющий сокетами PUB/SUB
    src/manage_main.cpp   # сама "главная" программа (интерфейс)
)

# Исходники для "node"
# (если у вас node.cpp с логикой fork'а, exec'а и т. п.)
set(SOURCES_NODE
    src/node.cpp
)

# Создаём исполняемый файл manage_main
add_executable(manage_main ${SOURCES_MAIN})
target_link_libraries(manage_main zmq)

# Создаём исполняемый файл node (если нужно отдельное "node")
add_executable(node ${SOURCES_NODE})
target_link_libraries(node zmq)

# Дополнительная цель "trace" (strace)
add_custom_target(trace
    COMMAND strace -o trace_output.txt ./manage_main
    DEPENDS manage_main
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running strace on manage_main, output saved to trace_output.txt"
)
