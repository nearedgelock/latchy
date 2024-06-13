if (BUILD_EXECUTABLE)
  target_sources(${CMAKE_PROJECT_NAME} PUBLIC
    help.cpp
    configuration.cpp
    curl.cpp
    latchyMain.cpp
    main.cpp
  )
else()
  target_sources(${CMAKE_PROJECT_NAME} PUBLIC
    help.cpp
    configuration.cpp
    curl.cpp
    latchyMain.cpp
  )
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})   

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/assets)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/helpers)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/metaInfo)

 
