target_sources(${CMAKE_PROJECT_NAME} PUBLIC
  fileAccess.cpp
  forkExec.cpp
  log.cpp
)
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})  
