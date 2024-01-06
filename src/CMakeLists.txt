target_sources(db_lib
    PRIVATE
        server.cpp
        context.cpp
)

target_link_libraries(db_lib
    PUBLIC 
        LibeventPlus
        CryptPlus
)

target_link_libraries(db_exec db_lib)