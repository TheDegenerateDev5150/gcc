# configure.ac -- Go library configure script.

# Copyright 2009 The Go Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# Process this file with autoreconf to produce configure.

AC_INIT(package-unused, version-unused,, libgo)
AC_CONFIG_SRCDIR(Makefile.am)
AC_CONFIG_HEADER(config.h)

libtool_VERSION=24:0:0
AC_SUBST(libtool_VERSION)

AM_ENABLE_MULTILIB(, ..)

AC_CANONICAL_SYSTEM
target_alias=${target_alias-$host_alias}

AM_INIT_AUTOMAKE([1.9.3 no-define foreign no-dist subdir-objects -Wall -Wno-portability -Wno-override])
AH_TEMPLATE(PACKAGE, [Name of package])
AH_TEMPLATE(VERSION, [Version number of package])

m4_rename([_AC_ARG_VAR_PRECIOUS],[glibgo_PRECIOUS])
m4_define([_AC_ARG_VAR_PRECIOUS],[])
AC_PROG_CC
AC_PROG_GO
AM_PROG_AS
m4_rename_force([glibgo_PRECIOUS],[_AC_ARG_VAR_PRECIOUS])

AC_SUBST(CFLAGS)

case ${host} in
  *-*-aix*)
    # Check default architecture for FAT library creation
    if test -z "`$CC -x c -E /dev/null -g3 -o - | grep 64BIT`" ; then
        AIX_EXTRA_ARCH='64'
    else
        AIX_EXTRA_ARCH='32'
    fi
    AC_SUBST(AIX_EXTRA_ARCH)
    ;;
esac

AM_MAINTAINER_MODE

AC_PROG_LD
AC_PROG_RANLIB
AC_CHECK_TOOL(OBJCOPY, objcopy, missing-objcopy)

AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL
AC_SUBST(enable_shared)
AC_SUBST(enable_static)
AM_CONDITIONAL([ENABLE_DARWIN_AT_RPATH], [test x$enable_darwin_at_rpath = xyes])

CC_FOR_BUILD=${CC_FOR_BUILD:-gcc}
AC_SUBST(CC_FOR_BUILD)

AC_PROG_AWK

WARN_FLAGS='-Wall -Wextra -Wwrite-strings -Wcast-qual'
AC_SUBST(WARN_FLAGS)

AC_ARG_ENABLE([werror],
  [AS_HELP_STRING([--disable-werror], [disable building with -Werror])])
AS_IF([test "x$enable_werror" != "xno" && test "x$GCC" = "xyes"],
  [WERROR="-Werror"])
AC_SUBST(WERROR)

glibgo_toolexecdir=no
glibgo_toolexeclibdir=no

AC_MSG_CHECKING([for --enable-version-specific-runtime-libs])
AC_ARG_ENABLE([version-specific-runtime-libs],
  AC_HELP_STRING([--enable-version-specific-runtime-libs],
                 [Specify that runtime libraries should be installed in a compiler-specific directory]),
  [case "$enableval" in
    yes) version_specific_libs=yes ;;
    no)  version_specific_libs=no ;;
    *)   AC_MSG_ERROR([Unknown argument to enable/disable version-specific libs]);;
   esac],
  [version_specific_libs=no])
AC_MSG_RESULT($version_specific_libs)

GCC_WITH_TOOLEXECLIBDIR

# Version-specific runtime libs processing.
if test $version_specific_libs = yes; then
  glibgo_toolexecdir='${libdir}/gcc/${host_alias}'
  glibgo_toolexeclibdir='${toolexecdir}/${gcc_version}$(MULTISUBDIR)'
fi

# Calculate glibgo_toolexecdir, glibgo_toolexeclibdir
# Install a library built with a cross compiler in tooldir, not libdir.
if test -n "$with_cross_host" &&
   test x"$with_cross_host" != x"no"; then
  nover_glibgo_toolexecdir='${exec_prefix}/${host_alias}'
  case ${with_toolexeclibdir} in
    no)
      nover_glibgo_toolexeclibdir='${toolexecdir}/lib'
      ;;
    *)
      nover_glibgo_toolexeclibdir=${with_toolexeclibdir}
      ;;
  esac
else
  nover_glibgo_toolexecdir='${libdir}/gcc/${host_alias}'
  nover_glibgo_toolexeclibdir='${libdir}'
fi
multi_os_directory=`$GOC -print-multi-os-directory`
case $multi_os_directory in
  .) ;; # Avoid trailing /.
  *) nover_glibgo_toolexeclibdir=${nover_glibgo_toolexeclibdir}/${multi_os_directory} ;;
esac

if test x"$glibgo_toolexecdir" = x"no"; then
  glibgo_toolexecdir="${nover_glibgo_toolexecdir}"
  glibgo_toolexeclibdir="${nover_glibgo_toolexeclibdir}"
fi

AC_SUBST(glibgo_toolexecdir)
AC_SUBST(glibgo_toolexeclibdir)
AC_SUBST(nover_glibgo_toolexeclibdir)

# See if the user wants to configure without libffi.  Some
# architectures don't support it.  FIXME: We should set a default
# based on the host.
AC_ARG_WITH(libffi,
  AS_HELP_STRING([--without-libffi],
                 [don't use libffi]),
  [:],
  [with_libffi=${with_libffi_default-yes}])

LIBFFI=
LIBFFIINCS=
if test "$with_libffi" != no; then
   AC_DEFINE(USE_LIBFFI, 1, [Define if we're to use libffi.])
   LIBFFI=../libffi/libffi_convenience.la
   LIBFFIINCS='-I$(top_srcdir)/../libffi/include -I../libffi/include'
fi
AC_SUBST(LIBFFI)
AC_SUBST(LIBFFIINCS)
AM_CONDITIONAL(USE_LIBFFI, test "$with_libffi" != "no")

# See if the user wants to configure without libatomic. This is useful if we are
# on an architecture for which libgo does not need an atomic support library and
# libatomic does not support our C compiler.
AC_ARG_WITH(libatomic,
  AS_HELP_STRING([--without-libatomic],
                 [don't use libatomic]),
  [:],
  [with_libatomic=${with_libatomic_default-yes}])

LIBATOMIC=
if test "$with_libatomic" != no; then
   LIBATOMIC=../libatomic/libatomic_convenience.la
fi
AC_SUBST(LIBATOMIC)

# Used to tell GNU make to include a file without telling automake to
# include it.
go_include="-include"
AC_SUBST(go_include)

# All known GOOS values.  This is the union of all operating systems
# supported by the gofrontend and all operating systems supported by
# the gc toolchain.
ALLGOOS="aix android darwin dragonfly freebsd hurd illumos ios irix js linux netbsd openbsd plan9 rtems solaris windows zos"

is_darwin=no
is_freebsd=no
is_irix=no
is_linux=no
is_netbsd=no
is_openbsd=no
is_dragonfly=no
is_rtems=no
is_solaris=no
is_aix=no
is_hurd=no
GOOS=unknown
case ${host} in
  *-*-darwin*)   is_darwin=yes;  GOOS=darwin ;;
  *-*-freebsd*)  is_freebsd=yes; GOOS=freebsd ;;
  *-*-irix6*)    is_irix=yes;    GOOS=irix ;;
  *-*-linux*)    is_linux=yes;   GOOS=linux ;;
  *-*-netbsd*)	 is_netbsd=yes;  GOOS=netbsd ;;
  *-*-openbsd*)  is_openbsd=yes; GOOS=openbsd ;;
  *-*-dragonfly*) is_dragonfly=yes; GOOS=dragonfly ;;
  *-*-rtems*)    is_rtems=yes;   GOOS=rtems ;;
  *-*-solaris2*) is_solaris=yes; GOOS=solaris ;;
  *-*-aix*)      is_aix=yes;     GOOS=aix ;;
  *-*-gnu*)      is_hurd=yes;    GOOS=hurd ;;
esac
AM_CONDITIONAL(LIBGO_IS_DARWIN, test $is_darwin = yes)
AM_CONDITIONAL(LIBGO_IS_FREEBSD, test $is_freebsd = yes)
AM_CONDITIONAL(LIBGO_IS_IRIX, test $is_irix = yes)
AM_CONDITIONAL(LIBGO_IS_LINUX, test $is_linux = yes)
AM_CONDITIONAL(LIBGO_IS_NETBSD, test $is_netbsd = yes)
AM_CONDITIONAL(LIBGO_IS_OPENBSD, test $is_openbsd = yes)
AM_CONDITIONAL(LIBGO_IS_DRAGONFLY, test $is_dragonfly = yes)
AM_CONDITIONAL(LIBGO_IS_RTEMS, test $is_rtems = yes)
AM_CONDITIONAL(LIBGO_IS_SOLARIS, test $is_solaris = yes)
AM_CONDITIONAL(LIBGO_IS_AIX, test $is_aix = yes)
AM_CONDITIONAL(LIBGO_IS_HURD, test $is_hurd = yes)
AM_CONDITIONAL(LIBGO_IS_BSD, test $is_darwin = yes -o $is_dragonfly = yes -o $is_freebsd = yes -o $is_netbsd = yes -o $is_openbsd = yes)
AC_SUBST(GOOS)
AC_SUBST(ALLGOOS)

if test "${GOOS}" = "unknown"; then
   AC_MSG_ERROR("could not determine GOOS from ${host}")
fi

dnl Test whether we need to use DejaGNU or whether we can use the
dnl simpler gotest approach.  We can only use gotest for a native
dnl build.
USE_DEJAGNU=no
case ${host} in
  *-*-rtems*) USE_DEJAGNU=yes ;;
  ${build}) ;;
  *) USE_DEJAGNU=yes ;;
esac
AC_SUBST(USE_DEJAGNU)

# All known GOARCH values.  This is the union of all architectures
# supported by the gofrontend and all architectures supported by the
# gc toolchain.
# To add a new architecture:
# - add it to this list
# - if appropriate, add an entry to ALLGOARCHFAMILY below
# - add an entry to the case on ${host} below to set GOARCH
# - update goarchList in libgo/go/go/build/syslist.go
# - update goarch.sh to report the values for this architecture
# - update go-set-goarch in gcc/testsuite/go.test/go-test.exp
# - update ptrSizeMap and intSizeMap in libgo/go/cmd/cgo/main.go
# - update arch lists in libgo/match.sh
# - update arch lists in libgo/testsuite/gotest
# - update +build lines in several places
#   - libgo/go/runtime/lfstack_NNbit.go
#   - libgo/go/runtime/hashNN.go
#   - libgo/go/runtime/unalignedN.go
#   - libgo/go/syscall/endian_XX.go
#   - possibly others
# - possibly update files in libgo/go/internal/syscall/unix
ALLGOARCH="386 alpha amd64 amd64p32 arm armbe arm64 arm64be ia64 m68k mips mipsle mips64 mips64le mips64p32 mips64p32le nios2 ppc ppc64 ppc64le riscv riscv64 s390 s390x sh shbe sparc sparc64 wasm"

# All known GOARCH family values.
ALLGOARCHFAMILY="I386 ALPHA AMD64 ARM ARM64 IA64 M68K MIPS MIPS64 NIOS2 PPC PPC64 RISCV RISCV64 S390 S390X SH SPARC SPARC64 WASM"

GOARCH=unknown
case ${host} in
  alpha*-*-*)
    GOARCH=alpha
    ;;
  aarch64-*-*)
    GOARCH=arm64
    ;;
  aarch64_be-*-*)
    GOARCH=arm64be
    ;;
  arm*-*-* | strongarm*-*-* | ep9312*-*-* | xscale-*-*)
    GOARCH=arm
    case ${host} in
      arm*b*-*-*)
	GOARCH=armbe
	;;
    esac
    ;;
changequote(,)dnl
  i[34567]86-*-* | x86_64-*-*)
changequote([,])dnl
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#ifdef __x86_64__
#error 64-bit
#endif
])],
	[GOARCH=386],
	AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#ifdef __ILP32__
#error x32
#endif
])],
	[GOARCH=amd64],
	[GOARCH=amd64p32]))
    ;;
  ia64-*-*)
    GOARCH=ia64
    ;;
  m68k*-*-*)
    GOARCH=m68k
    ;;
  mips*-*-*)
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#if _MIPS_SIM != _ABIO32
#error not o32
#endif
])],
[mips_abi="o32"],
	[AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#if _MIPS_SIM != _ABIN32
#error not n32
#endif
])],
[mips_abi="n32"],
	[AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#if _MIPS_SIM != _ABI64
#error not n64
#endif
])],
[mips_abi="n64"],
	[AC_MSG_ERROR([unknown MIPS ABI])
[mips_abi="n32"]])])])
    case "$mips_abi" in
    "o32") GOARCH=mips ;;
    "n32") GOARCH=mips64p32 ;;
    "n64") GOARCH=mips64 ;;
    esac
    case "${host}" in
    mips*el-*-*)
        GOARCH="${GOARCH}le"
        ;;
    esac
    ;;
  nios2-*-*)
    GOARCH=nios2
    ;;
  rs6000*-*-* | powerpc*-*-*)
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#ifdef _ARCH_PPC64
#error 64-bit
#endif
])],
[GOARCH=ppc],
    [
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#if defined(_BIG_ENDIAN) || defined(__BIG_ENDIAN__)
#error 64be
#endif
])],
[GOARCH=ppc64le],
[GOARCH=ppc64])])
    ;;
  riscv*-*-*)
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#if __riscv_xlen == 64
#error 64-bit
#endif
])],
[GOARCH=riscv],
[GOARCH=riscv64])
    ;;
  s390*-*-*)
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#if defined(__s390x__)
#error 64-bit
#endif
])],
[GOARCH=s390],
[GOARCH=s390x])
    ;;
  sh3eb*-*-* | sh4eb*-*-*)
    GOARCH=shbe
    ;;
  sh3*-*-* | sh4*-*-*)
    GOARCH=sh
    ;;
  sparc*-*-*)
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#if defined(__sparcv9) || defined(__arch64__)
#error 64-bit
#endif
])],
[GOARCH=sparc],
[GOARCH=sparc64])
    ;;
esac
AC_SUBST(GOARCH)
AC_SUBST(ALLGOARCH)
AC_SUBST(ALLGOARCHFAMILY)

if test "${GOARCH}" = "unknown"; then
   AC_MSG_ERROR("could not determine GOARCH from ${host}")
fi

AM_CONDITIONAL(LIBGO_IS_X86, test "$GOARCH" = "386" -o "$GOARCH" = "amd64" -o "$GOARCH" = "amd64p32")

FUNCTION_DESCRIPTORS=false
case ${host} in
  rs6000*-*-* | powerpc*-*-*)
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#if _CALL_ELF == 1
#error descriptors
#endif
])],
	[FUNCTION_DESCRIPTORS=false],
	[FUNCTION_DESCRIPTORS=true])
    ;;
esac
AC_SUBST(FUNCTION_DESCRIPTORS)

dnl Some files are only present when needed for specific architectures.
GO_LIBCALL_OS_FILE=
GO_LIBCALL_OS_ARCH_FILE=
GO_SYSCALL_OS_FILE=
GO_SYSCALL_OS_ARCH_FILE=
if test -f "${srcdir}/go/syscall/libcall_${GOOS}.go"; then
  GO_LIBCALL_OS_FILE="go/syscall/libcall_${GOOS}.go"
fi
if test -f "${srcdir}/go/syscall/libcall_${GOOS}_${GOARCH}.go"; then
  GO_LIBCALL_OS_ARCH_FILE="go/syscall/libcall_${GOOS}_${GOARCH}.go"
fi
if test -f "${srcdir}/go/syscall/syscall_${GOOS}.go"; then
  GO_SYSCALL_OS_FILE="go/syscall/syscall_${GOOS}.go"
fi
if test -f "${srcdir}/go/syscall/syscall_${GOOS}_${GOARCH}.go"; then
  GO_SYSCALL_OS_ARCH_FILE="go/syscall/syscall_${GOOS}_${GOARCH}.go"
fi
AC_SUBST(GO_LIBCALL_OS_FILE)
AC_SUBST(GO_LIBCALL_OS_ARCH_FILE)
AC_SUBST(GO_SYSCALL_OS_FILE)
AC_SUBST(GO_SYSCALL_OS_ARCH_FILE)

dnl Special flags used to generate sysinfo.go.
OSCFLAGS="-D_GNU_SOURCE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64"
case "$target" in
    mips-sgi-irix6.5*)
	# IRIX 6 needs _XOPEN_SOURCE=500 for the XPG5 version of struct
	# msghdr in <sys/socket.h>.
	OSCFLAGS="$OSCFLAGS -D_XOPEN_SOURCE=500"
	;;
    *-*-solaris2.*)
	# Solaris 10+ needs this so struct msghdr gets the msg_control
	# etc. fields in <sys/socket.h> (_XPG4_2).  _XOPEN_SOURCE=600 as
	# above doesn't work with C99.
	OSCFLAGS="$OSCFLAGS -std=gnu99 -D_XOPEN_SOURCE=600 -D__EXTENSIONS__"
	;;
esac
AC_SUBST(OSCFLAGS)

dnl Check if assembler supports disabling hardware capability support.
GCC_CHECK_ASSEMBLER_HWCAP

dnl Use -fsplit-stack when compiling C code if available.
AC_CACHE_CHECK([whether -fsplit-stack is supported],
[libgo_cv_c_split_stack_supported],
[CFLAGS_hold=$CFLAGS
CFLAGS="$CFLAGS -fsplit-stack"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([int i;])],
[libgo_cv_c_split_stack_supported=yes],
[libgo_cv_c_split_stack_supported=no])
CFLAGS=$CFLAGS_hold])

dnl Make sure the linker permits -fsplit-stack.  Old versions of gold will
dnl reject split-stack code calling non-split-stack code on targets
dnl they don't support.
AC_CACHE_CHECK([whether linker supports split/non-split linked together],
[libgo_cv_c_linker_split_non_split],
[cat > conftest1.c << EOF
extern void f();
int main() { f(); return 0; }
EOF
cat > conftest2.c << EOF
void f() {}
EOF
$CC -c -fsplit-stack $CFLAGS $CPPFLAGS conftest1.c >/dev/null 2>&1
$CC -c $CFLAGS $CPPFLAGS conftest2.c > /dev/null 2>&1
if $CC -o conftest conftest1.$ac_objext conftest2.$ac_objext > /dev/null 2>&1; then
  libgo_cv_c_linker_split_non_split=yes
else
  libgo_cv_c_linker_split_non_split=no
fi
rm -f conftest1.* conftest2.* conftest])

if test "$libgo_cv_c_split_stack_supported" = yes -a "$libgo_cv_c_linker_split_non_split" = yes; then
  SPLIT_STACK=-fsplit-stack
  AC_DEFINE(USING_SPLIT_STACK, 1,
		[Define if the compiler supports -fsplit-stack])
else
  SPLIT_STACK=
fi
AC_SUBST(SPLIT_STACK)
AM_CONDITIONAL(USING_SPLIT_STACK,
	test "$libgo_cv_c_split_stack_supported" = yes -a "$libgo_cv_c_linker_split_non_split" = yes)

dnl If the compiler supports split-stack but the linker does not, then
dnl we need to explicitly disable split-stack for Go.
if test "$libgo_cv_c_split_stack_supported" = yes -a "$libgo_cv_c_linker_split_non_split" = no; then
  GO_SPLIT_STACK=-fno-split-stack
else
  GO_SPLIT_STACK=
fi
AC_SUBST(GO_SPLIT_STACK)

dnl Check whether the linker does stack munging when calling from
dnl split-stack into non-split-stack code.  We check this by looking
dnl at the --help output.  FIXME: This is only half right: it's
dnl possible for the linker to support this for some targets but not
dnl others.
dnl This is slightly different from the above check, which is whether
dnl the linker permits the call at all.
AC_CACHE_CHECK([whether linker supports split stack],
[libgo_cv_c_linker_supports_split_stack],
[libgo_cv_c_linker_supports_split_stack=no
if $GOC -Wl,--help 2>/dev/null | grep split-stack-adjust-size >/dev/null 2>&1; then
  libgo_cv_c_linker_supports_split_stack=yes
fi])
if test "$libgo_cv_c_linker_supports_split_stack" = yes; then
  AC_DEFINE(LINKER_SUPPORTS_SPLIT_STACK, 1,
	    [Define if the linker support split stack adjustments])
fi

AC_CACHE_CHECK([whether compiler is llgo],
[libgo_cv_c_goc_is_llgo],
[libgo_cv_c_goc_is_llgo=no
if $GOC -dumpversion 2>/dev/null | grep llgo >/dev/null 2>&1; then
  libgo_cv_c_goc_is_llgo=yes
fi])
AM_CONDITIONAL(GOC_IS_LLGO, test "$libgo_cv_c_goc_is_llgo" = yes)

dnl Test for the -lm library.
MATH_LIBS=
AC_CHECK_LIB([m], [sqrt], MATH_LIBS=-lm)
AC_SUBST(MATH_LIBS)

dnl Test for -lsocket and -lnsl.  Copied from libjava/configure.ac.
AC_CACHE_CHECK([for socket libraries], libgo_cv_lib_sockets,
  [libgo_cv_lib_sockets=
   libgo_check_both=no
   AC_CHECK_FUNC(connect, libgo_check_socket=no, libgo_check_socket=yes)
   if test "$libgo_check_socket" = "yes"; then
     unset ac_cv_func_connect
     AC_CHECK_LIB(socket, main, libgo_cv_lib_sockets="-lsocket",
     		  libgo_check_both=yes)
   fi
   if test "$libgo_check_both" = "yes"; then
     libgo_old_libs=$LIBS
     LIBS="$LIBS -lsocket -lnsl"
     unset ac_cv_func_accept
     AC_CHECK_FUNC(accept,
		   [libgo_check_nsl=no
		    libgo_cv_lib_sockets="-lsocket -lnsl"])
     unset ac_cv_func_accept
     LIBS=$libgo_old_libs
   fi
   unset ac_cv_func_gethostbyname
   libgo_old_libs="$LIBS"
   AC_CHECK_FUNC(gethostbyname, ,
		 [AC_CHECK_LIB(nsl, main,
		 	[libgo_cv_lib_sockets="$libgo_cv_lib_sockets -lnsl"])])
   unset ac_cv_func_gethostbyname
   AC_CHECK_FUNC(sendfile, ,
		 [AC_CHECK_LIB(sendfile, main,
		 	[libgo_cv_lib_sockets="$libgo_cv_lib_sockets -lsendfile"])])
   LIBS=$libgo_old_libs
])
NET_LIBS="$libgo_cv_lib_sockets"
AC_SUBST(NET_LIBS)

dnl Test whether the compiler supports the -pthread option.
AC_CACHE_CHECK([whether -pthread is supported],
[libgo_cv_lib_pthread],
[CFLAGS_hold=$CFLAGS
CFLAGS="$CFLAGS -pthread -L../libatomic/.libs"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([int i;])],
[libgo_cv_lib_pthread=yes],
[libgo_cv_lib_pthread=no])
CFLAGS=$CFLAGS_hold])
PTHREAD_CFLAGS=
if test "$libgo_cv_lib_pthread" = yes; then
  # RISC-V apparently adds -latomic when using -pthread.
  PTHREAD_CFLAGS="-pthread -L../libatomic/.libs"
fi
AC_SUBST(PTHREAD_CFLAGS)

dnl Test for the -lpthread library.
PTHREAD_LIBS=
AC_CHECK_LIB([pthread], [pthread_create], PTHREAD_LIBS=-lpthread)
AC_SUBST(PTHREAD_LIBS)

dnl Test if -lucontext is required for makecontext.
AC_SEARCH_LIBS([makecontext], [ucontext])

dnl Test if -lrt is required for sched_yield or nanosleep or clock_gettime.
AC_SEARCH_LIBS([sched_yield], [rt])
AC_SEARCH_LIBS([nanosleep], [rt])
AC_SEARCH_LIBS([clock_gettime], [rt])

AC_C_BIGENDIAN

GCC_CHECK_UNWIND_GETIPINFO

AC_CHECK_HEADERS(fcntl.h port.h sched.h semaphore.h sys/file.h sys/mman.h syscall.h sys/epoll.h sys/event.h sys/inotify.h sys/ptrace.h sys/syscall.h sys/sysctl.h sys/user.h sys/utsname.h sys/select.h sys/socket.h net/bpf.h net/if.h net/if_arp.h net/route.h netpacket/packet.h sys/prctl.h sys/mount.h sys/vfs.h sys/statfs.h sys/timex.h sys/sysinfo.h utime.h linux/ether.h linux/fs.h linux/ptrace.h linux/reboot.h netinet/in_syst.h netinet/ip.h netinet/ip_mroute.h netinet/if_ether.h lwp.h)

AC_CHECK_HEADERS([netinet/icmp6.h], [], [],
[#include <netinet/in.h>
])

AC_CHECK_HEADERS([linux/filter.h linux/if_addr.h linux/if_ether.h linux/if_tun.h linux/netlink.h linux/rtnetlink.h], [], [],
[#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

AM_CONDITIONAL(HAVE_SYS_MMAN_H, test "$ac_cv_header_sys_mman_h" = yes)

AC_CHECK_FUNCS(strerror_r strsignal wait4 mincore setenv unsetenv dl_iterate_phdr memmem)
AM_CONDITIONAL(HAVE_STRERROR_R, test "$ac_cv_func_strerror_r" = yes)
AM_CONDITIONAL(HAVE_WAIT4, test "$ac_cv_func_wait4" = yes)

AC_STRUCT_DIRENT_D_TYPE

AC_CHECK_FUNCS(accept4 dup3 epoll_create1 faccessat fallocate fchmodat fchownat futimesat getxattr inotify_add_watch inotify_init inotify_init1 inotify_rm_watch listxattr mkdirat mknodat open64 openat pipe2 removexattr renameat setxattr sync_file_range splice syscall tee unlinkat unshare utimensat)
AC_TYPE_OFF_T

CFLAGS_hold="$CFLAGS"
CFLAGS="$OSCFLAGS $CFLAGS"
AC_CHECK_TYPES([loff_t], [], [], [[
#include <sys/types.h>
#include <fcntl.h>
]])
CFLAGS="$CFLAGS_hold"

LIBS_hold="$LIBS"
LIBS="$LIBS -lm"
AC_CHECK_FUNCS(cosl expl logl sinl tanl acosl asinl atanl atan2l expm1l ldexpl log10l log1pl)
LIBS="$LIBS_hold"

CFLAGS_hold="$CFLAGS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
LIBS_hold="$LIBS"
LIBS="$LIBS $PTHREAD_LIBS"
AC_CHECK_FUNCS(sem_timedwait)
CFLAGS="$CFLAGS_hold"
LIBS="$LIBS_hold"

LIBS_hold="$LIBS"
LIBS="$LIBS $MATH_LIBS"
AC_CHECK_FUNCS(matherr)
LIBS="$LIBS_hold"

dnl For x86 we want to use the -minline-all-stringops option to avoid
dnl forcing a stack split when calling memcpy and friends.
AC_CACHE_CHECK([whether compiler supports -minline-all-stringops],
[libgo_cv_c_stringops],
[CFLAGS_hold=$CFLAGS
CFLAGS="$CFLAGS -minline-all-stringops"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([int i;])],
[libgo_cv_c_stringops=yes],
[libgo_cv_c_stringops=no])
CFLAGS=$CFLAGS_hold])
STRINGOPS_FLAG=
if test "$libgo_cv_c_stringops" = yes; then
  STRINGOPS_FLAG=-minline-all-stringops
fi
AC_SUBST(STRINGOPS_FLAG)

dnl For x86 we want to compile the math library with -mfancy-math-387
dnl so that we can use the builtin instructions directly.
AC_CACHE_CHECK([whether compiler supports -mfancy-math-387],
[libgo_cv_c_fancymath],
[CFLAGS_hold=$CFLAGS
CFLAGS="$CFLAGS -mfancy-math-387"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([int i;])],
[libgo_cv_c_fancymath=yes],
[libgo_cv_c_fancymath=no])
CFLAGS=$CFLAGS_hold])
MATH_FLAG=
if test "$libgo_cv_c_fancymath" = yes; then
  MATH_FLAG="-mfancy-math-387"
fi
MATH_FLAG="${MATH_FLAG} -ffp-contract=off -fno-math-errno -fno-trapping-math"
AC_SUBST(MATH_FLAG)

CFLAGS_hold=$CFLAGS
CFLAGS="$CFLAGS -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE"
AC_CHECK_TYPES([off64_t])
CFLAGS=$CFLAGS_hold

dnl Work out the size of the epoll_events struct on GNU/Linux.
AC_CACHE_CHECK([epoll_event size],
[libgo_cv_c_epoll_event_size],
[AC_COMPUTE_INT(libgo_cv_c_epoll_event_size,
[sizeof (struct epoll_event)],
[#include <sys/epoll.h>],
[libgo_cv_c_epoll_event_size=0])])
SIZEOF_STRUCT_EPOLL_EVENT=${libgo_cv_c_epoll_event_size}
AC_SUBST(SIZEOF_STRUCT_EPOLL_EVENT)

dnl Work out the offset of the fd field in the epoll_events struct on
dnl GNU/Linux.
AC_CACHE_CHECK([epoll_event data.fd offset],
[libgo_cv_c_epoll_event_fd_offset],
[AC_COMPUTE_INT(libgo_cv_c_epoll_event_fd_offset,
[offsetof (struct epoll_event, data.fd)],
[#include <stddef.h>
#include <sys/epoll.h>],
[libgo_cv_c_epoll_event_fd_offset=0])])
STRUCT_EPOLL_EVENT_FD_OFFSET=${libgo_cv_c_epoll_event_fd_offset}
AC_SUBST(STRUCT_EPOLL_EVENT_FD_OFFSET)

dnl Check if <sys/stat.h> uses timespec_t for st_?tim members.  Introduced
dnl in Solaris 11.4 for XPG7 compatibility.
AC_EGREP_HEADER([timespec_t.*st_atim], [sys/stat.h],
		[have_stat_timespec=yes], [have_stat_timespec=no])
AM_CONDITIONAL(HAVE_STAT_TIMESPEC, test $have_stat_timespec = yes)

dnl See if struct exception is defined in <math.h>.
AC_CHECK_TYPE([struct exception],
[libgo_has_struct_exception=yes],
[libgo_has_struct_exception=no],
[#include <math.h>])
if test "$libgo_has_struct_exception" = "yes"; then
  AC_DEFINE(HAVE_STRUCT_EXCEPTION, 1,
            [Define to 1 if <math.h> defines struct exception])
fi

dnl See whether setcontext changes the value of TLS variables.
AC_CACHE_CHECK([whether setcontext clobbers TLS variables],
[libgo_cv_lib_setcontext_clobbers_tls],
[CFLAGS_hold="$CFLAGS"
CFLAGS="$PTHREAD_CFLAGS"
LIBS_hold="$LIBS"
LIBS="$LIBS $PTHREAD_LIBS"
AC_CHECK_SIZEOF([void *])
AS_VAR_ARITH([ptr_type_size], [$ac_cv_sizeof_void_p \* 8])
AC_RUN_IFELSE(
  [AC_LANG_SOURCE([
#include <pthread.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>

__thread int tls;

static char stack[[10 * 1024 * 1024]];
static ucontext_t c;

/* Called via makecontext/setcontext.  */

static void
cfn (void)
{
  exit (tls);
}

/* Called via pthread_create.  */

static void *
tfn (void *dummy)
{
  /* The thread should still see this value after calling
     setcontext.  */
  tls = 0;

  setcontext (&c);

  /* The call to setcontext should not return.  */
  abort ();
}

int
main ()
{
  pthread_t tid;

  /* The thread should not see this value.  */
  tls = 1;

  if (getcontext (&c) < 0)
    abort ();

  c.uc_stack.ss_sp = stack;
#ifdef MAKECONTEXT_STACK_TOP
  c.uc_stack.ss_sp += sizeof stack;
#endif
  c.uc_stack.ss_flags = 0;
  c.uc_stack.ss_size = sizeof stack;
  c.uc_link = NULL;
  makecontext (&c, cfn, 0);

  if (pthread_create (&tid, NULL, tfn, NULL) != 0)
    abort ();

  if (pthread_join (tid, NULL) != 0)
    abort ();

  /* The thread should have called exit.  */
  abort ();
}
])],
[libgo_cv_lib_setcontext_clobbers_tls=no],
[libgo_cv_lib_setcontext_clobbers_tls=yes],
[case "$target:$ptr_type_size" in
  i?86-*-solaris2.1[[01]]:64 | x86_64*-*-solaris2.1[[01]]:64)
    libgo_cv_lib_setcontext_clobbers_tls=yes ;;
  *)
    libgo_cv_lib_setcontext_clobbers_tls=no ;;
 esac
])
CFLAGS="$CFLAGS_hold"
LIBS="$LIBS_hold"
])
if test "$libgo_cv_lib_setcontext_clobbers_tls" = "yes"; then
  AC_DEFINE(SETCONTEXT_CLOBBERS_TLS, 1,
	    [Define if setcontext clobbers TLS variables])
fi

AC_CACHE_CHECK([whether .eh_frame section should be read-only],
libgo_cv_ro_eh_frame, [
libgo_cv_ro_eh_frame=no
echo 'extern void foo (void); void bar (void) { foo (); foo (); }' > conftest.c
if $CC $CFLAGS -S -fpic -fexceptions -o conftest.s conftest.c > /dev/null 2>&1; then
  if grep '.section.*eh_frame.*"a"' conftest.s > /dev/null; then
    libgo_cv_ro_eh_frame=yes
  elif grep '.section.*eh_frame.*#alloc' conftest.c \
       | grep -v '#write' > /dev/null; then
    libgo_cv_ro_eh_frame=yes
  fi
fi
rm -f conftest.*
])
if test "x$libgo_cv_ro_eh_frame" = xyes; then
  AC_DEFINE(EH_FRAME_FLAGS, "a",
	    [Define to the flags needed for the .section .eh_frame directive.])
else
  AC_DEFINE(EH_FRAME_FLAGS, "aw",
	    [Define to the flags needed for the .section .eh_frame directive.])
fi

AC_CACHE_CHECK([if compiler supports -Qunused-arguments],
[libgo_cv_c_unused_arguments],
[CFLAGS_hold=$CFLAGS
CFLAGS="$CFLAGS -Qunused-arguments"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([int i;])],
[libgo_cv_c_unused_arguments=yes],
[libgo_cv_c_unused_arguments=no])
CFLAGS=$CFLAGS_hold])

AC_CACHE_CHECK([if assembler supports GNU comdat group syntax],
libgo_cv_as_comdat_gnu, [
echo '.section .text,"axG",@progbits,.foo,comdat' > conftest.s
CFLAGS_hold=$CFLAGS
if test "$libgo_cv_c_unused_arguments" = yes; then
  CFLAGS="$CFLAGS -Qunused-arguments"
fi
if $CC $CFLAGS -c conftest.s > /dev/null 2>&1; then
  libgo_cv_as_comdat_gnu=yes
else
  libgo_cv_as_comdat_gnu=no
fi
CFLAGS=$CFLAGS_hold
])
if test "x$libgo_cv_as_comdat_gnu" = xyes; then
  AC_DEFINE(HAVE_AS_COMDAT_GAS, 1,
	    [Define if your assembler supports GNU comdat group syntax.])
fi

AC_CACHE_CHECK([assembler supports pc related relocs],
libgo_cv_as_x86_pcrel, [
libgo_cv_as_x86_pcrel=yes
echo '.text; foo: nop; .data; .long foo-.; .text' > conftest.s
CFLAGS_hold=$CFLAGS
if test "$libgo_cv_c_unused_arguments" = yes; then
  CFLAGS="$CFLAGS -Qunused-arguments"
fi
if $CC $CFLAGS -c conftest.s 2>&1 | $EGREP -i 'illegal|warning' > /dev/null; then
    libgo_cv_as_x86_pcrel=no
fi
CFLAGS=$CFLAGS_hold
])
if test "x$libgo_cv_as_x86_pcrel" = xyes; then
  AC_DEFINE(HAVE_AS_X86_PCREL, 1,
	    [Define if your assembler supports PC relative relocs.])
fi

AC_CACHE_CHECK([assembler supports unwind section type],
libgo_cv_as_x86_64_unwind_section_type, [
libgo_cv_as_x86_64_unwind_section_type=yes
echo '.section .eh_frame,"a",@unwind' > conftest.s
CFLAGS_hold=$CFLAGS
if test "$libgo_cv_c_unused_arguments" = yes; then
  CFLAGS="$CFLAGS -Qunused-arguments"
fi
if $CC $CFLAGS -c conftest.s 2>&1 | grep -i warning > /dev/null; then
    libgo_cv_as_x86_64_unwind_section_type=no
fi
CFLAGS=$CFLAGS_hold
])
if test "x$libgo_cv_as_x86_64_unwind_section_type" = xyes; then
  AC_DEFINE(HAVE_AS_X86_64_UNWIND_SECTION_TYPE, 1,
	    [Define if your assembler supports unwind section type.])
fi

AC_CACHE_CHECK([assembler supports AES instructions],
libgo_cv_as_x86_aes, [
libgo_cv_as_x86_aes=yes
echo 'aesenc %xmm0, %xmm1' > conftest.s
CFLAGS_hold=$CFLAGS
if test "$libgo_cv_c_unused_arguments" = yes; then
  CFLAGS="$CFLAGS -Qunused-arguments"
fi
if $CC $CFLAGS -c conftest.s 2>&1 | grep -i error > /dev/null; then
    libgo_cv_as_x86_aes=no
fi
CFLAGS=$CFLAGS_hold
])
if test "x$libgo_cv_as_x86_aes" = xyes; then
  AC_DEFINE(HAVE_AS_X86_AES, 1,
	    [Define if your assembler supports AES instructions.])
fi

dnl Test whether the linker supports the -static option.
AC_CACHE_CHECK([whether -static is supported],
[libgo_cv_ld_static],
[LDFLAGS_hold=$LDFLAGS
LDFLAGS="$LDFLAGS -static"
AC_LINK_IFELSE([AC_LANG_PROGRAM(,)],
[libgo_cv_ld_static=yes],
[libgo_cv_ld_static=no])
LDFLAGS=$LDFLAGS_hold])
AM_CONDITIONAL(HAVE_STATIC_LINK, test "$libgo_cv_ld_static" = yes)

AC_SUBST(SYSROOT_CFLAGS_FOR_TARGET)

AC_CACHE_SAVE

if test ${multilib} = yes; then
  multilib_arg="--enable-multilib"
else
  multilib_arg=
fi

AC_CONFIG_FILES(Makefile testsuite/Makefile testsuite/libgo-test-support.exp)

AC_CONFIG_COMMANDS([default],
[if test -n "$CONFIG_FILES"; then
   # Multilibs need MULTISUBDIR defined correctly in certain makefiles so
   # that multilib installs will end up installed in the correct place.
   # The testsuite needs it for multilib-aware ABI baseline files.
   # To work around this not being passed down from config-ml.in ->
   # srcdir/Makefile.am -> srcdir/{src,libsupc++,...}/Makefile.am, manually
   # append it here.  Only modify Makefiles that have just been created.
   #
   # Also, get rid of this simulated-VPATH thing that automake does.
   cat > vpsed << \_EOF
s!`test -f '$<' || echo '$(srcdir)/'`!!
_EOF
   for i in $SUBDIRS; do
    case $CONFIG_FILES in
     *${i}/Makefile*)
       #echo "Adding MULTISUBDIR to $i/Makefile"
       sed -f vpsed $i/Makefile > tmp
       grep '^MULTISUBDIR =' Makefile >> tmp
       mv tmp $i/Makefile
       ;;
    esac
   done
   rm vpsed
 fi
],
[
# Variables needed in config.status (file generation) which aren't already
# passed by autoconf.
SUBDIRS="$SUBDIRS"
])

AC_OUTPUT
