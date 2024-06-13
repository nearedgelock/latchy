target_sources(${CMAKE_PROJECT_NAME} PUBLIC
  assets.cpp
  assetSource_File.cpp
  assetSource_Clevis.cpp
  assetProvider.cpp
)
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})  
