include(FetchContent)

FetchContent_Declare(
  chipmunk
  # GIT_REPOSITORY https://github.com/slembcke/Chipmunk2D.git
  GIT_REPOSITORY https://codeberg.org/slembcke/Chipmunk2D.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)

# Inject the policy patch into the chipmunk project scope
set(CMAKE_PROJECT_chipmunk_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/chipmunk2d_patch.cmake")

block()
    # 1. Force shared library (only for this block) but not for emscripten
    if(EMSCRIPTEN)
        set(BUILD_SHARED OFF)

        # Forces the compiler to respect standard IEEE-754 Infinity rules
        add_compile_options(-fno-finite-math-only)
    endif()

    set(BUILD_DEMOS OFF)



    FetchContent_MakeAvailable(chipmunk)
endblock()

# TODO - probably better!!!
# # Inject the Emscripten fixes directly into Chipmunk's target
# if(EMSCRIPTEN)
#   # Prevent undefined behavior bugs by keeping infinity intact
#   target_compile_options(chipmunk PRIVATE -fno-finite-math-only)
  
#   # Or simply quiet the warning if it still logs messages
#   target_compile_options(chipmunk PRIVATE -Wno-nan-infinity-disabled)
# endif()



# add_library( chipmunk::chipmunk ALIAS chipmunk )
# Check if the target is just named 'chipmunk'
# 1. Resolve which target name Chipmunk used internally
if(TARGET chipmunk)
    add_library(chipmunk::chipmunk ALIAS chipmunk)
# Upstream sometimes names it 'chipmunk_static' based on internal logic
elseif(TARGET chipmunk_static) 
    add_library(chipmunk::chipmunk ALIAS chipmunk_static)
elseif(TARGET chipmunk_shared)
    add_library(chipmunk::chipmunk ALIAS chipmunk_shared)
endif()


# 2. Inject the missing include paths so downstream consumers can find them
if(TARGET ${CHIPMUNK_REAL_TARGET})
    target_include_directories(${CHIPMUNK_REAL_TARGET}
        INTERFACE 
            # Exposes the main 'include' directory where chipmunk.h lives
            "${chipmunk_SOURCE_DIR}/include"
    )
    
    # 3. Safe-guard your ALIAS setup as before
    add_library(chipmunk::chipmunk ALIAS ${CHIPMUNK_REAL_TARGET})
endif()