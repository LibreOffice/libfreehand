if PLATFORM_WIN32
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -avoid-version
else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
endif

noinst_LTLIBRARIES = libfreehand-internal.la
lib_LTLIBRARIES = libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@.la

AM_CXXFLAGS = \
	-DLIBFREEHAND_BUILD=1 \
	-I$(top_srcdir)/inc \
	$(REVENGE_CFLAGS) \
	$(ZLIB_CFLAGS) \
	$(ICU_CFLAGS) \
	$(LCMS2_CFLAGS) \
	$(DEBUG_CXXFLAGS)

BUILT_SOURCES = tokens.h tokenhash.h

libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_LIBADD = \
	libfreehand-internal.la \
	$(REVENGE_LIBS) \
	$(ZLIB_LIBS) \
	$(LCMS2_LIBS) \
	@LIBFREEHAND_WIN32_RESOURCE@

libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_DEPENDENCIES = libfreehand-internal.la @LIBFREEHAND_WIN32_RESOURCE@
libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic -no-undefined
libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_SOURCES = \
	FreeHandDocument.cpp

libfreehand_internal_la_SOURCES = \
	FHCollector.cpp \
	FHInternalStream.cpp \
	FHParser.cpp \
	FHPath.cpp \
	FHTransform.cpp \
	libfreehand_utils.cpp \
	FHCollector.h \
	FHColorProfiles.h \
	FHConstants.h \
	FHInternalStream.h \
	FHParser.h \
	FHPath.h \
	FHTransform.h \
	FHTypes.h \
	libfreehand_utils.h \
	$(generated_files)

tokens.h : tokens.gperf

tokenhash.h : tokens.gperf
	$(GPERF) --compare-strncmp -C -m 20 tokens.gperf \
		| $(SED) -e 's/(char\*)0/(char\*)0, 0/g' -e 's/register //g' > tokenhash.h

tokens.gperf : $(top_srcdir)/src/lib/tokens.txt $(top_srcdir)/src/lib/gentoken.pl
	$(PERL) $(top_srcdir)/src/lib/gentoken.pl $(top_srcdir)/src/lib/tokens.txt \
		tokens.h tokens.gperf

if OS_WIN32

@LIBFREEHAND_WIN32_RESOURCE@ : libfreehand.rc $(libfreehand_@FH_MAJOR_VERSION@_@FH_MINOR_VERSION@_la_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource libfreehand.rc @LIBFREEHAND_WIN32_RESOURCE@

endif

MOSTLYCLEANFILES = \
	$(BUILT_SOURCES) \
	tokens.gperf

EXTRA_DIST = \
	$(BUILT_SOURCES) \
	tokens.txt \
	gentoken.pl \
	libfreehand.rc \
	libfreehand.rc.in
