if (NETPBM2OSBADGEICONS_TEST_VALGRIND)
  add_executable (valgrind-checker valgrind-checker.c)
endif()

file (GLOB TESTDIRS_RAW "*")
set (TESTDIRS)

foreach (TESTDIR IN LISTS TESTDIRS_RAW)
  if (IS_DIRECTORY "${TESTDIR}")
    list (APPEND TESTDIRS "${TESTDIR}")
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
  list (APPEND TESTARGS "${TESTDIR}/primary")

  if (EXISTS "${TESTDIR_FULL}/alpha")
    if (EXISTS "${TESTDIR_FULL}/secondary")
      list (APPEND TESTARGS "${TESTDIR}/secondary")
    else()
      list (APPEND TESTARGS "none")
    endif()
    list (APPEND TESTARGS "${TESTDIR}/alpha")
  else()
    if (EXISTS "${TESTDIR_FULL}/secondary")
      list (APPEND TESTARGS "${TESTDIR}/secondary")
    endif()
  endif()

  add_test (NAME "${TESTDIR}"
    COMMAND "$<TARGET_FILE:${PROJECT_NAME}>" ${TESTARGS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
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
        ${VALGRIND_BIN}
          --show-error-list=yes
          --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all
          --error-exitcode=205
        "$<TARGET_FILE:${PROJECT_NAME}>" ${TESTARGS}
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
  endif()
endforeach()
