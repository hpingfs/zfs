include $(top_srcdir)/config/Rules.am

# Suppress unused but set variable warnings often due to ASSERTs
AM_CFLAGS += $(NO_UNUSED_BUT_SET_VARIABLE)
AM_CFLAGS += $(LIBBLKID_CFLAGS) $(LIBUDEV_CFLAGS)
AM_CFLAGS += -fvisibility=hidden

# for sys/vnode.h
DEFAULT_INCLUDES += -I$(top_srcdir)/include/spl

noinst_LTLIBRARIES = libuzfs.la

USER_C = \
	libuzfs.c

libuzfs_la_SOURCES = $(USER_C)

libuzfs_la_LIBADD = \
	$(abs_top_builddir)/lib/libzfs/libzfs.la \
	$(abs_top_builddir)/lib/libzfs_core/libzfs_core.la \
	$(abs_top_builddir)/lib/libnvpair/libnvpair.la \
	$(abs_top_builddir)/lib/libuutil/libuutil.la \
	$(abs_top_builddir)/lib/libzpool/libzpool.la

libuzfs_la_LIBADD += -lm -lrt -luuid -lz $(LIBBLKID_LIBS) $(LIBUDEV_LIBS)

include $(top_srcdir)/config/CppCheck.am
