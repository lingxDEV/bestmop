#
# Copyright (C) 2008-2021 ZonaXtrema group (TheDark)
#

AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/MyPlayer.cpp")
AC_ADD_SCRIPT_LOADER("MyPlayer" "${CMAKE_CURRENT_LIST_DIR}/src/loader.h")
AC_ADD_CONFIG_FILE("${CMAKE_CURRENT_LIST_DIR}/conf/myplayer.conf.dist")

file(GLOB_RECURSE scripts_MyPlayer src/*.cpp src/*.h)
source_group(Modules\\MyPlayer FILES ${scripts_MyPlayer})

set(scripts_STAT_SRCS
  ${scripts_STAT_SRCS}
  ${scripts_MyPlayer}
)

message("  -> Module prepared: MyPlayer")