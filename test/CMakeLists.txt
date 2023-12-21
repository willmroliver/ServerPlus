target_sources(db_test
    PRIVATE
        main.cpp
        server.cpp
        buffer.cpp
)

target_link_libraries(db_test
    PRIVATE
        db_lib
    PUBLIC 
        ${Boost_LIBRARIES}
        LibeventPlus
)