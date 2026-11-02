if(NOT DEFINED TEST_BINARY OR NOT DEFINED TEST_DIR OR NOT DEFINED TEST_DOC OR NOT DEFINED TEST_DOC2)
    message(FATAL_ERROR "TEST_BINARY, TEST_DIR, TEST_DOC and TEST_DOC2 are required")
endif()
file(MAKE_DIRECTORY "${TEST_DIR}")
file(REMOVE_RECURSE "${TEST_DIR}/drafts")
file(COPY "${TEST_BINARY}" DESTINATION "${TEST_DIR}")
# Portable mode keeps the drafts and reading positions in the test folder
file(WRITE "${TEST_DIR}/settings.ini" "[Settings]\nhasAskedFileAssociation=1\ncheckUpdates=0\n")
get_filename_component(TEST_NAME "${TEST_BINARY}" NAME)
# The test saves into copies it writes itself, never into the fixtures
execute_process(COMMAND "${TEST_DIR}/${TEST_NAME}" --byte-order-mark-tests "${TEST_DOC}" "${TEST_DOC2}"
    WORKING_DIRECTORY "${TEST_DIR}" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Byte order mark tests failed: ${result}")
endif()
