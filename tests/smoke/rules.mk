check_PROGRAMS += \
    tests/smoke/crash_regression \
    $(EMPTY)

tests_smoke_crash_regression_SOURCES = \
    tests/smoke/crash_regression.cpp \
    $(EMPTY)

tests_smoke_crash_regression_LDADD = \
    $(test_LIBS) \
    $(OPENAL_LIBS) \
    $(LUA_LIBS) \
    $(ICONV_LIBS) \
    $(XVID_LIBS) \
    $(SDL_LIBS) \
    $(EMPTY)

tests_smoke_crash_regression_CXXFLAGS = $(test_CXXFLAGS)
