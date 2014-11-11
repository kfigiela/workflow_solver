cmake_minimum_required(VERSION 3.0)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -std=c++11")
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
# SET(CMAKE_CXX_FLAGS "-D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3  -std=c++11 -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fno-common -Woverloaded-virtual -Wcast-qual")

link_directories(/usr/local/lib/)
link_directories(/opt/local/lib/) # For Atlas installed via OS X MacPorts 
# link_directories(/usr/local/opt/llvm/lib/) # For Atlas installed via OS X MacPorts 
link_directories(graph_grammar_solver/build)


add_executable(WorkflowSolver WorkflowSolver.cpp Workflow.cpp)
target_link_libraries(WorkflowSolver -ldl -lpthread -lcblas -llapack -lsatlas -ltatlas -lcurses -lSolverLib -lboost_program_options)
include_directories("/usr/local/opt/llvm/include")
