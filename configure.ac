#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([logmoko],[m4_esyscmd_s([cat VERSION])],[jecklgamis@gmail.com])
AC_CANONICAL_HOST
AC_PROG_MAKE_SET
AC_CONFIG_SRCDIR([src/main/logmoko.c])
AM_INIT_AUTOMAKE

# Configuration header
AC_CONFIG_HEADERS([src/main/logmoko_config_env.h])

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
AC_SEARCH_LIBS([socket],[socket nsl],,[AC_MSG_ERROR([Unable to find socket library])])
AC_SEARCH_LIBS([inet_pton],[nsl socket],,[AC_MSG_ERROR([Unable to find inet_pton])])
AC_SEARCH_LIBS([openlog],[],,[AC_MSG_ERROR([Unable to find syslog functions])])
AC_SEARCH_LIBS([EVP_EncryptInit_ex],[crypto ssl],,[AC_MSG_ERROR([Unable to find OpenSSL crypto library])])
AC_CHECK_HEADERS([openssl/evp.h openssl/rand.h openssl/sha.h openssl/hmac.h openssl/crypto.h],,[AC_MSG_ERROR([OpenSSL headers not found])])

# Checks for header files.
AC_CHECK_HEADERS([stdio.h stdlib.h string.h inttypes.h stdint.h strings.h \
    sys/stat.h sys/types.h sys/socket.h netinet/in.h arpa/inet.h \
    unistd.h setjmp.h time.h stdarg.h pthread.h syslog.h ctype.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_STRINGIZE
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(void*)

# Checks for library functions.
AC_FUNC_MALLOC

# Checks for functions
AC_CHECK_FUNCS([socket sendto inet_pton inet_ntop openlog syslog closelog \
    pthread_create pthread_mutex_init strtok_r])

# Configuration files
AC_CONFIG_FILES([Makefile src/main/Makefile src/test/Makefile src/examples/Makefile docs/Makefile])

AC_OUTPUT


