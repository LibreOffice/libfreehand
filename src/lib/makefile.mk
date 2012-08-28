EXTERNAL_WARNINGS_NOT_ERRORS := TRUE

PRJ=..$/..$/..$/..$/..$/..

PRJNAME=libfreehand
TARGET=freehandlib
ENABLE_EXCEPTIONS=TRUE
LIBTARGET=NO

.INCLUDE :  settings.mk

.IF "$(GUI)$(COM)"=="WNTMSC"
CFLAGS+=-GR
.ENDIF
.IF "$(COM)"=="GCC"
CFLAGSCXX+=-frtti
.ENDIF

.IF "$(SYSTEM_LIBWPD)" == "YES"
INCPRE+=$(WPD_CFLAGS) -I..
.ELSE
INCPRE+=$(SOLARVER)$/$(INPATH)$/inc$/libwpd
.ENDIF

.IF "$(SYSTEM_LIBWPG)" == "YES"
INCPRE+=$(WPG_CFLAGS) -I..
.ELSE
INCPRE+=$(SOLARVER)$/$(INPATH)$/inc$/libwpg
.ENDIF

SLOFILES= \
        $(SLO)$/FHCollector.obj \
        $(SLO)$/FreeHandDocument.obj \
		$(SLO)$/FHParser.obj \
		$(SLO)$/FHStringVector.obj \
		$(SLO)$/FHSVGGenerator.obj \
		$(SLO)$/libfreehand_utils.obj

LIB1ARCHIV=$(LB)$/libfreehandlib.a
LIB1TARGET=$(SLB)$/$(TARGET).lib
LIB1OBJFILES= $(SLOFILES)

.INCLUDE :  target.mk
