set(ENHINT_SOURCES
    enhint.cpp
    cedict.cpp
)

add_fcitx5_addon(enhint ${ENHINT_SOURCES})

set_target_properties(enhint PROPERTIES
    PREFIX "lib"
    OUTPUT_NAME "enhint"
)

target_compile_definitions(enhint PRIVATE
    -DFCITX_INSTALL_LOCALEDIR="${FCITX_INSTALL_LOCALEDIR}"
)

target_link_libraries(enhint PRIVATE
    Fcitx5::Core
    Fcitx5::Config
)

install(TARGETS enhint
    DESTINATION "${FCITX_INSTALL_ADDONDIR}"
)

configure_file(enhint.conf.in "${CMAKE_CURRENT_BINARY_DIR}/enhint.conf" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/enhint.conf"
    DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
)

# CC-CEDICT data. If a dictionary is present at build time it can be installed
# system-wide; otherwise users may drop it into
#   ~/.local/share/fcitx5/enhint/cedict_ts.u8
set(ENHINT_CEDICT "" CACHE FILEPATH "Path to CC-CEDICT (cedict_ts.u8) to install")
if(ENHINT_CEDICT AND EXISTS "${ENHINT_CEDICT}")
    install(FILES "${ENHINT_CEDICT}"
        DESTINATION "${FCITX_INSTALL_PKGDATADIR}/enhint"
        RENAME cedict_ts.u8
    )
endif()
