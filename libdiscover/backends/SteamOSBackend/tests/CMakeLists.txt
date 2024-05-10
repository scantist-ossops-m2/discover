set(EXTRA_LIBS
    libdiscover-backend-steamos-logging-category
)
add_unit_test(steamostest SteamOSTest.cpp)
set_tests_properties(steamostest PROPERTIES TIMEOUT 600)
