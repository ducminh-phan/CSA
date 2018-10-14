add_library(csa_lib
        config.hpp
        data_structure.cpp data_structure.hpp
        csa.cpp csa.hpp)
add_executable(csa
        main.cpp
        config.hpp
        experiments.cpp experiments.hpp)

target_link_libraries(csa csa_lib)
target_link_libraries(csa z)
set_target_properties(csa PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set_target_properties(csa_lib PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(csa_lib Threads::Threads)
