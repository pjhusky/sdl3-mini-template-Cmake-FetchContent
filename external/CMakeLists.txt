add_subdirectory(SDL3)
add_subdirectory(imgui) # imgui depends on SDL3, so it must be added after SDL3
add_subdirectory(fmt)
add_subdirectory(luajit)
add_subdirectory(sol3) # sol3 depends on a lua impl (here: luajit), so it must be added after luajit

