bin_PROGRAMS = fh2xhtml

AM_CXXFLAGS = -I$(top_srcdir)/src/lib -I$(top_builddir)/src/lib $(WPD_CFLAGS) $(WPG_CFLAGS) $(DEBUG_CXXFLAGS)

fh2xhtml_DEPENDENCIES = @FH2XHTML_WIN32_RESOURCE@

if STATIC_TOOLS
fh2xhtml_LDADD = ../../lib/@FH_OBJDIR@/libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@.a $(WPD_LIBS) $(ZLIB_LIBS) @FH2XHTML_WIN32_RESOURCE@
fh2xhtml_LDFLAGS = -all-static 
else
fh2xhtml_LDADD = ../../lib/libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@.la $(WPD_LIBS) $(ZLIB_LIBS) @FH2XHTML_WIN32_RESOURCE@ 
endif

fh2xhtml_SOURCES = \
	fh2xhtml.cpp

if OS_WIN32

@FH2XHTML_WIN32_RESOURCE@ : fh2xhtml.rc $(fh2xhtml_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource fh2xhtml.rc @FH2XHTML_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(fh2xhtml_SOURCES) \
	fh2xhtml.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	fh2xhtml.rc
