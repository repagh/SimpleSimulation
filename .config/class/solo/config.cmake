# ============================================================================
#       item            : CMake additional configuration for a node class
#       made by         : René van Paassen
#       date            : 180326
#       copyright       : (c) 2018 TUDelft-AE-C&S
# ============================================================================

# A "node" is a computer participating in a DUECA distributed process
# Each node requires a specific configuration, e.g., to show instruments,
# out-of-the-window view, perform IO with hardware. The software
# configuration for a node (chosen modules, libraries) is determined by
# it's "machine class"
#
# The "machine class" thus indicates what part of the application runs on this
# computer, examples are control_loading, ig, efis
#
# Per machine class, specify what libraries to link, additional DUECA
# components, compile flags etc.

# This file is included when dueca_setup_project is called from the main
# CMakeLists.txt file

# extend DUECA_COMPONENTS with additional components

# This defines some extra options, so that for the testing this project can be
# compiled in various combinations.
option(USE_GTK3      "Use GTK3 for graphic interface + GL winows" ON)
option(USE_GTK4      "Use GTK4 for graphic interface + GL windows" OFF)
option(GL_WITH_X     "Force a BareDuecaGL window (X11) for GL windows" OFF)
option(GL_WITH_GLFW  "Force a GLFW window (X11 or Wayland) for GL windows" OFF)

if(USE_GTK3)
    set(GUI_COMPONENT "gtk3")
endif()
if(USE_GTK4)
    set(GUI_COMPONENT "gtk4")
endif()

if(GL_WITH_X)
    set(CFLAGS "-DGL_WITH_BARE")
elseif(GL_WITH_GLFW)
    set(CFLAGS "-DGL_WITH_GLFW")
endif()

if(GUI_COMPONENT)
    list(APPEND DUECA_COMPONENTS ${GUI_COMPONENT})
endif()

# define PROJECT_LIBRARIES with libraries needed on the current platform,
# use CMAKE to detect these if needed
#set(PROJECT_LIBRARIES )

# define PROJECT_INCLUDE_DIRS with include directories common to all on
# the current platform
#set(PROJECT_INCLUDE_DIRS )

# define PROJECT_COMPILE_FLAGS with the flags needed for compiling
set(PROJECT_COMPILE_FLAGS ${CFLAGS})
