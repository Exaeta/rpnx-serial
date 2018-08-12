cmake_minimum_required(VERSION 3.5)

add_library(rpnx-serial INTERFACE)
target_include_directories(rpnx-serial INTERFACE include/)

INSTALL(FILES "include/rpnx/serial_traits.hpp" DESTINATION "include/rpnx" RENAME "serial_traits")
