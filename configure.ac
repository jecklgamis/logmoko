#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([logmoko],[1.0.0-beta],[jecklgamis@gmail.com])
AC_CANONICAL_HOST
AC_PROG_MAKE_SET
AC_CONFIG_SRCDIR([src/main/logmoko.c])
AM_INIT_AUTOMAKE

# Configuration header
AC_CONFIG_HEADER(src/main/logmoko_config_env.h)

AC_ARG_ENABLE(debug,[  --enable-debug    Turn on debugging],
	[case "${enableval}" in
       yes) debug=true ;;
       no)  debug=false ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
    esac],[debug=false])
AM_CONDITIONAL(DEBUG, test x$debug = xtrue)

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_PROG_RANLIB
AC_SEARCH_LIBS([pthread_create],[pthread],,[AC_MSG_ERROR([Unable to find pthread library])])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdio.h stdlib.h string.h setjmp.h time.h stdarg.h pthread.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_STRINGIZE
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(void*)

# Checks for library functions.
AC_FUNC_MALLOC

# Checks for functions
#AC_CHECK_FUNCS()

# Configuration files
AC_CONFIG_FILES([Makefile src/main/Makefile src/test/Makefile src/examples/Makefile docs/Makefile])

AC_OUTPUT


