include $(top_srcdir)/config/Rules.am

AUTOMAKE_OPTIONS = subdir-objects

DEFAULT_INCLUDES += \
	-I$(top_srcdir)/include

libexec_PROGRAMS = zfs-pool-importer

zfs-pool-importer_SOURCES = \
	main.m \
	launch.h \
	launch.m \
	stdout.h \
	stdout.m

zfs-pool-importer_CFLAGS = -stdlib=libc
zfs-pool-importer_OBJCFLAGS = -stdlib=libc
zfs-pool-importer_LDFLAGS = -static -framework CoreFoundation -framework Foundation
