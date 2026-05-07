cmake_minimum_required(VERSION 3.16)
project(boundary_value_problems_for_odes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

set(BVP_SOURCES
    src/solver.cpp
    src/task_utils.cpp
    src/tasks/first_dirichlet_test.cpp
    src/tasks/first_dirichlet_main.cpp
    src/tasks/mixed_test_classic.cpp
    src/tasks/mixed_main_improved.cpp
)

add_executable(bvp_solver_gui
    src/main.cpp
    ${BVP_SOURCES}
)

target_include_directories(bvp_solver_gui PRIVATE include)
target_link_libraries(bvp_solver_gui PRIVATE Qt5::Widgets)

add_executable(bvp_solver_cli
    src/cli_main.cpp
    ${BVP_SOURCES}
)

target_include_directories(bvp_solver_cli PRIVATE include)

if(MSVC)
    target_compile_options(bvp_solver_gui PRIVATE /utf-8)
    target_compile_options(bvp_solver_cli PRIVATE /utf-8)
endif()
