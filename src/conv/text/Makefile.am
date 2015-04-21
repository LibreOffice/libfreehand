if BUILD_TOOLS

bin_PROGRAMS = fh2text

AM_CXXFLAGS = \
	-I$(top_srcdir)/inc \
	$(LIBFREEHAND_CXXFLAGS) \
	$(REVENGE_STREAM_CFLAGS) \
	$(REVENGE_GENERATORS_CFLAGS) \
	$(DEBUG_CXXFLAGS)

fh2text_DEPENDENCIES = @FH2TEXT_WIN32_RESOURCE@

fh2text_LDADD = \
	../../lib/libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@.la \
	$(REVENGE_GENERATORS_LIBS) \
	$(LIBFREEHAND_LIBS) \
	$(REVENGE_STREAM_LIBS) \
	@FH2TEXT_WIN32_RESOURCE@

fh2text_SOURCES = \
	fh2text.cpp

if OS_WIN32

@FH2TEXT_WIN32_RESOURCE@ : fh2text.rc $(fh2text_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource fh2text.rc @FH2TEXT_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(fh2text_SOURCES) \
	fh2text.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	fh2text.rc	

endif
