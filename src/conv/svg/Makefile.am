bin_PROGRAMS = fh2xhtml

AM_CXXFLAGS = -I$(top_srcdir)/src/lib -I$(top_builddir)/src/lib $(LIBFREEHAND_CXXFLAGS) $(DEBUG_CXXFLAGS)

fh2xhtml_DEPENDENCIES = @PPP2XHTML_WIN32_RESOURCE@

if STATIC_TOOLS
fh2xhtml_LDADD = ../../lib/@FH_OBJDIR@/libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@.a $(LIBFREEHAND_LIBS) @PPP2XHTML_WIN32_RESOURCE@
fh2xhtml_LDFLAGS = -all-static 
else
fh2xhtml_LDADD = ../../lib/libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@.la $(LIBFREEHAND_LIBS) @PPP2XHTML_WIN32_RESOURCE@ 
endif

fh2xhtml_SOURCES = \
	fh2xhtml.cpp

if OS_WIN32

@PPP2XHTML_WIN32_RESOURCE@ : fh2xhtml.rc $(fh2xhtml_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource fh2xhtml.rc @PPP2XHTML_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(fh2xhtml_SOURCES) \
	fh2xhtml.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	fh2xhtml.rc
