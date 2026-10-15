if (NETPBM2OSBADGEICONS_TEST_VALGRIND)
  add_executable (valgrind-checker valgrind-checker.c)
endif()

list (APPEND VALGRIND_ARGS
  --show-error-list=yes
  --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all
  --error-exitcode=205
)

file (GLOB TESTDIRS_RAW "*")
set (TESTDIRS)

# Manually: Awkward to automate
add_test (NAME "none-prints-usage"
  COMMAND "$<TARGET_FILE:${PROJECT_NAME}>"
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
if (NETPBM2OSBADGEICONS_TEST_VALGRIND)
  add_test (NAME "valgrind-none-prints-usage"
    COMMAND
      "$<TARGET_FILE:valgrind-checker>"
        ${VALGRIND_BIN} ${VALGRIND_ARGS}
        "$<TARGET_FILE:${PROJECT_NAME}>"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
endif()

add_test (NAME "toomany-prints-usage"
  COMMAND
    "$<TARGET_FILE:${PROJECT_NAME}>"
      ${CMAKE_CURRENT_BINARY_DIR}/small.pgm
      ${CMAKE_CURRENT_BINARY_DIR}/small.pgm
      ${CMAKE_CURRENT_BINARY_DIR}/small.pgm
      ${CMAKE_CURRENT_BINARY_DIR}/small.pgm
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
if (NETPBM2OSBADGEICONS_TEST_VALGRIND)
  add_test (NAME "valgrind-toomany-prints-usage"
    COMMAND
      "$<TARGET_FILE:valgrind-checker>"
        ${VALGRIND_BIN} ${VALGRIND_ARGS}
        "$<TARGET_FILE:${PROJECT_NAME}>"
          ${CMAKE_CURRENT_BINARY_DIR}/small.pgm
          ${CMAKE_CURRENT_BINARY_DIR}/small.pgm
          ${CMAKE_CURRENT_BINARY_DIR}/small.pgm
          ${CMAKE_CURRENT_BINARY_DIR}/small.pgm
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
endif()

add_test (NAME "missing-p"
  COMMAND "$<TARGET_FILE:${PROJECT_NAME}>" ${CMAKE_CURRENT_SOURCE_DIR}/missing
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
set_property (TEST "missing-p"
  PROPERTY WILL_FAIL TRUE
)

add_test (NAME "missing-s"
  COMMAND "$<TARGET_FILE:${PROJECT_NAME}>" ${CMAKE_CURRENT_SOURCE_DIR}/test.ppm ${CMAKE_CURRENT_SOURCE_DIR}/missing
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
set_property (TEST "missing-s"
  PROPERTY WILL_FAIL TRUE
)

add_test (NAME "missing-a"
  COMMAND
    "$<TARGET_FILE:${PROJECT_NAME}>"
      ${CMAKE_CURRENT_SOURCE_DIR}/test.ppm
      ${CMAKE_CURRENT_SOURCE_DIR}/test.ppm
      ${CMAKE_CURRENT_SOURCE_DIR}/missing
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
set_property (TEST "missing-a"
  PROPERTY WILL_FAIL TRUE
)

# Automatically
foreach (TESTDIR IN LISTS TESTDIRS_RAW)
  if (IS_DIRECTORY "${TESTDIR}")
    file (REAL_PATH "${TESTDIR}" TESTDIR_ABSOLUTE)
    list (APPEND TESTDIRS "${TESTDIR_ABSOLUTE}")
  endif()
endforeach()

foreach (TESTDIR_FULL IN LISTS TESTDIRS)
  get_filename_component (TESTDIR "${TESTDIR_FULL}" NAME)
  string (FIND "${TESTDIR}" "success-" TEST_SUCCESS_PREFIX_POS)
  string (FIND "${TESTDIR}" "failure-" TEST_FAILURE_PREFIX_POS)
  set (TEST_TYPE)
  if (TEST_SUCCESS_PREFIX_POS EQUAL 0)
    set (TEST_TYPE "success")
  elseif (TEST_FAILURE_PREFIX_POS EQUAL 0)
    set (TEST_TYPE "failure")
  else()
    message (FATAL_ERROR "Cannot determine test type (starts with neither 'success-' nor 'failure-': ${TESTDIR}")
  endif()

  set (TESTARGS)
  list (APPEND TESTARGS "${TESTDIR_FULL}/primary")

  if (EXISTS "${TESTDIR_FULL}/alpha")
    if (EXISTS "${TESTDIR_FULL}/secondary")
      list (APPEND TESTARGS "${TESTDIR_FULL}/secondary")
    else()
      list (APPEND TESTARGS "none")
    endif()
    list (APPEND TESTARGS "${TESTDIR_FULL}/alpha")
  else()
    if (EXISTS "${TESTDIR_FULL}/secondary")
      list (APPEND TESTARGS "${TESTDIR_FULL}/secondary")
    endif()
  endif()

  add_test (NAME "${TESTDIR}"
    COMMAND "$<TARGET_FILE:${PROJECT_NAME}>" ${TESTARGS}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
  if (TEST_TYPE STREQUAL "failure")
    set_property (TEST "${TESTDIR}"
      PROPERTY WILL_FAIL TRUE
    )
  endif()

  if (
    NETPBM2OSBADGEICONS_TEST_VALGRIND
    # when netpbm code fails, it aborts the process - can't be nice and ensure no leaks
    AND TEST_TYPE STREQUAL "success"
  )
    add_test (NAME "valgrind-${TESTDIR}"
      COMMAND
        "$<TARGET_FILE:valgrind-checker>"
          ${VALGRIND_BIN} ${VALGRIND_ARGS}
          "$<TARGET_FILE:${PROJECT_NAME}>" ${TESTARGS}
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
  endif()
endforeach()
