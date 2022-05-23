include $(top_srcdir)/config/Rules.am

# Unconditionally enable debugging for zdb
AM_CPPFLAGS += -DDEBUG -UNDEBUG -DZFS_DEBUG

sbin_PROGRAMS = zdb

DEFAULT_INCLUDES += -I$(top_srcdir)/include/spl
DEFAULT_INCLUDES += -I$(top_srcdir)/include/os/linux/zfs

zdb_SOURCES = \
	zdb.c \
	zdb_il.c \
	zdb.h

zdb_LDADD = \
	$(abs_top_builddir)/lib/libzpool/libzpool.la \
	$(abs_top_builddir)/lib/libzfs_core/libzfs_core.la \
	$(abs_top_builddir)/lib/libnvpair/libnvpair.la

include $(top_srcdir)/config/CppCheck.am
