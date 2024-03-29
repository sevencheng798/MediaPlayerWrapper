#
# Creator by Sven
#
cmake_minimum_required(VERSION 3.1)

set(AOWrapperTest_SOURCES)
list(APPEND AOWrapperTest_SOURCES AOWrapperTest.cpp)

# This is a temporary settings only for am113-board
#link_directories("/home/sven/work/3rd/_install/libbz2/lib")

add_executable(AOWrapperTest ${AOWrapperTest_SOURCES})
add_executable(AOWrapperTestLoop AOWrapperTestLoop.cpp)
add_executable(AOWrapperAttachmentTest AOWrapperAttachmentTest.cpp)
if (GTEST_ENABLE)
add_executable(AOWrapperMockTest AOWrapperMockTest.cpp)
endif()

target_include_directories(AOWrapperTest PUBLIC
		"${AOWrapperTest_SOURCES_DIR}/include")

target_include_directories(AOWrapperTestLoop PUBLIC
		"${AOWrapperTest_SOURCES_DIR}/include")

target_include_directories(AOWrapperAttachmentTest PUBLIC
		"${AOWrapperTest_SOURCES_DIR}/include")
if (GTEST_ENABLE)
target_include_directories(AOWrapperMockTest PUBLIC
		"${AOWrapperTest_SOURCES_DIR}/include"
		"${GTEST_INCLUDE_DIR}")
endif()
target_link_libraries(AOWrapperTest 
		AICommon
		AudioMediaPlayer
		ao
		asound
		z)

target_link_libraries(AOWrapperTestLoop 
		AICommon
		AudioMediaPlayer
		ao
		asound
		z)
target_link_libraries(AOWrapperAttachmentTest 
		AICommon
		AudioMediaPlayer
		ao
		asound
		z)
if (GTEST_ENABLE)
target_link_libraries(AOWrapperMockTest 
		AICommon
		AudioMediaPlayer
		ao
		asound
		gmock_main
		gmock
		gtest
		pthread
		z)
endif()

install(TARGETS AOWrapperTest
      RUNTIME DESTINATION bin
      BUNDLE  DESTINATION bin
      LIBRARY DESTINATION lib)
