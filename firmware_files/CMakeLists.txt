cmake_minimum_required(VERSION 3.16)

# Define the project
project(gridsentry_firmware)

# Explicitly set the component search paths
set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_CURRENT_SOURCE_DIR}/components"
    "${CMAKE_CURRENT_SOURCE_DIR}/managed_components"
)
