noinst_PROGRAMS = fhfuzzer

AM_CXXFLAGS = -I$(top_srcdir)/inc \
	$(REVENGE_GENERATORS_CFLAGS) \
	$(REVENGE_CFLAGS) \
	$(REVENGE_STREAM_CFLAGS) \
	$(DEBUG_CXXFLAGS)

fhfuzzer_LDADD = \
	$(top_builddir)/src/lib/libfreehand-@FH_MAJOR_VERSION@.@FH_MINOR_VERSION@.la \
	$(REVENGE_GENERATORS_LIBS) \
	$(REVENGE_LIBS) \
	$(REVENGE_STREAM_LIBS) \
	-lFuzzingEngine

fhfuzzer_SOURCES = \
	fhfuzzer.cpp
