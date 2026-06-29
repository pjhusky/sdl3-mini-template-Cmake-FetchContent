# Verify the variable did not leak out of the subdirectory
if(DEFINED CMAKE_POLICY_VERSION_MINIMUM)
    message(WARNING "Leak detected! Policy minimum is still set to: ${CMAKE_POLICY_VERSION_MINIMUM}")
else()
    message(STATUS "SUCCESS: CMAKE_POLICY_VERSION_MINIMUM is safely out of scope.")
endif()


add_subdirectory(SDL3)
add_subdirectory(imgui) # imgui depends on SDL3, so it must be added after SDL3
add_subdirectory(fmt)

add_subdirectory(scripting/lua-based)

add_subdirectory(scripting/quickjs)

add_subdirectory(scripting/gravity)

add_subdirectory(box2d)
add_subdirectory(Chipmunk2D)



# Verify the variable did not leak out of the subdirectory
if(DEFINED CMAKE_POLICY_VERSION_MINIMUM)
    message(WARNING "Leak detected! Policy minimum is still set to: ${CMAKE_POLICY_VERSION_MINIMUM}")
else()
    message(STATUS "SUCCESS: CMAKE_POLICY_VERSION_MINIMUM is safely out of scope.")
endif()

