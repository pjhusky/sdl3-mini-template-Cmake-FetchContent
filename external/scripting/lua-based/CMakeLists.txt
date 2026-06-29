add_subdirectory(lua)
add_subdirectory(sol3) # sol3 depends on a lua impl (here: luajit), so it must be added after luajit
