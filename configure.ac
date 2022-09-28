# Process this file with autoconf to produce a configure script.
AC_INIT(cue2toc, 0.4, [[dermatsch@gmx.de]])
AM_INIT_AUTOMAKE(cue2toc, 0.4)
AC_CONFIG_SRCDIR([cue2toc.c])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_AWK

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([stddef.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([putenv strerror])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
