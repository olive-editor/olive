add_custom_target(docs
    COMMAND ${CMAKE_COMMAND} -E echo_append "Hello there, documenteer!"
    COMMAND calc
    COMMENT Custom target docs
)
# https://riptutorial.com/cmake/example/29469/running-a-custom-target