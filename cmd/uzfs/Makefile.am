include $(top_srcdir)/config/Rules.am

sbin_PROGRAMS = uzfs

DEFAULT_INCLUDES += -I$(top_srcdir)/include/spl

uzfs_SOURCES = \
	uzfs.c

uzfs_LDADD = \
	$(abs_top_builddir)/lib/libzfs/libzfs.la \
	$(abs_top_builddir)/lib/libzfs_core/libzfs_core.la \
	$(abs_top_builddir)/lib/libnvpair/libnvpair.la \
	$(abs_top_builddir)/lib/libuutil/libuutil.la \
	$(abs_top_builddir)/lib/libzpool/libzpool.la \
	$(abs_top_builddir)/lib/libuzfs/libuzfs.la

uzfs_LDADD += $(LTLIBINTL) -lpthread

include $(top_srcdir)/config/CppCheck.am
