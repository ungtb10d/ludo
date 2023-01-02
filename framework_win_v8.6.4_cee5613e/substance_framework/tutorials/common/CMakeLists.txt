#########################################################
# Substance Framework Tutorials Common Library
#########################################################

set(PROJECT_NAME substance_framework_tutorials_common)

set(FRAMEWORK_COMMON_SOURCES
	frameworktestcommon.cpp
	frameworktestcommon.h
	frameworktestddssave.cpp
	frameworktestddssave.h
)

add_library(${PROJECT_NAME} STATIC ${FRAMEWORK_COMMON_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
	../../include
)
