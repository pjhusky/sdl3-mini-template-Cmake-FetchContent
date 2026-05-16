include(FetchContent)

FetchContent_Declare(
  chipmunk
  GIT_REPOSITORY https://github.com/slembcke/Chipmunk2D.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)

# Inject the policy patch into the chipmunk project scope
set(CMAKE_PROJECT_chipmunk_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/chipmunk2d_patch.cmake")

FetchContent_MakeAvailable(chipmunk)

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