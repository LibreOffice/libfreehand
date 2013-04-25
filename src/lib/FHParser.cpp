/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sstream>
#include <string>
#include <string.h>
#include <libwpd-stream/WPXStream.h>
#include "FHParser.h"
#include "FHCollector.h"
#include "FHInternalStream.h"
#include "libfreehand_utils.h"
#include "tokens.h"

namespace
{

#include "tokenhash.h"

static int getTokenId(const char *name)
{
  const fhtoken *token = Perfect_Hash::in_word_set(name, strlen(name));
  if(token)
    return token->tokenId;
  else
    return FH_TOKEN_INVALID;
}

} // anonymous namespace

libfreehand::FHParser::FHParser(WPXInputStream *input, FHCollector *collector)
  : m_input(input), m_collector(collector), m_version(-1), m_dictionary(), m_records()
{
}

libfreehand::FHParser::~FHParser()
{
}

bool libfreehand::FHParser::parse()
{
  long dataOffset = m_input->tell();
  if ('A' != readU8(m_input))
    return false;
  if ('G' != readU8(m_input))
    return false;
  if ('D' != readU8(m_input))
    return false;
  m_version = readU8(m_input) - 0x30 + 5;

  // Skip a dword
  m_input->seek(4, WPX_SEEK_CUR);

  unsigned dataLength = readU32(m_input);
  m_input->seek(dataOffset+dataLength, WPX_SEEK_SET);

  parseDictionary(m_input);

  parseListOfRecords(m_input);

  m_input->seek(dataOffset+12, WPX_SEEK_SET);

  FHInternalStream dataStream(m_input, dataLength-12, m_version >= 9);
  parseData(&dataStream);

  return true;
}

void libfreehand::FHParser::parseDictionary(WPXInputStream *input)
{
  unsigned count = readU16(input);
  FH_DEBUG_MSG(("FHParser::parseDictionary - count 0x%x\n", count));
  input->seek(2, WPX_SEEK_CUR);
  for (unsigned i = 0; i < count; ++i)
  {
    unsigned short id = readU16(input);
    if (m_version <= 8)
      input->seek(2, WPX_SEEK_CUR);
    WPXString name;
    unsigned char tmpChar = 0;
    while (0 != (tmpChar = readU8(input)))
      name.append((char)tmpChar);
    FH_DEBUG_MSG(("FHParser::parseDictionary - ID: 0x%x, name: %s\n", id, name.cstr()));
    if (m_version <= 8)
    {
      for (unsigned f = 0; f < 2;)
      {
        if (!readU8(input))
          f++;
      }
    }
    m_dictionary[id] = getTokenId(name.cstr());
  }
}

void libfreehand::FHParser::parseListOfRecords(WPXInputStream *input)
{
  unsigned count = readU32(input);
  for (unsigned i = 0; i < count; ++i)
  {
    unsigned id = readU16(input);
    m_records.push_back(id);
    FH_DEBUG_MSG(("FHParser::parseListOfRecords - ID: 0x%x\n", id));
  }
}

void libfreehand::FHParser::parseData(WPXInputStream *input)
{
  for (std::vector<unsigned short>::const_iterator iterList = m_records.begin();
       iterList != m_records.end(); ++iterList)
  {
    std::map<unsigned short, int>::const_iterator iterDict = m_dictionary.find(*iterList);
    if (iterDict != m_dictionary.end())
    {
      switch (iterDict->second)
      {
      case FH_AGDFONT:
        readAGDFont(input);
        break;
      case FH_AGDSELECTION:
        readAGDSelection(input);
        break;
      case FH_ARROWPATH:
        readArrowPath(input);
        break;
      case FH_ATTRIBUTEHOLDER:
        readAttributeHolder(input);
        break;
      case FH_BASICFILL:
        readBasicFill(input);
        break;
      case FH_BASICLINE:
        readBasicLine(input);
        break;
      case FH_BENDFILTER:
        readBendFilter(input);
        break;
      case FH_BLOCK:
        readBlock(input);
        break;
      case FH_BRUSHLIST:
        readBrushList(input);
        break;
      case FH_BRUSH:
        readBrush(input);
        break;
      case FH_BRUSHSTROKE:
        readBrushStroke(input);
        break;
      case FH_BRUSHTIP:
        readBrushTip(input);
        break;
      case FH_CALLIGRAPHICSTROKE:
        readCalligraphicStroke(input);
        break;
      case FH_CHARACTERFILL:
        readCharacterFill(input);
        break;
      case FH_CLIPGROUP:
        readClipGroup(input);
        break;
      case FH_COLLECTOR:
        readCollector(input);
        break;
      case FH_COLOR6:
        readColor6(input);
        break;
      case FH_COMPOSITEPATH:
        readCompositePath(input);
        break;
      case FH_CONEFILL:
        readConeFill(input);
        break;
      case FH_CONNECTORLINE:
        readConnectorLine(input);
        break;
      case FH_CONTENTFILL:
        readContentFill(input);
        break;
      case FH_CONTOURFILL:
        readContourFill(input);
        break;
      case FH_CUSTOMPROC:
        readCustomProc(input);
        break;
      case FH_DATALIST:
        readDataList(input);
        break;
      case FH_DATA:
        readData(input);
        break;
      case FH_DATETIME:
        readDateTime(input);
        break;
      case FH_DUETFILTER:
        readDuetFilter(input);
        break;
      case FH_ELEMENT:
        readElement(input);
        break;
      case FH_ELEMLIST:
        readElemList(input);
        break;
      case FH_ELEMPROPLST:
        readElemPropLst(input);
        break;
      case FH_ENVELOPE:
        readEnvelope(input);
        break;
      case FH_EXPANDFILTER:
        readExpandFilter(input);
        break;
      case FH_EXTRUSION:
        readExtrusion(input);
        break;
      case FH_FHDOCHEADER:
        readFHDocHeader(input);
        break;
      case FH_FIGURE:
        readFigure(input);
        break;
      case FH_FILEDESCRIPTOR:
        readFileDescriptor(input);
        break;
      case FH_FILTERATTRIBUTEHOLDER:
        readFilterAttributeHolder(input);
        break;
      case FH_FWBEVELFILTER:
        readFWBevelFilter(input);
        break;
      case FH_FWBLURFILTER:
        readFWBlurFilter(input);
        break;
      case FH_FWFEATHERFILTER:
        readFWFeatherFilter(input);
        break;
      case FH_FWGLOWFILTER:
        readFWGlowFilter(input);
        break;
      case FH_FWSHADOWFILTER:
        readFWShadowFilter(input);
        break;
      case FH_FWSHARPENFILTER:
        readFWSharpenFilter(input);
        break;
      case FH_GRADIENTMASKFILTER:
        readGradientMaskFilter(input);
        break;
      case FH_GRAPHICSTYLE:
        readGraphicStyle(input);
        break;
      case FH_GROUP:
        readGroup(input);
        break;
      case FH_GUIDES:
        readGuides(input);
        break;
      case FH_HALFTONE:
        readHalftone(input);
        break;
      case FH_IMAGEFILL:
        readImageFill(input);
        break;
      case FH_IMAGEIMPORT:
        readImageImport(input);
        break;
      case FH_LAYER:
        readLayer(input);
        break;
      case FH_LENSFILL:
        readLensFill(input);
        break;
      case FH_LINEARFILL:
        readLinearFill(input);
        break;
      case FH_LINEPAT:
        readLinePat(input);
        break;
      case FH_LINETABLE:
        readLineTable(input);
        break;
      case FH_LIST:
        readList(input);
        break;
      case FH_MASTERPAGEDOCMAN:
        readMasterPageDocMan(input);
        break;
      case FH_MASTERPAGEELEMENT:
        readMasterPageElement(input);
        break;
      case FH_MASTERPAGELAYERELEMENT:
        readMasterPageLayerElement(input);
        break;
      case FH_MASTERPAGELAYERINSTANCE:
        readMasterPageLayerInstance(input);
        break;
      case FH_MASTERPAGESYMBOLCLASS:
        readMasterPageSymbolClass(input);
        break;
      case FH_MASTERPAGESYMBOLINSTANCE:
        readMasterPageSymbolInstance(input);
        break;
      case FH_MDICT:
        readMDict(input);
        break;
      case FH_MLIST:
        readMList(input);
        break;
      case FH_MNAME:
        readMName(input);
        break;
      case FH_MPOBJECT:
        readMpObject(input);
        break;
      case FH_MQUICKDICT:
        readMQuickDict(input);
        break;
      case FH_MSTRING:
        readMString(input);
        break;
      case FH_MULTIBLEND:
        readMultiBlend(input);
        break;
      case FH_MULTICOLORLIST:
        readMultiColorList(input);
        break;
      case FH_NEWBLEND:
        readNewBlend(input);
        break;
      case FH_NEWCONTOURFILL:
        readNewContourFill(input);
        break;
      case FH_NEWRADIALFILL:
        readNewRadialFill(input);
        break;
      case FH_OPACITYFILTER:
        readOpacityFilter(input);
        break;
      case FH_OVAL:
        readOval(input);
        break;
      case FH_PARAGRAPH:
        readParagraph(input);
        break;
      case FH_PATH:
        readPath(input);
        break;
      case FH_PATHTEXTLINEINFO:
        readPathTextLineInfo(input);
        break;
      case FH_PATTERNFILL:
        readPatternFill(input);
        break;
      case FH_PATTERNLINE:
        readPatternLine(input);
        break;
      case FH_PERSPECTIVEENVELOPE:
        readPerspectiveEnvelope(input);
        break;
      case FH_PERSPECTIVEGRID:
        readPerspectiveGrid(input);
        break;
      case FH_POLYGONFIGURE:
        readPolygonFigure(input);
        break;
      case FH_PROCEDURE:
        readProcedure(input);
        break;
      case FH_PROPLST:
        readPropLst(input);
        break;
      case FH_PSLINE:
        readPSLine(input);
        break;
      case FH_RADIALFILL:
        readRadialFill(input);
        break;
      case FH_RADIALFILLX:
        readRadialFillX(input);
        break;
      case FH_RAGGEDFILTER:
        readRaggedFilter(input);
        break;
      case FH_RECTANGLE:
        readRectangle(input);
        break;
      case FH_SKETCHFILTER:
        readSketchFilter(input);
        break;
      case FH_SPOTCOLOR6:
        readSpotColor6(input);
        break;
      case FH_STYLEPROPLST:
        readStylePropLst(input);
        break;
      case FH_SWFIMPORT:
        readSwfImport(input);
        break;
      case FH_SYMBOLCLASS:
        readSymbolClass(input);
        break;
      case FH_SYMBOLINSTANCE:
        readSymbolInstance(input);
        break;
      case FH_SYMBOLLIBRARY:
        readSymbolLibrary(input);
        break;
      case FH_TABTABLE:
        readTabTable(input);
        break;
      case FH_TAPEREDFILL:
        readTaperedFill(input);
        break;
      case FH_TAPEREDFILLX:
        readTaperedFillX(input);
        break;
      case FH_TEFFECT:
        readTEffect(input);
        break;
      case FH_TEXTBLOK:
        readTextBlok(input);
        break;
      case FH_TEXTCOLUMN:
        readTextColumn(input);
        break;
      case FH_TEXTINPATH:
        readTextInPath(input);
        break;
      case FH_TFONPATH:
        readTFOnPath(input);
        break;
      case FH_TILEFILL:
        readTileFill(input);
        break;
      case FH_TINTCOLOR6:
        readTintColor6(input);
        break;
      case FH_TRANSFORMFILTER:
        readTransformFilter(input);
        break;
      case FH_TSTRING:
        readTString(input);
        break;
      case FH_USTRING:
        readUString(input);
        break;
      case FH_VDICT:
        readVDict(input);
        break;
      case FH_VMPOBJ:
        readVMpObj(input);
        break;
      case FH_XFORM:
        readXform(input);
        break;
      default:
        FH_DEBUG_MSG(("FHParser::parseData UNKNOWN TOKEN\n"));
        return;
      }
    }
  }
}

void libfreehand::FHParser::readAGDFont(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readAGDSelection(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readArrowPath(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readAttributeHolder(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readBasicFill(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readBasicLine(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readBendFilter(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readBlock(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readBrushList(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readBrush(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readBrushStroke(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readBrushTip(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readCalligraphicStroke(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readCharacterFill(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readClipGroup(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readCollector(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readColor6(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readCompositePath(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readConeFill(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readConnectorLine(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readContentFill(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readContourFill(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readCustomProc(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readDataList(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readData(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readDateTime(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readDuetFilter(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readElement(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readElemList(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readElemPropLst(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readEnvelope(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readExpandFilter(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readExtrusion(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readFHDocHeader(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readFigure(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readFileDescriptor(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readFilterAttributeHolder(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readFWBevelFilter(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readFWBlurFilter(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readFWFeatherFilter(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readFWGlowFilter(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readFWShadowFilter(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readFWSharpenFilter(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readGradientMaskFilter(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readGraphicStyle(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readGroup(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readGuides(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readHalftone(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readImageFill(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readImageImport(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readLayer(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readLensFill(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readLinearFill(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readLinePat(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readLineTable(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readList(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMasterPageDocMan(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMasterPageElement(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMasterPageLayerElement(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMasterPageLayerInstance(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMasterPageSymbolClass(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMasterPageSymbolInstance(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMDict(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMList(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMName(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMpObject(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMQuickDict(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMString(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMultiBlend(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMultiColorList(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readNewBlend(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readNewContourFill(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readNewRadialFill(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readOpacityFilter(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readOval(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readParagraph(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readPath(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readPathTextLineInfo(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readPatternFill(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readPatternLine(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readPerspectiveEnvelope(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readPerspectiveGrid(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readPolygonFigure(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readProcedure(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readPropLst(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readPSLine(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readRadialFill(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readRadialFillX(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readRaggedFilter(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readRectangle(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readSketchFilter(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readSpotColor6(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readStylePropLst(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readSwfImport(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readSymbolClass(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readSymbolInstance(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readSymbolLibrary(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readTabTable(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readTaperedFill(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readTaperedFillX(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readTEffect(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readTextBlok(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readTextColumn(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readTextInPath(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readTFOnPath(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readTileFill(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readTintColor6(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readTransformFilter(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readTString(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readUString(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readVDict(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readVMpObj(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readXform(WPXInputStream * /* input */)
{
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
