list(APPEND CMAKE_MODULE_PATH ${CTEST_SCRIPT_DIRECTORY})

set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS "verbosity=1:symbolize=1:abort_on_error=1:detect_leaks=1")
set(CTEST_CUSTOM_WARNING_EXCEPTION
    ".*Warning in cling::IncrementalParser::CheckABICompatibility().*")

include(project_ctest_common)

set(CONFIGURE_OPTIONS "--preset clang" "-DENABLE_ASAN=${ENABLE_ASAN}"
                      "-DENABLE_TSAN=${ENABLE_TSAN}")

ctest_configure(OPTIONS "${CONFIGURE_OPTIONS}")

ctest_submit(PARTS Configure)

ctest_build()

ctest_submit(PARTS Build)

ctest_memcheck()

ctest_submit(PARTS MemCheck)
