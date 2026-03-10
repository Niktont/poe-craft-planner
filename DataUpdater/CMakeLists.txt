cmake_minimum_required(VERSION 3.22)

qt_add_executable(DataUpdater
    src/main.cpp
    src/DataUpdater.h src/DataUpdater.cpp
)

target_link_libraries(DataUpdater PRIVATE Qt6::Core Qt6::Network Qt6::Sql)

target_compile_definitions(DataUpdater PRIVATE
	APP_VERSION="${PROJECT_VERSION}"
	DB_PATH="${CMAKE_SOURCE_DIR}/additional.sqlite3"
)
