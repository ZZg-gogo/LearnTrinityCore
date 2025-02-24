cmake_minimum_required(VERSION 2.6)

project(LearnTrinityCore)

# 添加编译标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -ggdb -Wall -O0 -Wno-unused-variable -pthread")


# 添加库文件搜索路径
link_directories(
    ${PROJECT_SOURCE_DIR}/lib
    /usr/local/lib/
    /usr/lib/x86_64-linux-gnu/
    /usr/include/mysql
)

# 查找 Boost 库，并确保包含 thread 和 system 库
find_package(Boost REQUIRED COMPONENTS thread system)
# 查找 fmt 库
find_package(fmt REQUIRED)

# 定义需要链接的库列表
set(LIBRARIES Boost::thread Boost::system mysqlclient fmt::fmt)

# 数据库相关源文件
set(DATABASE_SRC
../Database/MySQLConnection.cpp
../Database/Field.cpp
../Database/QueryResult.cpp
)


# 线程相关源文件
set(Threading_SRC

)


# 工具类相关源文件
set(UTIL_SRC
../Utilities/Util.cpp
)


set(src
    ../main.cpp
)


# 创建可执行文件
add_executable(LearnTrinityCore ${src})


# 创建可执行文件
add_executable(ThreadPoolTest ../test/ThreadPoolTest.cpp)


# 创建可执行文件
add_executable(ProducerConsumerQueueTest ../test/ProducerConsumerQueueTest.cpp )

# 创建可执行文件
add_executable(MysqlConnectionTest ../test/MysqlConnectionTest.cpp ${UTIL_SRC} ${DATABASE_SRC})
# 链接 Boost 线程库和系统库
target_link_libraries(MysqlConnectionTest ${LIBRARIES} )

if(0)

endif()



# 链接 Boost 线程库和系统库
target_link_libraries(LearnTrinityCore ${LIBRARIES})
