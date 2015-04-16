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
#include <unicode/utf8.h>
#include <unicode/utf16.h>
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
  if (token)
    return token->tokenId;
  else
    return FH_TOKEN_INVALID;
}

static void _appendUTF16(librevenge::RVNGString &text, std::vector<unsigned short> characters)
{
  if (characters.empty())
    return;

  unsigned short *s = &characters[0];
  int j = 0;
  int length = characters.size();
  UChar32 c;

  while (j < length)
  {
    U16_NEXT(s, j, length, c)
    unsigned char outbuf[U8_MAX_LENGTH+1];
    int i = 0;
    U8_APPEND_UNSAFE(&outbuf[0], i, c);
    outbuf[i] = 0;

    text.append((char *)outbuf);
  }
}


#ifdef DEBUG
const char *getTokenName(int tokenId)
{
  if (tokenId >= FH_TOKEN_COUNT)
    return 0;
  const fhtoken *currentToken = wordlist;
  while (currentToken != wordlist+sizeof(wordlist)/sizeof(*wordlist))
  {
    if (currentToken->tokenId == tokenId)
      return currentToken->name;
    ++currentToken;
  }
  return 0;
}

#endif

} // anonymous namespace

libfreehand::FHParser::FHParser()
  : m_input(0), m_collector(0), m_version(-1), m_dictionary(),
    m_records(), m_currentRecord(0), m_pageInfo()
{
}

libfreehand::FHParser::~FHParser()
{
}

bool libfreehand::FHParser::parse(librevenge::RVNGInputStream *input, librevenge::RVNGDrawingInterface *painter)
{
  long dataOffset = input->tell();
  if ('A' != readU8(input))
    return false;
  if ('G' != readU8(input))
    return false;
  if ('D' != readU8(input))
    return false;
  m_version = readU8(input) - 0x30 + 5;

  // Skip a dword
  input->seek(4, librevenge::RVNG_SEEK_CUR);

  unsigned dataLength = readU32(input);
  input->seek(dataOffset+dataLength, librevenge::RVNG_SEEK_SET);

  parseDictionary(input);

  parseRecordList(input);

  input->seek(dataOffset+12, librevenge::RVNG_SEEK_SET);

  FHInternalStream dataStream(input, dataLength-12, m_version >= 9);
  dataStream.seek(0, librevenge::RVNG_SEEK_SET);
  FHCollector contentCollector;
  parseDocument(&dataStream, &contentCollector);
  contentCollector.outputContent(painter);

  return true;
}

void libfreehand::FHParser::parseDictionary(librevenge::RVNGInputStream *input)
{
  unsigned count = readU16(input);
  FH_DEBUG_MSG(("FHParser::parseDictionary - count 0x%x\n", count));
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  for (unsigned i = 0; i < count; ++i)
  {
    unsigned short id = readU16(input);
    if (m_version <= 8)
      input->seek(2, librevenge::RVNG_SEEK_CUR);
    librevenge::RVNGString name;
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

void libfreehand::FHParser::parseRecordList(librevenge::RVNGInputStream *input)
{
  unsigned count = readU32(input);
  for (unsigned i = 0; i < count; ++i)
  {
    unsigned id = readU16(input);
    m_records.push_back(id);
    FH_DEBUG_MSG(("FHParser::parseRecordList - ID: 0x%x\n", id));
  }
}

void libfreehand::FHParser::parseRecord(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector, int recordId)
{
  FH_DEBUG_MSG(("Parsing record number 0x%x: %s Offset 0x%lx\n", (unsigned)m_currentRecord+1, getTokenName(recordId), input->tell()));
  switch (recordId)
  {
  case FH_AGDFONT:
    readAGDFont(input, collector);
    break;
  case FH_AGDSELECTION:
    readAGDSelection(input, collector);
    break;
  case FH_ARROWPATH:
    readArrowPath(input, collector);
    break;
  case FH_ATTRIBUTEHOLDER:
    readAttributeHolder(input, collector);
    break;
  case FH_BASICFILL:
    readBasicFill(input, collector);
    break;
  case FH_BASICLINE:
    readBasicLine(input, collector);
    break;
  case FH_BENDFILTER:
    readBendFilter(input, collector);
    break;
  case FH_BLOCK:
    readBlock(input, collector);
    break;
  case FH_BRUSHLIST:
    readList(input, collector);
    break;
  case FH_BRUSH:
    readBrush(input, collector);
    break;
  case FH_BRUSHSTROKE:
    readBrushStroke(input, collector);
    break;
  case FH_BRUSHTIP:
    readBrushTip(input, collector);
    break;
  case FH_CALLIGRAPHICSTROKE:
    readCalligraphicStroke(input, collector);
    break;
  case FH_CHARACTERFILL:
    readCharacterFill(input, collector);
    break;
  case FH_CLIPGROUP:
    readClipGroup(input, collector);
    break;
  case FH_COLLECTOR:
    readCollector(input, collector);
    break;
  case FH_COLOR6:
    readColor6(input, collector);
    break;
  case FH_COMPOSITEPATH:
    readCompositePath(input, collector);
    break;
  case FH_CONEFILL:
    readConeFill(input, collector);
    break;
  case FH_CONNECTORLINE:
    readConnectorLine(input, collector);
    break;
  case FH_CONTENTFILL:
    readContentFill(input, collector);
    break;
  case FH_CONTOURFILL:
    readContourFill(input, collector);
    break;
  case FH_CUSTOMPROC:
    readCustomProc(input, collector);
    break;
  case FH_DATALIST:
    readDataList(input, collector);
    break;
  case FH_DATA:
    readData(input, collector);
    break;
  case FH_DATETIME:
    readDateTime(input, collector);
    break;
  case FH_DUETFILTER:
    readDuetFilter(input, collector);
    break;
  case FH_ELEMENT:
    readElement(input, collector);
    break;
  case FH_ELEMLIST:
    readElemList(input, collector);
    break;
  case FH_ELEMPROPLST:
    readElemPropLst(input, collector);
    break;
  case FH_ENVELOPE:
    readEnvelope(input, collector);
    break;
  case FH_EXPANDFILTER:
    readExpandFilter(input, collector);
    break;
  case FH_EXTRUSION:
    readExtrusion(input, collector);
    break;
  case FH_FHDOCHEADER:
    readFHDocHeader(input, collector);
    break;
  case FH_FIGURE:
    readFigure(input, collector);
    break;
  case FH_FILEDESCRIPTOR:
    readFileDescriptor(input, collector);
    break;
  case FH_FILTERATTRIBUTEHOLDER:
    readFilterAttributeHolder(input, collector);
    break;
  case FH_FWBEVELFILTER:
    readFWBevelFilter(input, collector);
    break;
  case FH_FWBLURFILTER:
    readFWBlurFilter(input, collector);
    break;
  case FH_FWFEATHERFILTER:
    readFWFeatherFilter(input, collector);
    break;
  case FH_FWGLOWFILTER:
    readFWGlowFilter(input, collector);
    break;
  case FH_FWSHADOWFILTER:
    readFWShadowFilter(input, collector);
    break;
  case FH_FWSHARPENFILTER:
    readFWSharpenFilter(input, collector);
    break;
  case FH_GRADIENTMASKFILTER:
    readGradientMaskFilter(input, collector);
    break;
  case FH_GRAPHICSTYLE:
    readGraphicStyle(input, collector);
    break;
  case FH_GROUP:
    readGroup(input, collector);
    break;
  case FH_GUIDES:
    readGuides(input, collector);
    break;
  case FH_HALFTONE:
    readHalftone(input, collector);
    break;
  case FH_IMAGEFILL:
    readImageFill(input, collector);
    break;
  case FH_IMAGEIMPORT:
    readImageImport(input, collector);
    break;
  case FH_LAYER:
    readLayer(input, collector);
    break;
  case FH_LENSFILL:
    readLensFill(input, collector);
    break;
  case FH_LINEARFILL:
    readLinearFill(input, collector);
    break;
  case FH_LINEPAT:
    readLinePat(input, collector);
    break;
  case FH_LINETABLE:
    readLineTable(input, collector);
    break;
  case FH_LIST:
    readList(input, collector);
    break;
  case FH_MASTERPAGEDOCMAN:
    readMasterPageDocMan(input, collector);
    break;
  case FH_MASTERPAGEELEMENT:
    readMasterPageElement(input, collector);
    break;
  case FH_MASTERPAGELAYERELEMENT:
    readMasterPageLayerElement(input, collector);
    break;
  case FH_MASTERPAGELAYERINSTANCE:
    readMasterPageLayerInstance(input, collector);
    break;
  case FH_MASTERPAGESYMBOLCLASS:
    readMasterPageSymbolClass(input, collector);
    break;
  case FH_MASTERPAGESYMBOLINSTANCE:
    readMasterPageSymbolInstance(input, collector);
    break;
  case FH_MDICT:
    readMDict(input, collector);
    break;
  case FH_MLIST:
    readList(input, collector);
    break;
  case FH_MNAME:
    readMName(input, collector);
    break;
  case FH_MPOBJECT:
    readMpObject(input, collector);
    break;
  case FH_MQUICKDICT:
    readMQuickDict(input, collector);
    break;
  case FH_MSTRING:
    readMString(input, collector);
    break;
  case FH_MULTIBLEND:
    readMultiBlend(input, collector);
    break;
  case FH_MULTICOLORLIST:
    readMultiColorList(input, collector);
    break;
  case FH_NEWBLEND:
    readNewBlend(input, collector);
    break;
  case FH_NEWCONTOURFILL:
    readNewContourFill(input, collector);
    break;
  case FH_NEWRADIALFILL:
    readNewRadialFill(input, collector);
    break;
  case FH_OPACITYFILTER:
    readOpacityFilter(input, collector);
    break;
  case FH_OVAL:
    readOval(input, collector);
    break;
  case FH_PARAGRAPH:
    readParagraph(input, collector);
    break;
  case FH_PATH:
    readPath(input, collector);
    break;
  case FH_PATHTEXTLINEINFO:
    readPathTextLineInfo(input, collector);
    break;
  case FH_PATTERNFILL:
    readPatternFill(input, collector);
    break;
  case FH_PATTERNLINE:
    readPatternLine(input, collector);
    break;
  case FH_PERSPECTIVEENVELOPE:
    readPerspectiveEnvelope(input, collector);
    break;
  case FH_PERSPECTIVEGRID:
    readPerspectiveGrid(input, collector);
    break;
  case FH_POLYGONFIGURE:
    readPolygonFigure(input, collector);
    break;
  case FH_PROCEDURE:
    readProcedure(input, collector);
    break;
  case FH_PROCESSCOLOR:
    readProcessColor(input, collector);
    break;
  case FH_PROPLST:
    readPropLst(input, collector);
    break;
  case FH_PSLINE:
    readPSLine(input, collector);
    break;
  case FH_RADIALFILL:
    readRadialFill(input, collector);
    break;
  case FH_RADIALFILLX:
    readRadialFillX(input, collector);
    break;
  case FH_RAGGEDFILTER:
    readRaggedFilter(input, collector);
    break;
  case FH_RECTANGLE:
    readRectangle(input, collector);
    break;
  case FH_SKETCHFILTER:
    readSketchFilter(input, collector);
    break;
  case FH_SPOTCOLOR:
    readSpotColor(input, collector);
    break;
  case FH_SPOTCOLOR6:
    readSpotColor6(input, collector);
    break;
  case FH_STYLEPROPLST:
    readStylePropLst(input, collector);
    break;
  case FH_SWFIMPORT:
    readSwfImport(input, collector);
    break;
  case FH_SYMBOLCLASS:
    readSymbolClass(input, collector);
    break;
  case FH_SYMBOLINSTANCE:
    readSymbolInstance(input, collector);
    break;
  case FH_SYMBOLLIBRARY:
    readSymbolLibrary(input, collector);
    break;
  case FH_TABTABLE:
    readTabTable(input, collector);
    break;
  case FH_TAPEREDFILL:
    readTaperedFill(input, collector);
    break;
  case FH_TAPEREDFILLX:
    readTaperedFillX(input, collector);
    break;
  case FH_TEFFECT:
    readTEffect(input, collector);
    break;
  case FH_TEXTBLOK:
    readTextBlok(input, collector);
    break;
  case FH_TEXTCOLUMN:
    readTextColumn(input, collector);
    break;
  case FH_TEXTINPATH:
    readTextInPath(input, collector);
    break;
  case FH_TFONPATH:
    readTFOnPath(input, collector);
    break;
  case FH_TILEFILL:
    readTileFill(input, collector);
    break;
  case FH_TINTCOLOR:
    readTintColor(input, collector);
    break;
  case FH_TINTCOLOR6:
    readTintColor6(input, collector);
    break;
  case FH_TRANSFORMFILTER:
    readTransformFilter(input, collector);
    break;
  case FH_TSTRING:
    readTString(input, collector);
    break;
  case FH_USTRING:
    readUString(input, collector);
    break;
  case FH_VDICT:
    readVDict(input, collector);
    break;
  case FH_VMPOBJ:
    readVMpObj(input, collector);
    break;
  case FH_XFORM:
    readXform(input, collector);
    break;
  default:
    FH_DEBUG_MSG(("FHParser::parseRecords UNKNOWN TOKEN\n"));
    return;
  }

}

void libfreehand::FHParser::parseRecords(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  for (m_currentRecord = 0; m_currentRecord < m_records.size() && !input->isEnd(); ++m_currentRecord)
  {
    std::map<unsigned short, int>::const_iterator iterDict = m_dictionary.find(m_records[m_currentRecord]);
    if (iterDict != m_dictionary.end())
    {
      parseRecord(input, collector, iterDict->second);
    }
    else
    {
      FH_DEBUG_MSG(("FHParser::parseRecords NO SUCH TOKEN IN DICTIONARY\n"));
    }
  }
  readFHTail(input, collector);
}

void libfreehand::FHParser::parseDocument(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  parseRecords(input, collector);
  collector->collectPageInfo(m_pageInfo);
}

void libfreehand::FHParser::readAGDFont(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  unsigned short num = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  for (unsigned short i = 0; i < num; ++i)
  {
    unsigned short key = readU16(input);
    input->seek(2, librevenge::RVNG_SEEK_CUR);
    if (key == 2)
      _readRecordId(input);
    else
      input->seek(4, librevenge::RVNG_SEEK_CUR);
  }
}

void libfreehand::FHParser::readAGDSelection(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short size = readU16(input);
  input->seek(6+size*4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readArrowPath(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  if (m_version > 8)
    input->seek(20, librevenge::RVNG_SEEK_CUR);
  unsigned size = readU16(input);
  if (m_version <= 8)
    input->seek(20, librevenge::RVNG_SEEK_CUR);
  input->seek(size*27+8, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readAttributeHolder(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
}

void libfreehand::FHParser::readBasicFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readBasicLine(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  input->seek(18, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readBendFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(10, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readBlock(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned layerListId = 0;
  unsigned defaultLayerId = 0;
  if (m_version == 10)
  {
    readU16(input);
    for (unsigned i = 1; i < 22; ++i)
      _readBlockInformation(input, i, layerListId, defaultLayerId);
    input->seek(1, librevenge::RVNG_SEEK_CUR);
    _readRecordId(input);
    _readRecordId(input);
  }
  else if (m_version == 8)
  {
    for (unsigned i = 0; i < 12; ++i)
      _readBlockInformation(input, i, layerListId, defaultLayerId);
    input->seek(14, librevenge::RVNG_SEEK_CUR);
  }
  else if (m_version < 8)
  {
    for (unsigned i = 0; i < 11; ++i)
      _readBlockInformation(input, i, layerListId, defaultLayerId);
    input->seek(10, librevenge::RVNG_SEEK_CUR);
    _readRecordId(input);
    _readRecordId(input);
    _readRecordId(input);
  }
  else
  {
    for (unsigned i = 0; i < 12; ++i)
      _readBlockInformation(input, i, layerListId, defaultLayerId);
    input->seek(14, librevenge::RVNG_SEEK_CUR);
    for (unsigned j = 3; j; --j)
      _readRecordId(input);
    input->seek(1, librevenge::RVNG_SEEK_CUR);
    for (unsigned k = 4; k; --k)
      _readRecordId(input);
    if (m_version < 10)
      input->seek(-6, librevenge::RVNG_SEEK_CUR);
  }
  FH_DEBUG_MSG(("Parsing Block: layerListId 0x%x, defaultLayerId 0x%x\n", layerListId, defaultLayerId));
  if (collector)
  {
    FHBlock block(layerListId, defaultLayerId);
    collector->collectBlock(m_currentRecord+1, block);
  }
}

void libfreehand::FHParser::readBrush(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
}

void libfreehand::FHParser::readBrushStroke(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);
}

void libfreehand::FHParser::readBrushTip(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  input->seek(60, librevenge::RVNG_SEEK_CUR);
  if (m_version == 11)
    input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readCalligraphicStroke(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  input->seek(12, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
}

void libfreehand::FHParser::readCharacterFill(librevenge::RVNGInputStream * /* input */, libfreehand::FHCollector * /* collector */)
{
}

void libfreehand::FHParser::readClipGroup(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readCollector(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readColor6(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(1, librevenge::RVNG_SEEK_CUR);
  unsigned char var = readU8(input);
  _readRecordId(input);
  input->seek(10, librevenge::RVNG_SEEK_CUR);
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
  input->seek(length, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readCompositePath(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readConeFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  input->seek(16, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  input->seek(14, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readConnectorLine(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(20, librevenge::RVNG_SEEK_CUR);
  unsigned short num = readU16(input);
  input->seek(46+num*27, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readContentFill(librevenge::RVNGInputStream * /* input */, libfreehand::FHCollector * /* collector */)
{
}

void libfreehand::FHParser::readContourFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  if (m_version == 10)
    input->seek(24, librevenge::RVNG_SEEK_CUR);
  else
  {
    unsigned short num = readU16(input);
    unsigned short size = readU16(input);
    while (num)
    {
      input->seek(6+size*2, librevenge::RVNG_SEEK_CUR);
      num = readU16(input);
      size = readU16(input);
    }
    input->seek(6+size*2, librevenge::RVNG_SEEK_CUR);
  }
}

void libfreehand::FHParser::readCustomProc(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short size = readU16(input);
  _readRecordId(input);
  input->seek(4+10*size, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readDataList(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short size = readU16(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  for (unsigned short i = 0; i < size; ++i)
    _readRecordId(input);
}

void libfreehand::FHParser::readData(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short size = readU16(input);
  input->seek(size*4+4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readDateTime(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(14, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readDuetFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(14, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readElement(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readElemList(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readElemPropLst(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  if (m_version > 8)
    input->seek(2, librevenge::RVNG_SEEK_CUR);
  unsigned short size = readU16(input);
  if (m_version <= 8)
    input->seek(2, librevenge::RVNG_SEEK_CUR);
  input->seek(6, librevenge::RVNG_SEEK_CUR);
  for (unsigned short i = 0; i < size*2; ++i)
    _readRecordId(input);
}

void libfreehand::FHParser::readEnvelope(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  _readRecordId(input);
  input->seek(14, librevenge::RVNG_SEEK_CUR);
  unsigned short num = readU16(input);
  _readRecordId(input);
  input->seek(19, librevenge::RVNG_SEEK_CUR);
  unsigned short num2 = readU16(input);
  input->seek(4*num2+27*num, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readExpandFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(14, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readExtrusion(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  long startPosition = input->tell();
  input->seek(96, librevenge::RVNG_SEEK_CUR);
  unsigned char var1 = readU8(input);
  unsigned char var2 = readU8(input);
  input->seek(startPosition, librevenge::RVNG_SEEK_SET);
  _readRecordId(input);
  _readRecordId(input);
  input->seek(92 + _xformCalc(var1, var2) + 2, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readFHDocHeader(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readFHTail(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned blockId = _readRecordId(input);
  unsigned propLstId = _readRecordId(input);
  unsigned fontId = _readRecordId(input);
  if (collector)
    collector->collectFHTail(m_currentRecord+1, blockId, propLstId, fontId);
}

void libfreehand::FHParser::readFigure(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readFileDescriptor(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  input->seek(5, librevenge::RVNG_SEEK_CUR);
  unsigned short size = readU16(input);
  input->seek(size, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readFilterAttributeHolder(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  _readRecordId(input);
}

void libfreehand::FHParser::readFWBevelFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  input->seek(28, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readFWBlurFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(12, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readFWFeatherFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(8, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readFWGlowFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  input->seek(20, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readFWShadowFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  input->seek(20, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readFWSharpenFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(16, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readGradientMaskFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
}

void libfreehand::FHParser::readGraphicStyle(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  unsigned short size = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  /* unsigned short parent = */
  readU16(input);
  /* unsigned short attrid = */
  readU16(input);
  for (unsigned i = 0; i<size; ++i)
    input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readGroup(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  _readRecordId(input);
}

void libfreehand::FHParser::readGuides(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short size = readU16(input);
  _readRecordId(input);
  _readRecordId(input);
  input->seek(16+size*8, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readHalftone(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readImageFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(6, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readImageImport(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  unsigned recid = _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);

  if (m_version > 8)
    input->seek(37, librevenge::RVNG_SEEK_CUR);
  else if (m_version == 9)
    input->seek(35, librevenge::RVNG_SEEK_CUR);
  else if (m_version < 9)
    input->seek(32, librevenge::RVNG_SEEK_CUR);
  if (recid > 0)
    input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readLayer(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHLayer layer;
  layer.m_graphicStyleId = _readRecordId(input);
  input->seek(10, librevenge::RVNG_SEEK_CUR);
  layer.m_elementsId = _readRecordId(input);
  _readRecordId(input);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectLayer(m_currentRecord+1, layer);
}

void libfreehand::FHParser::readLensFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  input->seek(38, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readLinearFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  input->seek(12, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  input->seek(16, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readLinePat(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short numStrokes = readU16(input);
  if (!numStrokes && m_version == 8)
    input->seek(26, librevenge::RVNG_SEEK_CUR);
  else
    input->seek(8+numStrokes*4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readLineTable(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short tmpSize = readU16(input);
  unsigned short size = readU16(input);
  if (m_version < 10)
    size = tmpSize;
  for (unsigned short i = 0; i < size; ++i)
  {
    input->seek(48, librevenge::RVNG_SEEK_CUR);
    _readRecordId(input);
  }
}

void libfreehand::FHParser::readMasterPageDocMan(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readMasterPageElement(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(14, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readMasterPageLayerElement(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(14, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readMasterPageLayerInstance(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(14, librevenge::RVNG_SEEK_CUR);
  unsigned char var1 = readU8(input);
  unsigned char var2 = readU8(input);
  input->seek(_xformCalc(var1, var2) + 2, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readMasterPageSymbolClass(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(12, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readMasterPageSymbolInstance(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(14, librevenge::RVNG_SEEK_CUR);
  unsigned char var1 = readU8(input);
  unsigned char var2 = readU8(input);
  input->seek(_xformCalc(var1, var2) + 2, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readMDict(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  unsigned short size = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  for (unsigned short i = 0; i < size; ++i)
  {
    _readRecordId(input);
    _readRecordId(input);
  }
}

void libfreehand::FHParser::readList(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned short size2 = readU16(input);
  unsigned short size = readU16(input);
  input->seek(6, librevenge::RVNG_SEEK_CUR);
  FHList lst;
  lst.m_listType = readU16(input);
  for (unsigned short i = 0; i < size; ++i)
    lst.m_elements.push_back(_readRecordId(input));
  if (m_version < 9)
    input->seek(2*(size2-size),librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectList(m_currentRecord+1, lst);
}

void libfreehand::FHParser::readMName(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  long startPosition = input->tell();
  unsigned short size = readU16(input);
  unsigned short length = readU16(input);
  librevenge::RVNGString name;
  unsigned char character = 0;
  for (unsigned short i = 0; i < length && 0 != (character = readU8(input)); i++)
    name.append((char)character);
  FH_DEBUG_MSG(("FHParser::readMName %s\n", name.cstr()));
  input->seek(startPosition + (size+1)*4, librevenge::RVNG_SEEK_SET);
  if (collector)
    collector->collectMName(m_currentRecord+1, name);
}

void libfreehand::FHParser::readMpObject(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readMQuickDict(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short size = readU16(input);
  input->seek(5+size*4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readMString(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  long startPosition = input->tell();
  unsigned short size = readU16(input);
  unsigned short length = readU16(input);
  librevenge::RVNGString str;
  unsigned char character = 0;
  for (unsigned short i = 0; i < length && 0 != (character = readU8(input)); i++)
    str.append((char)character);
  FH_DEBUG_MSG(("FHParser::readMString %s\n", str.cstr()));
  input->seek(startPosition + (size+1)*4, librevenge::RVNG_SEEK_SET);
}

void libfreehand::FHParser::readMultiBlend(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short size = readU16(input);
  _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);
  input->seek(32 + size*6, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readMultiColorList(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short num = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  for (unsigned short i = 0; i < num; ++i)
  {
    input->seek(8, librevenge::RVNG_SEEK_CUR);
    _readRecordId(input);
  }
}

void libfreehand::FHParser::readNewBlend(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);
  input->seek(26, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readNewContourFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  input->seek(14, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  _readRecordId(input);
  input->seek(14, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readNewRadialFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  input->seek(16, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  input->seek(23, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readOpacityFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readOval(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned short graphicStyle = _readRecordId(input);
  unsigned short layer = _readRecordId(input);
  input->seek(12, librevenge::RVNG_SEEK_CUR);
  unsigned short xform = _readRecordId(input);
  double xa = _readCoordinate(input) / 72.0;
  double ya = _readCoordinate(input) / 72.0;
  double xb = _readCoordinate(input) / 72.0;
  double yb = _readCoordinate(input) / 72.0;
  double arc1 = 0.0;
  double arc2 = 0.0;
  bool closed = false;
  if (m_version > 10)
  {
    arc1 = _readCoordinate(input) * M_PI / 180.0;
    arc2 = _readCoordinate(input) * M_PI / 180.0;
    closed = bool(readU8(input));
    input->seek(1, librevenge::RVNG_SEEK_CUR);
  }

  double cx = (xb + xa) / 2.0;
  double cy = (yb + ya) / 2.0;
  double rx = fabs(xb - xa) / 2.0;
  double ry = fabs(yb - ya) / 2.0;

  while (arc1 < 0.0)
    arc1 += 2*M_PI;
  while (arc1 > 2*M_PI)
    arc1 -= 2*M_PI;

  while (arc2 < 0.0)
    arc2 += 2*M_PI;
  while (arc2 > 2*M_PI)
    arc2 -= 2*M_PI;

  FHPath path;
  if (arc1 != arc2)
  {
    if (arc2 < arc1)
      arc2 += 2*M_PI;
    double x0 = cx + rx*cos(arc1);
    double y0 = cy - ry*sin(arc1);

    double x1 = cx + rx*cos(arc2);
    double y1 = cy - ry*sin(arc2);

    bool largeArc = (arc2 - arc1 > M_PI);

    path.appendMoveTo(x0, y0);
    path.appendArcTo(rx, ry, 0.0, largeArc, false, x1, y1);
    if (closed)
    {
      path.appendLineTo(cx, cy);
      path.appendLineTo(x0, y0);
      path.appendClosePath();
    }
  }
  else
  {
    arc2 += M_PI/2.0;
    double x0 = cx + rx*cos(arc1);
    double y0 = cy - ry*sin(arc1);

    double x1 = cx + rx*cos(arc2);
    double y1 = cy - ry*sin(arc2);

    path.appendMoveTo(x0, y0);
    path.appendArcTo(rx, ry, 0.0, false, false, x1, y1);
    path.appendArcTo(rx, ry, 0.0, true, false, x0, y0);
    path.appendClosePath();
  }
  path.setXFormId(xform);
  if (collector)
    collector->collectPath(m_currentRecord+1, graphicStyle, layer, path, true);
}

void libfreehand::FHParser::readParagraph(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  unsigned short size = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  _readRecordId(input);
  for (unsigned short i = 0; i < size; ++i)
  {
    _readRecordId(input);
    input->seek(22, librevenge::RVNG_SEEK_CUR);
  }
}

void libfreehand::FHParser::readPath(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  long startPosition = input->tell();
  unsigned short size = readU16(input); // 0-2
  _readRecordId(input);
  _readRecordId(input);
  input->seek(14, librevenge::RVNG_SEEK_CUR);
  unsigned short numPoints = readU16(input);
  if (m_version > 8)
    size = numPoints;
  unsigned length = (unsigned)(input->tell() - startPosition);
  length += size*27;

  input->seek(startPosition, librevenge::RVNG_SEEK_SET);
  FHInternalStream stream(input, length);
  input->seek(startPosition+length, librevenge::RVNG_SEEK_SET);

  unsigned short graphicStyle = 0;
  std::vector<unsigned char> ptrTypes;
  std::vector<std::vector<std::pair<double, double> > > path;
  bool evenOdd = false;
  bool closed = false;

  try
  {
    stream.seek(2, librevenge::RVNG_SEEK_CUR);
    graphicStyle = readU16(&stream);
    stream.seek(15, librevenge::RVNG_SEEK_CUR);
    unsigned char flag = readU8(&stream);
    evenOdd = bool(flag&2);
    closed = bool(flag&1);
    stream.seek(2, librevenge::RVNG_SEEK_CUR);
    for (unsigned short i = 0; i < numPoints  && !stream.isEnd(); ++i)
    {
      stream.seek(1, librevenge::RVNG_SEEK_CUR);
      ptrTypes.push_back(readU8(&stream));
      stream.seek(1, librevenge::RVNG_SEEK_CUR);
      std::vector<std::pair<double, double> > segment;
      for (unsigned short j = 0; j < 3 && !stream.isEnd(); ++j)
      {
        double x = _readCoordinate(&stream);
        double y = _readCoordinate(&stream);
        std::pair<double, double> tmpPoint = std::make_pair(x, y);
        segment.push_back(tmpPoint);
      }
      path.push_back(segment);
      segment.clear();
    }
  }
  catch (const EndOfStreamException &)
  {
    FH_DEBUG_MSG(("Caught EndOfStreamException, continuing\n"));
  }

  if (path.empty())
  {
    FH_DEBUG_MSG(("No path was read\n"));
    return;
  }

  FHPath fhPath;
  fhPath.appendMoveTo(path[0][0].first / 72.0, path[0][0].second / 72.0);

  unsigned i = 0;
  for (i = 0; i<path.size()-1; ++i)
    fhPath.appendCubicBezierTo(path[i][2].first / 72.0, path[i][2].second / 72.0,
                               path[i+1][1].first / 72.0, path[i+1][1].second / 72.0,
                               path[i+1][0].first / 72.0,  path[i+1][0].second / 72.0);
  if (closed)
  {
    fhPath.appendCubicBezierTo(path[i][2].first / 72.0, path[i][2].second / 72.0,
                               path[0][1].first / 72.0, path[0][1].second / 72.0,
                               path[0][0].first / 72.0, path[0][0].second / 72.0);

    fhPath.appendClosePath();
  }

  if (collector)
    collector->collectPath(m_currentRecord+1, graphicStyle, 0, fhPath, evenOdd);
}

void libfreehand::FHParser::readPathTextLineInfo(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(46, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readPatternFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(10, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readPatternLine(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(22, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readPerspectiveEnvelope(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(177, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readPerspectiveGrid(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  while (readU8(input))
  {
  }
  input->seek(58, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readPolygonFigure(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned short graphicStyle = _readRecordId(input);
  unsigned short layer = _readRecordId(input);
  input->seek(12, librevenge::RVNG_SEEK_CUR);
  unsigned short xform = _readRecordId(input);
  unsigned short numSegments = readU16(input);
  bool evenodd = bool(readU8(input));
  double cx = _readCoordinate(input) / 72.0;
  double cy = _readCoordinate(input) / 72.0;

  double r1 = _readCoordinate(input) / 72.0;
  double r2 = _readCoordinate(input) / 72.0;
  double arc1 = _readCoordinate(input) * M_PI / 180.0;
  double arc2 = _readCoordinate(input) * M_PI / 180.0;
  while (arc1 < 0.0)
    arc1 += 2.0 * M_PI;
  while (arc1 > 2.0 * M_PI)
    arc1 -= 2.0 * M_PI;

  while (arc2 < 0.0)
    arc2 += 2.0 * M_PI;
  while (arc2 > 2*M_PI)
    arc2 -= 2.0 * M_PI;
  if (arc1 > arc2)
  {
    std::swap(arc1, arc2);
    std::swap(r1, r2);
  }

  FHPath path;
  path.appendMoveTo(r1 * cos(arc1) + cx, r1 * sin(arc1) + cy);
  double deltaArc = arc2 - arc1;
  for (double arc = arc1; arc < arc1 + 2.0 * M_PI; arc += 2.0 * M_PI / numSegments)
  {
    path.appendLineTo(r1 * cos(arc) + cx, r1 * sin(arc) + cy);
    path.appendLineTo(r2 * cos(arc + deltaArc) + cx, r2 * sin(arc + deltaArc) + cy);
  }
  path.appendLineTo(r1 * cos(arc1) + cx, r1 * sin(arc1) + cy);
  path.appendClosePath();
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  path.setXFormId(xform);
  if (collector)
    collector->collectPath(m_currentRecord+1, graphicStyle, layer, path, evenodd);
}

void libfreehand::FHParser::readProcedure(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readProcessColor(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(22, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readPropLst(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  unsigned short size = readU16(input);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  for (unsigned short i = 0; i < size*2; ++i)
    _readRecordId(input);
}

void libfreehand::FHParser::readPSLine(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(8, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readRadialFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(20, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
}

void libfreehand::FHParser::readRadialFillX(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(22, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readRaggedFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(16, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readRectangle(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned short graphicStyle = readU16(input);
  unsigned short layer = readU16(input);
  input->seek(12, librevenge::RVNG_SEEK_CUR);
  unsigned short xform = readU16(input);
  double x1 = _readCoordinate(input) / 72.0;
  double y1 = _readCoordinate(input) / 72.0;
  double x2 = _readCoordinate(input) / 72.0;
  double y2 = _readCoordinate(input) / 72.0;
  double rtlt = _readCoordinate(input) / 72.0;
  double rtll = _readCoordinate(input) / 72.0;
  double rtrt = rtlt;
  double rtrr = rtll;
  double rbrb = rtlt;
  double rbrr = rtll;
  double rblb = rtlt;
  double rbll = rtll;
  bool rbl(true);
  bool rtl(true);
  bool rtr(true);
  bool rbr(true);
  if (m_version >= 11)
  {
    rtrt = _readCoordinate(input) / 72.0;
    rtrr = _readCoordinate(input) / 72.0;
    rbrb = _readCoordinate(input) / 72.0;
    rbrr = _readCoordinate(input) / 72.0;
    rblb = _readCoordinate(input) / 72.0;
    rbll = _readCoordinate(input) / 72.0;
    input->seek(9, librevenge::RVNG_SEEK_CUR);
  }
  FHPath path;

  if (FH_ALMOST_ZERO(rbll) || FH_ALMOST_ZERO(rblb))
    path.appendMoveTo(x1, y1);
  else
  {
    path.appendMoveTo(x1 - rblb, y1);
    if (rbl)
      path.appendQuadraticBezierTo(x1, y1, x1, y1 + rbll);
    else
      path.appendQuadraticBezierTo(x1 + rblb, y1 + rbll, x1, y1 + rbll);
  }
  if (FH_ALMOST_ZERO(rtll) || FH_ALMOST_ZERO(rtlt))
    path.appendLineTo(x1, y2);
  else
  {
    path.appendLineTo(x1, y2 - rtll);
    if (rtl)
      path.appendQuadraticBezierTo(x1, y2, x1 + rtlt, y2);
    else
      path.appendQuadraticBezierTo(x1 + rtlt, y2 - rtll, x1 + rtlt, y2);
  }
  if (FH_ALMOST_ZERO(rtrt) || FH_ALMOST_ZERO(rtrr))
    path.appendLineTo(x2, y2);
  else
  {
    path.appendLineTo(x2 - rtrt, y2);
    if (rtr)
      path.appendQuadraticBezierTo(x2, y2, x2, y2 - rtrr);
    else
      path.appendQuadraticBezierTo(x2 - rtrt, y2 - rtrr, x2, y2 - rtrr);
  }
  if (FH_ALMOST_ZERO(rbrr) || FH_ALMOST_ZERO(rbrb))
    path.appendLineTo(x2, y1);
  else
  {
    path.appendLineTo(x2, y1 + rbrr);
    if (rbr)
      path.appendQuadraticBezierTo(x2, y1, x2 - rbrb, y1);
    else
      path.appendQuadraticBezierTo(x2 - rbrb, y1 + rbrr, x2 - rbrb, y1);
  }
  if (FH_ALMOST_ZERO(rbll) || FH_ALMOST_ZERO(rblb))
    path.appendLineTo(x1, y1);
  else
    path.appendLineTo(x1 - rblb, y1);
  path.appendClosePath();

  path.setXFormId(xform);
  if (collector)
    collector->collectPath(m_currentRecord+1, graphicStyle, layer, path, true);
}

void libfreehand::FHParser::readSketchFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(11, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readSpotColor(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(26, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readSpotColor6(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short size = readU16(input);
  _readRecordId(input);
  if (m_version < 10)
    input->seek(22, librevenge::RVNG_SEEK_CUR);
  else
    input->seek(24, librevenge::RVNG_SEEK_CUR);
  input->seek(size*4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readStylePropLst(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  if (m_version > 8)
    input->seek(2, librevenge::RVNG_SEEK_CUR);
  unsigned short size = readU16(input);
  if (m_version <= 8)
    input->seek(2, librevenge::RVNG_SEEK_CUR);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  _readRecordId(input);
  for (unsigned short i = 0; i < size*2; ++i)
    _readRecordId(input);
}

void libfreehand::FHParser::readSwfImport(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(43, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readSymbolClass(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);
}

void libfreehand::FHParser::readSymbolInstance(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  unsigned char var1 = readU8(input);
  unsigned char var2 = readU8(input);
  input->seek(_xformCalc(var1, var2), librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readSymbolLibrary(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  unsigned short size = readU16(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  for (unsigned short i = 0; i < size+3; ++i)
    _readRecordId(input);
}

void libfreehand::FHParser::readTabTable(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short size = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  if (m_version < 10)
    input->seek(size*2, librevenge::RVNG_SEEK_CUR);
  else
    input->seek(size*6, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readTaperedFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readTaperedFillX(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  input->seek(12, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
}

void libfreehand::FHParser::readTEffect(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  unsigned short num = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  for (unsigned i = 0; i < num; ++i)
  {
    unsigned short key = readU16(input);
    input->seek(2, librevenge::RVNG_SEEK_CUR);
    if (key == 2)
      _readRecordId(input);
    else
      input->seek(4, librevenge::RVNG_SEEK_CUR);
  }
}

void libfreehand::FHParser::readTextBlok(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short size = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  input->seek(size*4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readTextColumn(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  unsigned short num = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);

  for (unsigned short i = 0; i < num; ++i)
  {
    unsigned short key = readU16(input);
    if (key == 2)
    {
      input->seek(2, librevenge::RVNG_SEEK_CUR);
      _readRecordId(input);
    }
    else
      input->seek(6, librevenge::RVNG_SEEK_CUR);
  }
}

void libfreehand::FHParser::readTextInPath(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  unsigned short num = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);
  unsigned flag = readU32(input);
  if (flag == 0xffffffff)
    input->seek(-2, librevenge::RVNG_SEEK_CUR);
  else
    input->seek(-4, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);

  for (unsigned short i = 0; i < num; ++i)
  {
    unsigned short key = readU16(input);
    if (key == 2)
    {
      input->seek(2, librevenge::RVNG_SEEK_CUR);
      _readRecordId(input);
    }
    else
      input->seek(6, librevenge::RVNG_SEEK_CUR);
  }
}

void libfreehand::FHParser::readTFOnPath(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  unsigned short num = readU16(input);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  _readRecordId(input);
  _readRecordId(input);

  for (unsigned short i = 0; i < num; ++i)
  {
    unsigned short key = readU16(input);
    if (key == 2)
    {
      input->seek(2, librevenge::RVNG_SEEK_CUR);
      _readRecordId(input);
    }
    else
      input->seek(6, librevenge::RVNG_SEEK_CUR);
  }
}

void libfreehand::FHParser::readTileFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  _readRecordId(input);
  input->seek(28, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readTintColor(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(20, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readTintColor6(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
  if (m_version < 10)
    input->seek(-2, librevenge::RVNG_SEEK_CUR);
  input->seek(36, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readTransformFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(39, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readTString(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short size2 = readU16(input);
  unsigned short size = readU16(input);
  input->seek(16, librevenge::RVNG_SEEK_CUR);
  for (unsigned short i = 0; i < size; ++i)
    _readRecordId(input);
  if (m_version < 9)
    input->seek((size2-size)*2, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readUString(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  long startPosition = input->tell();
  unsigned short size = readU16(input);
  unsigned short length = readU16(input);
  std::vector<unsigned short> ustr;
  unsigned short character = 0;
  if (length)
  {
    for (unsigned short i = 0; i < length; i++)
    {
      character = readU16(input);
      if (!character)
        break;
      ustr.push_back(character);
    }
  }

  librevenge::RVNGString str;
  _appendUTF16(str, ustr);

  FH_DEBUG_MSG(("FHParser::readUString %i (%s)\n", length, str.cstr()));
  input->seek(startPosition + (size+1)*4, librevenge::RVNG_SEEK_SET);
  if (collector)
    collector->collectUString(m_currentRecord+1, str);
}

void libfreehand::FHParser::readVDict(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  unsigned short num = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  for (unsigned short i = 0; i < num; ++i)
  {
    unsigned short key = readU16(input);
    input->seek(2, librevenge::RVNG_SEEK_CUR);
    if (key == 2)
      _readRecordId(input);
    else
      input->seek(4, librevenge::RVNG_SEEK_CUR);
  }
}

void libfreehand::FHParser::readVMpObj(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  unsigned short num = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  double minX = 0.0;
  double minY = 0.0;
  double maxX = 0.0;
  double maxY = 0.0;
  for (unsigned short i = 0; i < num; ++i)
  {
    unsigned short key = readU16(input);
    unsigned short rec = readU16(input);
    if (key == 2)
      _readRecordId(input);
    else
    {
      switch (rec)
      {
      case FH_PAGE_START_X:
      {
        minX = _readCoordinate(input) / 72.0;
        if (m_pageInfo.m_minX > 0.0)
        {
          if (m_pageInfo.m_minX > minX)
            m_pageInfo.m_minX = minX;
        }
        else
          m_pageInfo.m_minX = minX;
        break;
      }
      case FH_PAGE_START_Y:
      {
        minY = _readCoordinate(input) / 72.0;
        if (m_pageInfo.m_minY > 0.0)
        {
          if (m_pageInfo.m_minY > minY)
            m_pageInfo.m_minY = minY;
        }
        else
          m_pageInfo.m_minY = minY;
        break;
      }
      case FH_PAGE_WIDTH:
      {
        maxX = minX + _readCoordinate(input) / 72.0;
        if (m_pageInfo.m_maxX < maxX)
          m_pageInfo.m_maxX = maxX;
        break;
      }
      case FH_PAGE_HEIGHT:
      {
        maxY = minY + _readCoordinate(input) / 72.0;
        if (m_pageInfo.m_maxY < maxY)
          m_pageInfo.m_maxY = maxY;
        break;
      }
      default:
        input->seek(4, librevenge::RVNG_SEEK_CUR);
      }
    }
  }
}

void libfreehand::FHParser::readXform(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  long startPosition = input->tell();
  if (m_version < 9)
    input->seek(52, librevenge::RVNG_SEEK_CUR);
  else
  {
    unsigned char var1 = readU8(input);
    unsigned char var2 = readU8(input);
    unsigned len1 = _xformCalc(var1, var2);
    input->seek(len1, librevenge::RVNG_SEEK_CUR);
    var1 = readU8(input);
    var2 = readU8(input);
    unsigned len2 =  _xformCalc(var1, var2);
    input->seek(len2, librevenge::RVNG_SEEK_CUR);
  }
  long length = input->tell() - startPosition;
  input->seek(startPosition, librevenge::RVNG_SEEK_SET);
  double m11 = 1.0;
  double m21 = 0.0;
  double m12 = 0.0;
  double m22 = 1.0;
  double m13 = 0.0;
  double m23 = 0.0;
  unsigned char var1 = readU8(input);
  unsigned char var2 = readU8(input);
  unsigned a5 = (((~var1)&0x20)>>5);
  unsigned a4 = (((~var1)&0x10)>>4);
  unsigned a2 = (var1&0x4)>>2;
  unsigned a1 = (var1&0x2)>>1;
  unsigned a0 = var1&0x1;

  unsigned b6 = (var2&0x40) >> 6;
  unsigned b5 = (var2&0x20) >> 5;
  if (!a2)
  {
    if (a4)
      m11 = _readCoordinate(input);
    if (b6)
      m21 = _readCoordinate(input);
    if (b5)
      m12 = _readCoordinate(input);
    if (a5)
      m22 = _readCoordinate(input);
    if (a0)
      m13 = _readCoordinate(input) / 72.0;
    if (a1)
      m23 = _readCoordinate(input) / 72.0;
  }
  if (collector)
    collector->collectXform(m_currentRecord+1, m11, m21, m12, m22, m13, m23);
  input->seek(startPosition+length, librevenge::RVNG_SEEK_SET);
}

unsigned libfreehand::FHParser::_readRecordId(librevenge::RVNGInputStream *input)
{
  unsigned recid = readU16(input);
  if (recid == 0xffff)
    recid = 0x1ff00 - readU16(input);
  return recid;
}

unsigned libfreehand::FHParser::_xformCalc(unsigned char var1, unsigned char var2)
{
  unsigned a5 = (((~var1)&0x20)>>5);
  unsigned a4 = (((~var1)&0x10)>>4);
  unsigned a2 = (var1&0x4)>>2;
  unsigned a1 = (var1&0x2)>>1;
  unsigned a0 = var1&0x1;

  unsigned b6 = (var2&0x40) >> 6;
  unsigned b5 = (var2&0x20) >> 5;
  if (a2)
    return 0;

  return (a5+a4+a1+a0+b6+b5)*4;
}

double libfreehand::FHParser::_readCoordinate(librevenge::RVNGInputStream *input)
{
  double value = (double)readS16(input);
  value += (double)readU16(input) / 65536.0;
  return value;
}

void libfreehand::FHParser::_readBlockInformation(librevenge::RVNGInputStream *input, unsigned i, unsigned &layerListId, unsigned &defaultLayerId)
{
  if (i == 5)
    layerListId = _readRecordId(input);
  else if (i == 6)
    defaultLayerId = _readRecordId(input);
  else
    _readRecordId(input);
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
