include(FindPkgConfig)

pkg_check_modules(SYSTEMD REQUIRED "systemd")
if(SYSTEMD_FOUND)
    if (NOT DEFINED SYSTEMD_SYSTEM_UNIT_DIR)
        execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=systemdsystemunitdir systemd
                        OUTPUT_VARIABLE SYSTEMD_SYSTEM_UNIT_DIR)
        string(REGEX REPLACE "[ \t\n]+" "" SYSTEMD_SYSTEM_UNIT_DIR ${SYSTEMD_SYSTEM_UNIT_DIR})
    endif()

    install(FILES "simplelogin.service" DESTINATION "${SYSTEMD_SYSTEM_UNIT_DIR}")
    install(FILES "simplelogin@.service" DESTINATION "${SYSTEMD_SYSTEM_UNIT_DIR}")
endif()
