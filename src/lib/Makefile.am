if PLATFORM_WIN32
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -avoid-version
else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
endif

lib_LTLIBRARIES = libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@.la
libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_includedir = $(includedir)/libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@/libfreehand
libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_include_HEADERS = \
	libfreehand.h \
	FreeHandDocument.h \
	FHStringVector.h

AM_CXXFLAGS = $(WPD_CFLAGS) $(WPG_CFLAGS) $(ZLIB_CFLAGS) $(DEBUG_CXXFLAGS)

libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_LIBADD  = $(WPD_LIBS) $(ZLIB_LIBS) @LIBFREEHAND_WIN32_RESOURCE@
libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_DEPENDENCIES = @LIBFREEHAND_WIN32_RESOURCE@
libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic -no-undefined
libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_SOURCES = \
	FHCollector.cpp \
	FreeHandDocument.cpp \
	FHInternalStream.cpp \
	FHParser.cpp \
	FHStringVector.cpp \
	FHSVGGenerator.cpp \
	libfreehand_utils.cpp \
	FHCollector.h \
	FHInternalStream.h \
	FHParser.h \
	FHSVGGenerator.h \
	libfreehand_utils.h 

if OS_WIN32

@LIBFREEHAND_WIN32_RESOURCE@ : libfreehand.rc $(libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource libfreehand.rc @LIBFREEHAND_WIN32_RESOURCE@

endif


EXTRA_DIST = \
	libfreehand.rc.in \
	makefile.mk

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	libfreehand.rc
