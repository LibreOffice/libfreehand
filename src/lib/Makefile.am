if PLATFORM_WIN32
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -avoid-version
else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
endif

lib_LTLIBRARIES = libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@.la
libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_includedir = $(includedir)/libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@/libfreehand
libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_include_HEADERS = \
	$(top_srcdir)/inc/libfreehand/libfreehand.h \
	$(top_srcdir)/inc/libfreehand/FreeHandDocument.h \
	$(top_srcdir)/inc/libfreehand/FHStringVector.h

AM_CXXFLAGS = -I$(top_srcdir)/inc $(WPD_CFLAGS) $(WPG_CFLAGS) $(ZLIB_CFLAGS) $(DEBUG_CXXFLAGS)

generated_files = \
	$(top_builddir)/src/lib/tokens.h \
	$(top_builddir)/src/lib/tokenhash.h

libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_LIBADD  = $(WPD_LIBS) $(ZLIB_LIBS) @LIBFREEHAND_WIN32_RESOURCE@
libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_DEPENDENCIES = @LIBFREEHAND_WIN32_RESOURCE@
libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic -no-undefined
libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_SOURCES = \
	FreeHandDocument.cpp \
	FHCollector.cpp \
	FHInternalStream.cpp \
	FHParser.cpp \
	FHPath.cpp \
	FHStringVector.cpp \
	FHSVGGenerator.cpp \
	FHTransform.cpp \
	libfreehand_utils.cpp \
	FHCollector.h \
	FHInternalStream.h \
	FHParser.h \
	FHPath.h \
	FHSVGGenerator.h \
	FHTransform.h \
	FHTypes.h \
	libfreehand_utils.h \
	$(generated_files)

FHParser.lo : $(generated_files)

$(top_builddir)/src/lib/tokens.h : $(top_builddir)/src/lib/tokens.gperf

$(top_builddir)/src/lib/tokenhash.h : $(top_builddir)/src/lib/tokens.gperf
	$(GPERF) --compare-strncmp -C -m 20 $(top_builddir)/src/lib/tokens.gperf \
		| $(SED) -e "s/(char\*)0/(char\*)0, 0/g" > $(top_builddir)/src/lib/tokenhash.h

$(top_builddir)/src/lib/tokens.gperf : $(top_srcdir)/src/lib/tokens.txt gentoken.pl
	perl $(top_srcdir)/src/lib/gentoken.pl $(top_srcdir)/src/lib/tokens.txt \
		$(top_builddir)/src/lib/tokens.h $(top_builddir)/src/lib/tokens.gperf

if OS_WIN32

@LIBFREEHAND_WIN32_RESOURCE@ : libfreehand.rc $(libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource libfreehand.rc @LIBFREEHAND_WIN32_RESOURCE@

endif

distclean-local :
	rm -f $(generated_files) $(top_builddir)/src/lib/*.gperf

EXTRA_DIST = \
	tokens.txt \
    gentoken.pl \
	libfreehand.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	libfreehand.rc
