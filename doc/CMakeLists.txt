find_program(PANDOC pandoc)
find_program(PDFLATEX pdflatex)

if (PANDOC AND PDFLATEX)
execute_process(COMMAND ${PANDOC} "--resource-path=/" INPUT_FILE /dev/null ERROR_FILE /dev/null RESULT_VARIABLE pandoc_exit)
endif ()

if (PANDOC AND PDFLATEX AND NOT pandoc_exit)
add_custom_command(OUTPUT perfecttin.pdf MAIN_DEPENDENCY perfecttin.xml COMMAND pandoc -f docbook -o perfecttin.pdf -V papersize=a4 -V margin-left=15mm -V margin-right=15mm -V margin-top=15mm -V margin-bottom=20mm --resource-path=${PROJECT_SOURCE_DIR}/doc ${PROJECT_SOURCE_DIR}/doc/perfecttin.xml)
add_custom_command(OUTPUT fileformat.pdf MAIN_DEPENDENCY fileformat.xml COMMAND pandoc -f docbook -o fileformat.pdf -V papersize=a4 -V margin-left=15mm -V margin-right=15mm -V margin-top=15mm -V margin-bottom=20mm --resource-path=${PROJECT_SOURCE_DIR}/doc ${PROJECT_SOURCE_DIR}/doc/fileformat.xml)
add_custom_target(perfecttin-doc ALL DEPENDS perfecttin.pdf fileformat.pdf)
install(FILES ${PROJECT_BINARY_DIR}/doc/fileformat.pdf ${PROJECT_BINARY_DIR}/doc/perfecttin.pdf DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/perfecttin)
endif ()
