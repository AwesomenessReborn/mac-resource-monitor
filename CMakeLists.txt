cmake_minimum_required(VERSION 3.15)
project(ResourceMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS frameworks
find_library(IOKIT_LIBRARY IOKit REQUIRED)
find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)

# ftxui (installed via Homebrew)
find_package(ftxui REQUIRED PATHS /opt/homebrew/opt/ftxui/lib/cmake/ftxui)

add_executable(resource-monitor
    src/main.cpp
    src/metrics/cpu_metrics.cpp
)

target_include_directories(resource-monitor PRIVATE src)

target_link_libraries(resource-monitor
    ${IOKIT_LIBRARY}
    ${COREFOUNDATION_LIBRARY}
    ftxui::screen
    ftxui::dom
    ftxui::component
)
