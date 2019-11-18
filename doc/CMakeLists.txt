add_custom_command(OUTPUT perfecttin.html MAIN_DEPENDENCY perfecttin.xml COMMAND pandoc -f docbook -t html -o perfecttin.html ${PROJECT_SOURCE_DIR}/doc/perfecttin.xml)
add_custom_target(perfecttin-doc ALL DEPENDS perfecttin.html)
