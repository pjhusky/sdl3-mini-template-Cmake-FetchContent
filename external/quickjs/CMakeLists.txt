# either use CMake-enabled https://github.com/quickjs-ng/quickjs
# or the original https://github.com/bellard/quickjs


if ( TRUE )
    message(STATUS "QuickJS using the NG CMake wrapper")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/quickjs-ng")
else()
    # 1. Extract the strings from the global list using real index numbers
    list(GET QUICKJS_MODE_CHOICES 0 MODE_QUICKJS_ORIGINAL)  # Luajit_OriginalExternalAdd
    list(GET QUICKJS_MODE_CHOICES 1 MODE_QUICKJS_NG)  # Luajit_WohlsoftCmakeWrapper

    # 2. Validate user command-line input
    list(FIND QUICKJS_MODE_CHOICES "${QUICKJS_MODE}" QUICKJS_MODE_INDEX)
    if(QUICKJS_MODE_INDEX EQUAL -1)
        message(FATAL_ERROR "Unsupported QUICKJS_MODE: '${QUICKJS_MODE}'. Choose from: ${QUICKJS_MODE_CHOICES}")
    endif()

    # 3. Use the helper variables for your conditional logic
    if(QUICKJS_MODE STREQUAL MODE_QUICKJS_ORIGINAL)
        message(STATUS "QuickJS using the original repo via homebrewed ExternalProject_Add()")
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/bellard")
    elseif(QUICKJS_MODE STREQUAL MODE_QUICKJS_NG)
        message(STATUS "QuickJS using the NG CMake wrapper")
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/quickjs-ng")
    else()
        message(ERROR "invalid QuickJS lib/build mode selected!")
    endif()
endif()
