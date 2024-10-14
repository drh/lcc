add_executable(lcc
               lcc.c)             
               
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")               
    target_sources(lcc PRIVATE win32.c)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(lcc PRIVATE linux.c)
endif()               

add_executable(bprint
               bprint.c)  

target_link_libraries(bprint PRIVATE profio)
               
add_executable(ops EXCLUDE_FROM_ALL
               ops.c)  
               
target_link_libraries(ops PRIVATE c)   