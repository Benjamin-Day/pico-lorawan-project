
aux_source_directory(. DIR_Gas_Sensor_SRCS)

include_directories(../Config)

add_library(Gas_Sensor ${DIR_Gas_Sensor_SRCS})

target_link_libraries(Gas_Sensor PUBLIC Config)
