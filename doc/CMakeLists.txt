find_program(PANDOC pandoc)

if (PANDOC)
execute_process(COMMAND ${PANDOC} "--resource-path=/" INPUT_FILE /dev/null ERROR_FILE /dev/null RESULT_VARIABLE pandoc_exit)
endif ()

if (PANDOC AND NOT pandoc_exit)
add_custom_command(OUTPUT perfecttin.pdf MAIN_DEPENDENCY perfecttin.xml COMMAND pandoc -f docbook -o perfecttin.pdf -V papersize=a4 --resource-path=${PROJECT_SOURCE_DIR}/doc ${PROJECT_SOURCE_DIR}/doc/perfecttin.xml)
add_custom_command(OUTPUT fileformat.pdf MAIN_DEPENDENCY fileformat.xml COMMAND pandoc -f docbook -o fileformat.pdf -V papersize=a4 --resource-path=${PROJECT_SOURCE_DIR}/doc ${PROJECT_SOURCE_DIR}/doc/fileformat.xml)
add_custom_target(perfecttin-doc ALL DEPENDS perfecttin.pdf fileformat.pdf)
endif ()
