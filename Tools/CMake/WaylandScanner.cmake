# Generate glue code and client header from wayland protocol xml file.
# TARGET: target to link library with.
# PHASE: protocol phase. e.g. stable, staging, unstable
# PROTOCOL: protocol directory name. e.g. "xdg_shell"
# XML_FILE: optional. Protocol xml name, if different from directory name. e.g. "xdg_shell.xml"
# Call to wayland_scanner(PHASE "stable" PROTOCOL "xdg_shell", "xdg_shell")
# will run wayland-scanner on stable/xdg-shell/xdg-shell.xml
# Call to wayland_scanner(PHASE "staging" PROTOCOL "xwayland-shell" XML_FILE "xwayland-shell-v1.xml")
# will run wayland-scanner on staging/xwayland-shell/xwayland-shell-v1.xml
macro(wayland_scanner)
    set(options "")
    set(oneValueArgs TARGET PHASE PROTOCOL XML_FILE)
    set(multiValueArgs "")
    cmake_parse_arguments(arg_wldscan
        "${options}" "${oneValueArgs}" "${multiValueArgs}"
        ${ARGN}
    )

    # Check arguments
    if(NOT DEFINED arg_wldscan_TARGET)
        message(FATAL_ERROR "TARGET must be set")
    endif()

    if(NOT DEFINED arg_wldscan_PHASE)
        message(FATAL_ERROR "PHASE must be set (stable, staging, unstable)")
    endif()

    if(NOT DEFINED arg_wldscan_PROTOCOL)
        message(FATAL_ERROR "PROTOCOL must be set")
    endif()

    if(NOT DEFINED arg_wldscan_XML_FILE)
        string(CONCAT arg_wldscan_XML_FILE ${arg_wldscan_PROTOCOL} ".xml")
    endif()

    # Generate protocol files
    include(FindPkgConfig)
    pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

    set(PATH_TO_XML ${WAYLAND_PROTOCOLS_DIR}/${arg_wldscan_PHASE}/${arg_wldscan_PROTOCOL}/${arg_wldscan_XML_FILE})
    message(STATUS "Generating files for Wayland protocol ${PATH_TO_XML}")

    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/${arg_wldscan_PROTOCOL})
    file(MAKE_DIRECTORY ${OUTPUT_DIR})
    set(OUTPUT_FILES ${OUTPUT_DIR}/${arg_wldscan_PROTOCOL}-protocol.h ${OUTPUT_DIR}/${arg_wldscan_PROTOCOL}-protocol.c)

    add_custom_command(
        OUTPUT ${OUTPUT_FILES}
        WORKING_DIRECTORY ${OUTPUT_DIR}
        DEPENDS ${PATH_TO_XML}
        COMMAND wayland-scanner ARGS client-header ${PATH_TO_XML} ${arg_wldscan_PROTOCOL}-protocol.h
        COMMAND wayland-scanner ARGS public-code ${PATH_TO_XML} ${arg_wldscan_PROTOCOL}-protocol.c
    )
    add_custom_target(WaylandScanner ALL DEPENDS ${OUTPUT_FILES})
    add_library(WaylandProtocolLib STATIC ${OUTPUT_FILES})
    target_include_directories(WaylandProtocolLib PUBLIC ${OUTPUT_DIR})
    add_dependencies(WaylandProtocolLib WaylandScanner)

    # add_dependencies(${arg_wldscan_TARGET} WaylandProtocol)
    target_link_libraries(${arg_wldscan_TARGET} WaylandProtocolLib)
endmacro()