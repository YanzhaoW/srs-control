if(APPLE)
    set(BASE_DIR "@loader_path")
else()
    set(BASE_DIR "$ORIGIN")
endif()

include(GNUInstallDirs)

file(
    RELATIVE_PATH
    relDir
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
)
set(CMAKE_INSTALL_RPATH ${BASE_DIR} ${BASE_DIR}/${relDir})
