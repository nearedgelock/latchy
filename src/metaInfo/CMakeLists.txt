target_sources(${CMAKE_PROJECT_NAME} PUBLIC
  containerInfo.cpp
  machineInfo.cpp
  metaInfo.cpp
  processInfo.cpp
)
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})  
