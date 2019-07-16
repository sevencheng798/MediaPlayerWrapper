# Create by Sven
# 201902

project(AICommon)

add_subdirectory("Utils")

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Utils/src/Logging  Logging_SOURCES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Utils/src/SharedBuffer  Sds_SOURCES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Utils/src/Attachment  Attachment_SOURCES)
add_compile_options(-fPIC)
add_library(sharedbuffer STATIC ${Sds_SOURCES} ${Attachment_SOURCES})

link_directories(${AICommon_SOURCE_DIR}/Utils/src)
add_library(AICommon SHARED 
	Utils/src/DeviceInfo.cpp
	Utils/src/Executor.cpp
	Utils/src/TaskQueue.cpp
	Utils/src/TaskThread.cpp
	Utils/src/SafeShutdown.cpp
	${Logging_SOURCES})

target_include_directories(sharedbuffer PUBLIC 
	"${AICommon_SOURCE_DIR}/Utils/include")
target_include_directories(AICommon PUBLIC 
	"${AICommon_SOURCE_DIR}/Utils/include"
	"${AICommon_SOURCE_DIR}/DMInterface/include"
	"${CURL_INCLUDE_DIRS}")

target_link_libraries(AICommon
    ${LIB_SHARED_BUFFER}
    ${CURL_LIBRARIES})

LIST(APPEND PATHS 
	"${PROJECT_SOURCE_DIR}/Utils/include"
	"${AICommon_SOURCE_DIR}/DMInterface/include")

asdk_install_multiple("${PATHS}")

