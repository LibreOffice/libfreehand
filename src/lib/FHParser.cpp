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

#ifdef DEBUG
const char *getTokenName(int tokenId)
{
  if(tokenId >= FH_TOKEN_COUNT)
    return 0;
  const fhtoken *currentToken = wordlist;
  while(currentToken != wordlist+sizeof(wordlist)/sizeof(*wordlist))
  {
    if(currentToken->tokenId == tokenId)
      return currentToken->name;
    ++currentToken;
  }
  return 0;
}
#endif

} // anonymous namespace

libfreehand::FHParser::FHParser(WPXInputStream *input, FHCollector *collector)
  : m_input(input), m_collector(collector), m_version(-1), m_dictionary(),
    m_records(), m_currentRecord(0)
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
  for (m_currentRecord = 0; m_currentRecord < m_records.size() && !input->atEOS(); ++m_currentRecord)
  {
    std::map<unsigned short, int>::const_iterator iterDict = m_dictionary.find(m_records[m_currentRecord]);
    if (iterDict != m_dictionary.end())
    {
      FH_DEBUG_MSG(("Parsing record number 0x%x: %s (%x)\n", (unsigned)m_currentRecord+1, getTokenName(iterDict->second), iterDict->second));
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
    else
    {
      FH_DEBUG_MSG(("FHParser::parseData NO SUCH TOKEN IN DICTIONARY\n"));
    }
  }
}

void libfreehand::FHParser::readAGDFont(WPXInputStream *input)
{
  input->seek(4, WPX_SEEK_CUR);
  unsigned short num = readU16(input);
  input->seek(2, WPX_SEEK_CUR);
  for (unsigned short i = 0; i < num; ++i)
  {
#if 0
    unsigned short key = readU16(input);
    unsigned short rec = readU16(input);
#else
    input->seek(4, WPX_SEEK_CUR);
#endif
    unsigned short something = readU16(input);
    if (0xffff == something)
      input->seek(2, WPX_SEEK_CUR);
  }
}

void libfreehand::FHParser::readAGDSelection(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readArrowPath(WPXInputStream *input)
{
  input->seek(21, WPX_SEEK_CUR);
  unsigned size = readU8(input);
  input->seek(size*27+8, WPX_SEEK_CUR);
}

void libfreehand::FHParser::readAttributeHolder(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readBasicFill(WPXInputStream *input)
{
  _readRecordId(input);
  input->seek(4, WPX_SEEK_CUR);
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

void libfreehand::FHParser::readColor6(WPXInputStream *input)
{
  input->seek(1, WPX_SEEK_CUR);
  unsigned char var = readU8(input);
  _readRecordId(input);
  input->seek(10, WPX_SEEK_CUR);
  _readRecordId(input);
  unsigned length = 12;
  if (var == 4)
    length = 16;
  else if (var == 7)
    length = 28;
  else if (var == 9)
    length = 36;
  if (m_version < 10)
    length -= 2;
  input->seek(length, WPX_SEEK_CUR);
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

void libfreehand::FHParser::readData(WPXInputStream *input)
{
  unsigned short size = readU16(input);
  input->seek(size*4+4, WPX_SEEK_CUR);
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

void libfreehand::FHParser::readElemPropLst(WPXInputStream *input)
{
  input->seek(2, WPX_SEEK_CUR);
  unsigned short size = readU16(input);
  input->seek(6, WPX_SEEK_CUR);
  for (unsigned short i = 0; i < size; ++i)
    _readRecordId(input);
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

void libfreehand::FHParser::readFileDescriptor(WPXInputStream *input)
{
  _readRecordId(input);
  _readRecordId(input);
  input->seek(5, WPX_SEEK_CUR);
  unsigned short size = readU16(input);
  input->seek(size, WPX_SEEK_CUR);
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

void libfreehand::FHParser::readGuides(WPXInputStream *input)
{
  unsigned short size = readU16(input);
  _readRecordId(input);
  _readRecordId(input);
  input->seek(16+size*8, WPX_SEEK_CUR);
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

void libfreehand::FHParser::readLinePat(WPXInputStream *input)
{
  unsigned short numStrokes = readU16(input);
  if (!numStrokes && m_version == 8)
    input->seek(26, WPX_SEEK_CUR);
  else
    input->seek(8+numStrokes*4, WPX_SEEK_CUR);
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

void libfreehand::FHParser::readMList(WPXInputStream *input)
{
  long startPosition = input->tell();
  unsigned short flag = readU16(input);
  unsigned short size = readU16(input);
  input->seek(8, WPX_SEEK_CUR);
  for (unsigned short i = 0; i < size; ++i)
    _readRecordId(input);
  if (m_version < 9 && (input->tell() - startPosition > 12 || flag))
    input->seek(startPosition+32, WPX_SEEK_SET);
}

void libfreehand::FHParser::readMName(WPXInputStream *input)
{
  long startPosition = input->tell();
  unsigned short size = readU16(input);
  unsigned short length = readU16(input);
  WPXString name;
  unsigned char character = 0;
  for (unsigned short i = 0; i < length && 0 != (character = readU8(input)); i++)
    name.append((char)character);
  FH_DEBUG_MSG(("FHParser::readMName %s\n", name.cstr()));
  input->seek(startPosition + (size+1)*4, WPX_SEEK_SET);
}

void libfreehand::FHParser::readMpObject(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMQuickDict(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readMString(WPXInputStream *input)
{
  long startPosition = input->tell();
  unsigned short size = readU16(input);
  unsigned short length = readU16(input);
  WPXString str;
  unsigned char character = 0;
  for (unsigned short i = 0; i < length && 0 != (character = readU8(input)); i++)
    str.append((char)character);
  FH_DEBUG_MSG(("FHParser::readMString %s\n", str.cstr()));
  input->seek(startPosition + (size+1)*4, WPX_SEEK_SET);
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

void libfreehand::FHParser::readSpotColor6(WPXInputStream *input)
{
  unsigned short size = readU16(input);
  _readRecordId(input);
  if (m_version < 10)
    input->seek(22, WPX_SEEK_CUR);
  else
    input->seek(24, WPX_SEEK_CUR);
  input->seek(size*4, WPX_SEEK_CUR);
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

void libfreehand::FHParser::readTabTable(WPXInputStream *input)
{
  unsigned short size = readU16(input);
  input->seek(2, WPX_SEEK_CUR);
  if (m_version < 10)
    input->seek(size*2, WPX_SEEK_CUR);
  else
    input->seek(size*6, WPX_SEEK_CUR);
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

void libfreehand::FHParser::readUString(WPXInputStream *input)
{
  long startPosition = input->tell();
  unsigned short size = readU16(input);
  unsigned short length = readU16(input);
  std::vector<unsigned short> ustr;
  unsigned short character = 0;
  for (unsigned short i = 0; i < length && 0 != (character = readU16(input)); i++)
    ustr.push_back(character);
#ifdef DEBUG
  WPXString str;
  for (std::vector<unsigned short>::const_iterator iter = ustr.begin(); iter != ustr.end(); ++iter)
    str.append((char)*iter);
  FH_DEBUG_MSG(("FHParser::readUString %s\n", str.cstr()));
#endif
  input->seek(startPosition + (size+1)*4, WPX_SEEK_SET);
}

void libfreehand::FHParser::readVDict(WPXInputStream * /* input */)
{
}

void libfreehand::FHParser::readVMpObj(WPXInputStream *input)
{
  input->seek(4, WPX_SEEK_CUR);
  unsigned short num = readU16(input);
  input->seek(2, WPX_SEEK_CUR);
  for (unsigned short i = 0; i < num; ++i)
  {
#if 0
    unsigned short key = readU16(input);
    unsigned short rec = readU16(input);
#else
    input->seek(4, WPX_SEEK_CUR);
#endif
    unsigned short something = readU16(input);
    if (0xffff == something)
      input->seek(2, WPX_SEEK_CUR);
  }
}

void libfreehand::FHParser::readXform(WPXInputStream *input)
{
  unsigned char var1 = readU8(input);
  unsigned char var2 = readU8(input);
  input->seek(_xformCalc(var1, var2), WPX_SEEK_CUR);
  var1 = readU8(input);
  var2 = readU8(input);
  input->seek(_xformCalc(var1, var2), WPX_SEEK_CUR);
}

unsigned libfreehand::FHParser::_readRecordId(WPXInputStream *input)
{
  unsigned recid = readU16(input);
  if (recid == 0xffff)
    recid = 0x1ff00 - readU16(input);
  return recid;
}

unsigned libfreehand::FHParser::_xformCalc(unsigned char var1, unsigned char var2)
{
  unsigned a5 = ~((var1&0x20)>>5);
  unsigned a4 = ~((var1&0x10)>>4);
  unsigned a2 = (var1&0x4)>>2;
  unsigned a1 = (var1&0x2)>>1;
  unsigned a0 = var1&0x1;

  unsigned b6 = (var2&0x40) >> 6;
  unsigned b5 = (var2&0x20) >> 5;
  if (a2)
    return 0;

  return (a5+a4+a1+a0+b6+b5)*4;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
