dnl Process this file with autoconf to produce a configure script.

dnl Make sure we're interpreted by some minimal autoconf.
AC_PREREQ(2.57)

AC_INIT(libkml, 1.3.0, http://code.google.com/p/libkml/issues)
m4_ifdef([AM_PROG_AR],[AM_PROG_AR])
AC_CONFIG_SRCDIR(README)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(-Wall -Werror subdir-objects)

AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_C_CONST
AC_C_INLINE
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

dnl Let Makefile know if we are GCC.
AM_CONDITIONAL(GCC, test "$GCC" = yes)

dnl Which platform are we building on?
PLATFORM=
SHLIBSUFFIX=
case "`uname -s`" in
	Linux)	PLATFORM=linux
		SHLIBSUFFIX="so"
	;;
	Darwin)	PLATFORM=darwin
		SHLIBSUFFIX="dylib"
	;;
	*) 	PLATFORM=unknown
		SHLIBSUFFIX="so"
	;;
esac
AC_SUBST(PLATFORM)
AC_SUBST(SHLIBSUFFIX)

dnl Where can we find the Expat headers and library?
AC_ARG_WITH(expat-include-dir,
	    [--with-expat-include-dir=DIR look in DIR for expat headers],
	    [CFLAGS="$CFLAGS -I$withval" CPPFLAGS="$CPPFLAGS -I$withval"])
AC_ARG_WITH(expat-lib-dir,
	    [--with-expat-lib-dir=DIR look in DIR for expat libraries],
	    [LDFLAGS="$LDFLAGS -L$withval"])

dnl Where can we find the Java headers and libs?
AC_ARG_WITH(java-include-dir,
	    [--with-java-include-dir=DIR look in DIR for Java headers],
	    [JAVA_INCLUDE_DIR=$withval],)
AC_SUBST(JAVA_INCLUDE_DIR)
AC_ARG_WITH(java-lib-dir,
	    [--with-java-lib-dir=DIR look in DIR for Java libraries],
	    [JAVA_LIB_DIR=$withval],)
AC_SUBST(JAVA_LIB_DIR)

dnl Where can we find the Python headers and libs?
AC_ARG_WITH(python-include-dir,
	    [--with-python-include-dir=DIR look in DIR for Python headers],
	    [PYTHON_INCLUDE_DIR=$withval],)
AC_SUBST(PYTHON_INCLUDE_DIR)
AC_ARG_WITH(python-lib-dir,
	    [--with-python-lib-dir=DIR look in DIR for Python libraries],
	    [PYTHON_LIB_DIR=$withval],)
AC_SUBST(PYTHON_LIB_DIR)


AC_CHECK_HEADERS(expat.h, [expat_h_found="yes"], [expat_h_found="no"])
if test $expat_h_found = "no"
then
	AC_MSG_ERROR("Unable to locate expat.h. Use configure --help to see how to specify the search path")
fi
AC_CHECK_LIB(expat, XML_ParserCreate, [],
	AC_MSG_ERROR("Expat library not found. Use configure --help to see how to specify the search path"))

AC_CHECK_HEADERS([float.h limits.h stdlib.h string.h])
AC_CHECK_FUNCS([floor memset strstr])

dnl If SWIG is available we will try to generate bindings.
AC_ARG_ENABLE(swig,
	[AS_HELP_STRING([--disable-swig],
			[do not generate any SWIG language bindings])])
if test "x$enable_swig" != xno; then
	AC_CHECK_PROG(SWIG, swig, swig)
	if test "x$SWIG" != x; then
		SWIG_VERSION=`swig -version 2>&1|grep Version|cut -d" " -f3`
		if test ${SWIG_VERSION:4} -lt 35; then
			AC_MSG_WARN([You have SWIG $SWIG_VERSION installed, but libkml requires at least SWIG 1.3.35. The bindings will not be built.])
			SWIG=
		fi
	fi
fi
AM_CONDITIONAL(WITH_SWIG, [test "x$SWIG" != x])

dnl Shall we install the python extensions in the system python extension
dnl directory? Note: to allow unprivileged installation this is off by default.
AC_ARG_ENABLE(systempython,
	[AS_HELP_STRING([--enable-systempython],
			[install the python SWIG bindings to the default system location using root privileges])])
USE_DEFAULT_PYTHON_LIBDIR=false
if test "x$enable_systempython" = xyes; then
	USE_DEFAULT_PYTHON_LIBDIR=true
fi

dnl Shall we build the Java SWIG bindings?
if test "x$SWIG" != x; then
	AC_ARG_ENABLE(java,
		      [AS_HELP_STRING([--disable-java],
				      [do not generate Java SWIG bindings])])
	if test "x$enable_java" != xno; then
		AC_CHECK_PROG(JAVA, java, java, nojava)
		if test "$JAVA" = "nojava"; then
		  AC_MSG_ERROR([no java binary in PATH])
		fi
		AC_CHECK_PROG(JAVAC, javac, javac, nojavac)
		if test "$JAVAC" = "nojavac"; then
		  AC_MSG_ERROR([no javac binary in PATH, bindings will not be built])
		fi
		AC_CHECK_PROG(JAR, jar, jar, nojar)
		if test "$JAR" = "nojar"; then
		  AC_MSG_ERROR([no jar binary in PATH, bindings will not be built])
		fi
		if test "x$JAVA" != x; then
			current_CPPFLAGS="$CPPFLAGS"
			if test "$CPPFLAGS" != ""; then
				CPPFLAGS="-I$current_CPPFLAGS -I$JAVA_INCLUDE_DIR -I$JAVA_INCLUDE_DIR/$PLATFORM"
			else
				CPPFLAGS="-I$JAVA_INCLUDE_DIR -I$JAVA_INCLUDE_DIR/$PLATFORM"
			fi
			AC_CHECK_HEADER(jni.h,,
					[AC_MSG_WARN([could not find jni.h. The Java bindings will not be built.])
			JAVA=
			])
			CPPFLAGS="$current_CPPFLAGS"
		fi
	fi
fi
AM_CONDITIONAL(WITH_JAVA, [test "x$JAVA" != x])

dnl Shall we build the Python SWIG bindings?
if test "x$SWIG" != x; then
	AC_ARG_ENABLE(python,
		      [AS_HELP_STRING([--disable-python],
				      [do not generate Python SWIG bindings])])
	if test "x$enable_python" != xno; then
		AC_CHECK_PROG(PYTHON, python, python)
		if test "x$PYTHON" != x; then
			PYTHON_VERSION=`$PYTHON -c "import sys; print sys.version[[:3]]"`
			if test "$USE_DEFAULT_PYTHON_LIBDIR" = true; then
				PYTHON_SITE_PACKAGES_DIR=`$PYTHON -c 'import distutils.sysconfig; print distutils.sysconfig.get_python_lib(1,0)'`
			else
				PYTHON_SITE_PACKAGES_DIR=`$PYTHON -c 'import distutils.sysconfig; print distutils.sysconfig.get_python_lib(1,0,"${exec_prefix}")'`
			fi
			AC_SUBST(PYTHON_SITE_PACKAGES_DIR)
			if test "$PYTHON_INCLUDE_DIR" = ""; then
				PYTHON_INCLUDE_DIR=`$PYTHON -c "import distutils.sysconfig; print distutils.sysconfig.get_python_inc()"`
			fi
			if test "$PYTHON_LIB_DIR" = ""; then
				PYTHON_LIB_DIR=`$PYTHON -c "import distutils.sysconfig; print distutils.sysconfig.get_config_var('LIBPL')"`
			fi
			current_CPPFLAGS="$CPPFLAGS"
			CPPFLAGS="-I$PYTHON_INCLUDE_DIR"
			AC_CHECK_HEADER(Python.h,,
					[AC_MSG_WARN([could not find Python.h. The Python bindings will not be built.])
			PYTHON=
			])
			PYTHON_LIBNAME="python"
			if test "$PLATFORM" = "linux"; then
				PYTHON_LIBNAME="python$PYTHON_VERSION"
			fi
			AC_CHECK_LIB($PYTHON_LIBNAME, Py_Initialize,,
					[AC_MSG_WARN([could not find the Python library. The Python bindings will not be built.])
			PYTHON=
			])
			LIBS=''  # don't append -lpython here.
			CPPFLAGS="$current_CPPFLAGS"
		fi
	fi
fi
AM_CONDITIONAL(WITH_PYTHON, [test "x$PYTHON" != x])

dnl If libcurl is available we will build networking examples.
dnl TODO: permit a --with-curl-prefix, etc
AC_CHECK_CURL(7.12.3)
AM_CONDITIONAL(HAVE_CURL, test "$CURL_LIBS")

dnl automake will add -DHAVE_CONFIG_H to compiler flags.
AM_CONFIG_HEADER([config.h]) 

AC_CONFIG_FILES([
	Makefile
	examples/Makefile
	examples/domviewer/Makefile
	examples/engine/Makefile
	examples/gpx/Makefile
	examples/gx/Makefile
	examples/hellonet/Makefile
	examples/helloworld/Makefile
	examples/java/Makefile
	examples/kml/Makefile
	examples/python/Makefile
	examples/regionator/Makefile
	examples/wxregionator/Makefile
	examples/wxviewer/Makefile
	examples/xsd/Makefile
	msvc/Makefile
	msvc/examples/Makefile
	msvc/tests/Makefile
	src/Makefile
	src/kml/Makefile
	src/kml/convenience/Makefile
	src/kml/dom/Makefile
	src/kml/engine/Makefile
	src/kml/regionator/Makefile
	src/kml/base/Makefile
	src/kml/xsd/Makefile
	src/swig/Makefile
	src/swig/java/Makefile
	src/swig/python/Makefile
	testdata/Makefile
	testdata/atom/Makefile
	testdata/balloon/Makefile
	testdata/csv/Makefile
	testdata/deprecated/Makefile
	testdata/gdata/Makefile
	testdata/gmaps/Makefile
	testdata/gpx/Makefile
	testdata/gx/Makefile
	testdata/kml/Makefile
	testdata/kmz/Makefile
        testdata/kmz/files/Makefile
        testdata/kmz/kmzfiles/Makefile
	testdata/kmz/rumsey/Makefile
	testdata/kmz/rumsey/kml/Makefile
	testdata/kmz/rumsey/imagery/Makefile
	testdata/links/Makefile
	testdata/style/Makefile
	testdata/style/weather/Makefile
	testdata/update/Makefile
	testdata/xal/Makefile
	testdata/xsd/Makefile
	third_party/Makefile
	xcode/Makefile
	xcode/LibKML/Makefile
])

AC_OUTPUT

