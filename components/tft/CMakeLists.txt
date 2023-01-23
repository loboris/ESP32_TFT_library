FILE(GLOB SOURCES *.c)
idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "."
                       REQUIRES spidriver
                       PRIV_REQUIRES driver)
