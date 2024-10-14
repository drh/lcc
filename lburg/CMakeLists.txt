
add_executable(lburg 
               lburg.c
               gram.c)
            
target_include_directories(lburg PRIVATE lburg)             
          
# lburg function for generation of c files from md files          
function(lburg md_file c_file)
    add_custom_command(OUTPUT  ${c_file}
                       DEPENDS ${md_file}
                       COMMAND lburg ${md_file} ${c_file}
                       VERBATIM)
endfunction()