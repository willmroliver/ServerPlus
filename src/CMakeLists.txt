target_sources(db_lib
    PRIVATE
        server.cpp
)

target_link_libraries(db_lib
    PUBLIC 
        LibeventPlus
)

target_link_libraries(db_exec db_lib)