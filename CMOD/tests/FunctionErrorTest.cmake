if(NOT DEFINED CMOD_EXECUTABLE OR NOT DEFINED PROJECT_FIXTURE
   OR NOT DEFINED TEST_WORK_DIR OR NOT DEFINED TEST_CASE)
    message(FATAL_ERROR "CMOD function test variables were not provided")
endif()

file(READ "${PROJECT_FIXTURE}" project_xml)
set(expected_details "CMOD project error:" "Suggestion:" "Build failed.")
if(TEST_CASE STREQUAL "project_expression")
    string(REPLACE "<Duration>1</Duration>" "<Duration>1+</Duration>"
           project_xml "${project_xml}")
    list(APPEND expected_details "1+" "Unexpected end of expression")
elseif(TEST_CASE STREQUAL "event_expression")
    string(REPLACE "<MaxChildDuration>1</MaxChildDuration>"
           "<MaxChildDuration>1+</MaxChildDuration>" project_xml "${project_xml}")
    list(APPEND expected_details "Event '0'" "1+" "Unexpected end of expression")
elseif(TEST_CASE STREQUAL "nested_expression")
    set(expression "2*(<Fun><Name>Random</Name><Low>1+</Low><High>2</High></Fun>)")
    string(REPLACE "<MaxChildDuration>1</MaxChildDuration>"
           "<MaxChildDuration>${expression}</MaxChildDuration>"
           project_xml "${project_xml}")
    list(APPEND expected_details "Event '0'" "${expression}"
         "1+" "Unexpected end of expression")
elseif(TEST_CASE STREQUAL "missing_top")
    string(REPLACE "<TopEvent>0</TopEvent>" "<TopEvent>MissingTop</TopEvent>"
           project_xml "${project_xml}")
    list(APPEND expected_details "MissingTop" "type 0")
elseif(TEST_CASE STREQUAL "missing_object")
    set(expression "<Fun><Name>ChooseL</Name><Entry><Fun><Name>ReadSIVFile</Name><File>MissingSieve</File></Fun></Entry></Fun>")
    string(REPLACE "<MaxChildDuration>1</MaxChildDuration>"
           "<MaxChildDuration>${expression}</MaxChildDuration>"
           project_xml "${project_xml}")
    list(APPEND expected_details "Event '0'" "MissingSieve" "type 7")
elseif(TEST_CASE STREQUAL "unknown_function")
    set(expression "<Fun><Name>MissingFunction</Name></Fun>")
    string(REPLACE "<MaxChildDuration>1</MaxChildDuration>"
           "<MaxChildDuration>${expression}</MaxChildDuration>"
           project_xml "${project_xml}")
    list(APPEND expected_details "Event '0'" "Unknown numeric function" "MissingFunction")
elseif(TEST_CASE STREQUAL "select_negative")
    set(expression "<Fun><Name>Select</Name><List>10</List><Index>-1</Index></Fun>")
    string(REPLACE "<MaxChildDuration>1</MaxChildDuration>"
           "<MaxChildDuration>${expression}</MaxChildDuration>"
           project_xml "${project_xml}")
    list(APPEND expected_details "Event '0'" "Select index" "outside" "-1")
elseif(TEST_CASE STREQUAL "select_object")
    set(expression "<Fun><Name>ChooseL</Name><Entry><Fun><Name>Select</Name><List><Fun><Name>ReadSIVFile</Name><File>unused</File></Fun></List><Index>1</Index></Fun></Entry></Fun>")
    string(REPLACE "<MaxChildDuration>1</MaxChildDuration>"
           "<MaxChildDuration>${expression}</MaxChildDuration>"
           project_xml "${project_xml}")
    list(APPEND expected_details "Event '0'" "Select index" "outside")
elseif(TEST_CASE STREQUAL "null_context")
    set(expression "<Fun><Name>GetPattern</Name><Method>IN_ORDER</Method><Origin>0</Origin><Pattern><Fun><Name>MakePattern</Name><List>1</List></Fun></Pattern></Fun>")
    string(REPLACE "<Duration>1</Duration>" "<Duration>${expression}</Duration>"
           project_xml "${project_xml}")
    list(APPEND expected_details "GetPattern" "requires an event context" "${expression}")
elseif(TEST_CASE STREQUAL "null_static_context")
    set(expression "<Fun><Name>CURRENT_CHILD_NUM</Name></Fun>")
    string(REPLACE "<Duration>1</Duration>" "<Duration>${expression}</Duration>"
           project_xml "${project_xml}")
    list(APPEND expected_details "CURRENT_CHILD_NUM" "requires an event context")
elseif(TEST_CASE STREQUAL "nonfinite_expression")
    string(REPLACE "<MaxChildDuration>1</MaxChildDuration>"
           "<MaxChildDuration>1/0</MaxChildDuration>" project_xml "${project_xml}")
    list(APPEND expected_details "Event '0'" "non-finite" "Expression: 1/0")
elseif(TEST_CASE STREQUAL "missing_markov_library")
    string(REPLACE "<MarkovModelLibrary>0</MarkovModelLibrary>" ""
           project_xml "${project_xml}")
    list(APPEND expected_details "Missing required project section" "MarkovModelLibrary")
elseif(TEST_CASE STREQUAL "invalid_markov_count")
    string(REPLACE "<MarkovModelLibrary>0</MarkovModelLibrary>"
           "<MarkovModelLibrary>0oops</MarkovModelLibrary>" project_xml "${project_xml}")
    list(APPEND expected_details "MarkovModelLibrary" "model count")
elseif(TEST_CASE STREQUAL "temporary_library_write")
    set(expected_details "CMOD output error:" "lib.temp" "Suggestion:" "Build failed.")
else()
    message(FATAL_ERROR "Unknown function error test case: ${TEST_CASE}")
endif()

set(case_dir "${TEST_WORK_DIR}/${TEST_CASE}")
file(MAKE_DIRECTORY "${case_dir}")
if(TEST_CASE STREQUAL "temporary_library_write")
    file(MAKE_DIRECTORY "${case_dir}/lib.temp")
endif()
set(project "${case_dir}/FunctionError.dissco")
set(run_input "${case_dir}/input.txt")
file(WRITE "${project}" "${project_xml}")
file(WRITE "${run_input}" "1\n")

execute_process(
    COMMAND "${CMOD_EXECUTABLE}" "${project}"
    INPUT_FILE "${run_input}"
    WORKING_DIRECTORY "${case_dir}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
    TIMEOUT 10
)
file(WRITE "${case_dir}/stdout.txt" "${stdout}")
file(WRITE "${case_dir}/stderr.txt" "${stderr}")

set(failure "")
if(NOT "${result}" STREQUAL "1")
    set(failure "Expected exit 1, got ${result}")
elseif(stdout MATCHES "Build complete\\." OR stderr MATCHES "Build complete\\.")
    set(failure "An invalid project was reported as complete")
else()
    list(APPEND expected_details "FunctionError.dissco")
    foreach(detail IN LISTS expected_details)
        string(FIND "${stderr}" "${detail}" position)
        if(position EQUAL -1)
            set(failure "Missing error detail: ${detail}")
            break()
        endif()
    endforeach()
endif()
if(NOT failure STREQUAL "")
    message(FATAL_ERROR
        "${TEST_CASE}: ${failure}\nstdout:\n${stdout}\nstderr:\n${stderr}")
endif()
