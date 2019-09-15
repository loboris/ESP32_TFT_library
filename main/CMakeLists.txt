set(SOURCES tft_demo.c)
idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS
          ${CMAKE_CURRENT_LIST_DIR}
          $ENV{IDF_PATH}/components
        REQUIRES
            tft
            spiffs
)
