# @FILE multi_master.cmake
# @AUTHOR wei

# ------------------- MySQL include
INCLUDE_DIRECTORIES(SYSTEM ${LIBEVENT_INCLUDE_DIRS})

# ------------------- Custom Include Search Path
INCLUDE_DIRECTORIES(extra/multi-master-tool/include)

SET(SQDLOG_INCLUDE_DIR extra/multi-master-tool/easy_logger/include)
INCLUDE_DIRECTORIES(BEFORE SYSTEM ${SQDLOG_INCLUDE_DIR})

INCLUDE_DIRECTORIES(extra/multi-master-tool/multi_net_io/include)
# -------------------

ADD_SUBDIRECTORY(extra/multi-master-tool)