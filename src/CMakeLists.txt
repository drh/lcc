add_executable(rcc
               main.c)
               
target_link_libraries(rcc PRIVATE librcc)                  
                  
lburg(${CMAKE_CURRENT_SOURCE_DIR}/dagcheck.md dagcheck.c)
lburg(${CMAKE_CURRENT_SOURCE_DIR}/alpha.md    alpha.c   )
lburg(${CMAKE_CURRENT_SOURCE_DIR}/mips.md     mips.c    )
lburg(${CMAKE_CURRENT_SOURCE_DIR}/sparc.md    sparc.c   )
lburg(${CMAKE_CURRENT_SOURCE_DIR}/x86.md      x86.c     ) 
lburg(${CMAKE_CURRENT_SOURCE_DIR}/x86linux.md x86linux.c)                   
                   
add_library(librcc STATIC    
            alloc.c     
            bind.c
            dag.c
            dagcheck.c
            decl.c
            enode.c
            error.c
            expr.c
            event.c
            init.c
            inits.c
            input.c
            lex.c
            list.c
            main.c
            output.c
            prof.c
            profio.c
            simp.c
            stmt.c
            string.c
            sym.c
            trace.c
            tree.c
            types.c
            null.c
            symbolic.c
            gen.c
            bytecode.c
            alpha.c
            mips.c
            sparc.c
            stab.c
            x86.c
            x86linux.c)
            
target_include_directories(librcc PUBLIC .)

add_library(c INTERFACE)
target_include_directories(c INTERFACE .)

add_library(profio INTERFACE)
target_include_directories(profio INTERFACE .)