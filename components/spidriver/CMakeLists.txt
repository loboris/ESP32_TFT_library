FILE(GLOB SOURCES *.c)
idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS "."
                       PRIV_REQUIRES driver)
