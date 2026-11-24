function(hmi_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE hmi_core ${QT}::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

hmi_add_test(tst_alarmmodel)
hmi_add_test(tst_processvalue)
hmi_add_test(tst_simulatedbackend)

