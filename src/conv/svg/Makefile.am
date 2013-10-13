bin_PROGRAMS = fh2svg

AM_CXXFLAGS = -I$(top_srcdir)/inc -I$(top_builddir)/src/lib $(WPD_CFLAGS) $(WPG_CFLAGS) $(DEBUG_CXXFLAGS)

fh2svg_DEPENDENCIES = @FH2SVG_WIN32_RESOURCE@

fh2svg_LDADD = ../../lib/libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@.la $(WPD_LIBS) $(ZLIB_LIBS) @FH2SVG_WIN32_RESOURCE@ 

fh2svg_SOURCES = \
	fh2svg.cpp

if OS_WIN32

@FH2SVG_WIN32_RESOURCE@ : fh2svg.rc $(fh2svg_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource fh2svg.rc @FH2SVG_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(fh2svg_SOURCES) \
	fh2svg.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	fh2svg.rc
