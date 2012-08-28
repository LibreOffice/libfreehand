bin_PROGRAMS = fh2raw

AM_CXXFLAGS = -I$(top_srcdir)/src/lib -I$(top_builddir)/src/lib $(LIBFREEHAND_CXXFLAGS) $(DEBUG_CXXFLAGS)

fh2raw_DEPENDENCIES = @PPP2RAW_WIN32_RESOURCE@

if STATIC_TOOLS
fh2raw_LDADD = ../../lib/@FH_OBJDIR@/libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@.a $(LIBFREEHAND_LIBS) @PPP2RAW_WIN32_RESOURCE@
fh2raw_LDFLAGS = -all-static 
else
fh2raw_LDADD = ../../lib/libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@.la $(LIBFREEHAND_LIBS) @PPP2RAW_WIN32_RESOURCE@ 
endif

fh2raw_SOURCES = \
	fh2raw.cpp

if OS_WIN32

@PPP2RAW_WIN32_RESOURCE@ : fh2raw.rc $(fh2raw_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource fh2raw.rc @PPP2RAW_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(fh2raw_SOURCES) \
	fh2raw.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	fh2raw.rc	 
