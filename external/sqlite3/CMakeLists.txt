# SQLite3 amalgamation

add_library(sqlite3 sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(sqlite3 PRIVATE -DSQLITE_THREADSAFE=1 -DSQLITE_DEFAULT_FOREIGN_KEYS=1)

# For static builds, ensure proper symbol visibility
if(NOT BUILD_SHARED_LIBS)
  set_target_properties(sqlite3 PROPERTIES
    COMPILE_FLAGS "-fPIC"
  )
endif()

# Install targets
install(FILES sqlite3.h DESTINATION include)
