#------------------------------------------------------------------------------
# Set up test macros
#------------------------------------------------------------------------------
#
# na prefix is added to executable
#
function(build_na_test test_name)
  add_executable(na_test_${test_name} test_${test_name}.c)
  target_link_libraries(na_test_${test_name} na_test)
  if(MERCURY_ENABLE_COVERAGE)
    set_coverage_flags(na_test_${test_name})
  endif()
endfunction()

function(add_na_test test_name server client)
  foreach(comm ${NA_PLUGINS})
    set(test_args --comm ${comm})
    set(full_test_name ${test_name}_${comm})

    # Dynamic client/server test
    add_test(NAME "na_${full_test_name}"
      COMMAND $<TARGET_FILE:mercury_test_driver>
      --server $<TARGET_FILE:na_test_${server}>
      --client $<TARGET_FILE:na_test_${client}> ${test_args}
    )

    # Static client/server test
    if(${comm} STREQUAL "mpi")
      set(static_test_name ${full_test_name}_static)
      set(static_test_args ${test_args} --static)
      add_test(NAME "na_${static_test_name}"
        COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 1
        ${MPIEXEC_PREFLAGS} $<TARGET_FILE:na_test_${server}> ${MPIEXEC_POSTFLAGS}
        ${static_test_args} : ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
        ${MPIEXEC_PREFLAGS} $<TARGET_FILE:na_test_${client}> ${static_test_args}
      )
  endif()
  endforeach()
endfunction()

#------------------------------------------------------------------------------
# na_test : Lib used by tests contains main test initialization etc
#------------------------------------------------------------------------------
add_library(na_test STATIC na_test.c na_test_getopt.c)
target_link_libraries(na_test na ${MERCURY_TEST_EXT_LIB_DEPENDENCIES})
if(MERCURY_ENABLE_COVERAGE)
  set_coverage_flags(na_test)
endif()

#------------------------------------------------------------------------------
# Network abstraction test
build_na_test(client)
build_na_test(server)

#------------------------------------------------------------------------------
# Set list of tests

# Client / server test with all enabled NA plugins
add_na_test(simple server client)
