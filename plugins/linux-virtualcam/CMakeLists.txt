project(linux-virtualcam)

option(ENABLE_VIRTUALCAM "Build OBS Virtualcam (Linux)" ON)

if(NOT ENABLE_VIRTUALCAM)
  obs_status(DISABLED "linux-virtualcam")
  return()
endif()

find_package(ALSA REQUIRED)
find_package(Libv4l2 REQUIRED)

include_directories(
	SYSTEM "${CMAKE_SOURCE_DIR}/libobs"
	${ALSA_INCLUDE_DIR}
	${LIBV4L2_INCLUDE_DIRS}
)

set(linux-virtualcam_SOURCES
	alsa-output.c
	v4l2-output.c
	linux-virtualcam.c
)

add_library(linux-virtualcam MODULE
	${linux-virtualcam_SOURCES}
)

target_link_libraries(linux-virtualcam
	libobs
	${ALSA_LIBRARY}
	${LIBV4L2_LIBRARIES}
)

set_target_properties(linux-virtualcam PROPERTIES FOLDER "plugins")

install_obs_plugin_with_data(linux-virtualcam data)
