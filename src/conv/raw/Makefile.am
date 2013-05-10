bin_PROGRAMS = fh2raw

AM_CXXFLAGS = -I$(top_srcdir)/inc -I$(top_builddir)/src/lib $(WPD_CFLAGS) $(WPG_CFLAGS) $(DEBUG_CXXFLAGS)

fh2raw_DEPENDENCIES = @FH2RAW_WIN32_RESOURCE@
fh2raw_LDADD = ../../lib/libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@.la $(WPD_LIBS) $(ZLIB_LIBS) @FH2RAW_WIN32_RESOURCE@ 

fh2raw_SOURCES = \
	fh2raw.cpp

if OS_WIN32

@FH2RAW_WIN32_RESOURCE@ : fh2raw.rc $(fh2raw_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource fh2raw.rc @FH2RAW_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(fh2raw_SOURCES) \
	fh2raw.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	fh2raw.rc	 
