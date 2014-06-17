# Install script for directory: /media/psf/Home/Downloads/mysql-5.6.14/include

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/usr/local/mysql")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "RelWithDebInfo")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

# Install shared libraries without execute permission?
IF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  SET(CMAKE_INSTALL_SO_NO_EXE "1")
ENDIF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Development")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/media/psf/Home/Downloads/mysql-5.6.14/include/mysql.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/mysql_com.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/mysql_time.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/my_list.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/my_alloc.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/typelib.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/mysql/plugin.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/mysql/plugin_audit.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/mysql/plugin_ftparser.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/mysql/plugin_validate_password.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/my_dbug.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/m_string.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/my_sys.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/my_xml.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/mysql_embed.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/my_pthread.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/decimal.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/errmsg.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/my_global.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/my_net.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/my_getopt.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/sslopt-longopts.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/my_dir.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/sslopt-vars.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/sslopt-case.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/sql_common.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/keycache.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/m_ctype.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/my_attribute.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/my_compiler.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/mysql_com_server.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/my_byteorder.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/byte_order_generic.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/byte_order_generic_x86.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/byte_order_generic_x86_64.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/little_endian.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/include/big_endian.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/bl/include/mysql_version.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/bl/include/my_config.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/bl/include/mysqld_ername.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/bl/include/mysqld_error.h"
    "/media/psf/Home/Downloads/mysql-5.6.14/bl/include/sql_state.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Development")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Development")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mysql" TYPE DIRECTORY FILES "/media/psf/Home/Downloads/mysql-5.6.14/include/mysql/" REGEX "/[^/]*\\.h$" REGEX "/psi\\_abi[^/]*$" EXCLUDE)
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Development")

