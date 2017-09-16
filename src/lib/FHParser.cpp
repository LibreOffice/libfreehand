/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <string.h>

#include <unicode/utf8.h>
#include <unicode/utf16.h>
#include <lcms2.h>
#include "FHCollector.h"
#include "FHColorProfiles.h"
#include "FHConstants.h"
#include "FHInternalStream.h"
#include "FHParser.h"
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

#ifdef DEBUG
const char *getTokenName(int tokenId)
{
  if (tokenId >= FH_TOKEN_COUNT)
    return nullptr;
  const fhtoken *currentToken = wordlist;
  while (currentToken != wordlist+sizeof(wordlist)/sizeof(*wordlist))
  {
    if (currentToken->tokenId == tokenId)
      return currentToken->name;
    ++currentToken;
  }
  return nullptr;
}

#endif

} // anonymous namespace

libfreehand::FHParser::FHParser()
  : m_input(nullptr), m_collector(nullptr), m_version(-1), m_dictionary(),
    m_records(), m_currentRecord(0), m_pageInfo(), m_colorTransform(nullptr)
{
  cmsHPROFILE inProfile  = cmsOpenProfileFromMem(CMYK_icc, sizeof(CMYK_icc)/sizeof(CMYK_icc[0]));
  cmsHPROFILE outProfile = cmsCreate_sRGBProfile();

  m_colorTransform = cmsCreateTransform(inProfile, TYPE_CMYK_16, outProfile, TYPE_RGB_16, INTENT_PERCEPTUAL, 0);

  cmsCloseProfile(inProfile);
  cmsCloseProfile(outProfile);
}

libfreehand::FHParser::~FHParser()
{
  if (m_colorTransform)
    cmsDeleteTransform(m_colorTransform);
}

bool libfreehand::FHParser::parse(librevenge::RVNGInputStream *input, librevenge::RVNGDrawingInterface *painter)
{
  long dataOffset = input->tell();
  unsigned agd = readU32(input);
  if (((agd >> 24) & 0xff) == 'A' && ((agd >> 16) & 0xff) == 'G' && ((agd >> 8) & 0xff) == 'D')
    m_version = (agd & 0xff) - 0x30 + 5;
  else if (((agd >> 24) & 0xff) == 'F' && ((agd >> 16) & 0xff) == 'H' && ((agd >> 8) & 0xff) == '3')
    m_version = 3;
  else
    return false;

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
  contentCollector.outputDrawing(painter);

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
  if (count > getRemainingLength(input) / 2)
    count = getRemainingLength(input) / 2;
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
  case FH_BLENDOBJECT:
    readBlendObject(input, collector);
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
  case FH_DISPLAYTEXT:
    readDisplayText(input, collector);
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
  case FH_IMPORT:
    readImport(input, collector);
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
  case FH_PANTONECOLOR:
    readPantoneColor(input, collector);
    break;
  case FH_PARAGRAPH:
    readParagraph(input, collector);
    break;
  case FH_PATH:
    readPath(input, collector);
    break;
  case FH_PATHTEXT:
    readPathText(input, collector);
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
  case FH_PSFILL:
    readPSFill(input, collector);
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
  case FH_TEXTINPATH:
  case FH_TFONPATH:
    readTextObject(input, collector);
    break;
  case FH_TEXTEFFS:
    readTextEffs(input, collector);
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

void libfreehand::FHParser::readAGDFont(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  unsigned short num = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  FHAGDFont font;
  for (unsigned short i = 0; i < num; ++i)
  {
    unsigned key = readU32(input);
    switch (key & 0xffff)
    {
    case FH_AGD_FONT_NAME:
      font.m_fontNameId = _readRecordId(input);
      break;
    case FH_AGD_STYLE:
      font.m_fontStyle = readU32(input);
      break;
    case FH_AGD_SIZE:
      font.m_fontSize = _readCoordinate(input);
      break;
    default:
      if ((key >> 16) == 2)
        _readRecordId(input);
      else
        input->seek(4, librevenge::RVNG_SEEK_CUR);
      break;
    }
  }
  if (collector)
    collector->collectAGDFont(m_currentRecord+1, font);
}

void libfreehand::FHParser::readAGDSelection(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short size = readU16(input);
  input->seek(6+size*4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readArrowPath(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  if (m_version > 8)
    input->seek(20, librevenge::RVNG_SEEK_CUR);
  unsigned numPoints = readU16(input);
  if (m_version <= 8)
    input->seek(20, librevenge::RVNG_SEEK_CUR);
  if (m_version > 3)
    input->seek(4, librevenge::RVNG_SEEK_CUR);
  input->seek(4, librevenge::RVNG_SEEK_CUR);

  long endPos=input->tell()+27*numPoints;
  std::vector<unsigned char> ptrTypes;
  std::vector<std::vector<std::pair<double, double> > > path;
  try
  {
    for (unsigned short i = 0; i < numPoints  && !input->isEnd(); ++i)
    {
      input->seek(1, librevenge::RVNG_SEEK_CUR);
      ptrTypes.push_back(readU8(input));
      input->seek(1, librevenge::RVNG_SEEK_CUR);
      std::vector<std::pair<double, double> > segment;
      for (unsigned short j = 0; j < 3 && !input->isEnd(); ++j)
      {
        double x = _readCoordinate(input);
        double y = _readCoordinate(input);
        std::pair<double, double> tmpPoint = std::make_pair(72.*x, 72.*y);
        segment.push_back(tmpPoint);
      }
      if (segment.size() == 3)
        path.push_back(segment);
      segment.clear();
    }
  }
  catch (const EndOfStreamException &)
  {
    FH_DEBUG_MSG(("libfreehand::FHParser::readArrowPath:Caught EndOfStreamException, continuing\n"));
  }
  input->seek(endPos, librevenge::RVNG_SEEK_SET);

  if (path.empty())
  {
    FH_DEBUG_MSG(("libfreehand::FHParser::readArrowPath:No path was read\n"));
    return;
  }

  FHPath fhPath;
  fhPath.appendMoveTo(path[0][0].first / 72.0, path[0][0].second / 72.0);
  unsigned i = 0;
  for (i = 0; i<path.size()-1; ++i)
    fhPath.appendCubicBezierTo(path[i][2].first / 72.0, path[i][2].second / 72.0,
                               path[i+1][1].first / 72.0, path[i+1][1].second / 72.0,
                               path[i+1][0].first / 72.0,  path[i+1][0].second / 72.0);
  fhPath.appendCubicBezierTo(path[i][2].first / 72.0, path[i][2].second / 72.0,
                             path[0][1].first / 72.0, path[0][1].second / 72.0,
                             path[0][0].first / 72.0, path[0][0].second / 72.0);
  fhPath.appendClosePath();
  if (collector && !fhPath.empty())
    collector->collectArrowPath(m_currentRecord+1, fhPath);
}

void libfreehand::FHParser::readAttributeHolder(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHAttributeHolder attributeHolder;
  attributeHolder.m_parentId = _readRecordId(input);
  attributeHolder.m_attrId = _readRecordId(input);
  if (collector)
    collector->collectAttributeHolder(m_currentRecord+1, attributeHolder);
}

void libfreehand::FHParser::readBasicFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHBasicFill fill;
  fill.m_colorId = _readRecordId(input);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectBasicFill(m_currentRecord+1, fill);
}

void libfreehand::FHParser::readBasicLine(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHBasicLine line;
  line.m_colorId = _readRecordId(input);
  line.m_linePatternId = _readRecordId(input);
  line.m_startArrowId = _readRecordId(input);
  line.m_endArrowId = _readRecordId(input);
  line.m_mitter = _readCoordinate(input) / 72.0;
  line.m_width = _readCoordinate(input) / 72.0;
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectBasicLine(m_currentRecord+1, line);
}

void libfreehand::FHParser::readBendFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(10, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readBlendObject(librevenge::RVNGInputStream *input, libfreehand::FHCollector */*collector*/)
{
  // osnola useme
  for (int i=0; i<2; ++i) _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  input->seek(16, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readBlock(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned layerListId = 0;
  if (m_version == 10)
  {
    readU16(input);
    for (unsigned i = 1; i < 22; ++i)
      _readBlockInformation(input, i, layerListId);
    input->seek(1, librevenge::RVNG_SEEK_CUR);
    _readRecordId(input);
    _readRecordId(input);
  }
  else if (m_version == 8)
  {
    for (unsigned i = 0; i < 12; ++i)
      _readBlockInformation(input, i, layerListId);
    input->seek(14, librevenge::RVNG_SEEK_CUR);
  }
  else if (m_version < 8)
  {
    for (unsigned i = 0; i < 11; ++i)
      _readBlockInformation(input, i, layerListId);
    input->seek(10, librevenge::RVNG_SEEK_CUR);
    _readRecordId(input);
    _readRecordId(input);
    _readRecordId(input);
  }
  else
  {
    for (unsigned i = 0; i < 12; ++i)
      _readBlockInformation(input, i, layerListId);
    input->seek(14, librevenge::RVNG_SEEK_CUR);
    for (unsigned j = 3; j; --j)
      _readRecordId(input);
    input->seek(1, librevenge::RVNG_SEEK_CUR);
    for (unsigned k = m_version < 10 ? 1 : 4; k; --k)
      _readRecordId(input);
  }
  FH_DEBUG_MSG(("Parsing Block: layerListId 0x%x\n", layerListId));
  if (collector)
  {
    FHBlock block(layerListId);
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

void libfreehand::FHParser::readClipGroup(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHGroup group;
  group.m_graphicStyleId = _readRecordId(input);
  _readRecordId(input);
  if (m_version > 3)
    input->seek(4, librevenge::RVNG_SEEK_CUR);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  group.m_elementsId = _readRecordId(input);
  group.m_xFormId = _readRecordId(input);
  if (collector)
    collector->collectClipGroup(m_currentRecord+1, group);
}

void libfreehand::FHParser::readCollector(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readColor6(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned var = readU16(input);
  _readRecordId(input);
  FHRGBColor color = _readRGBColor(input);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  unsigned length = 12;
  if (var == 4)
    length = 16;
  if (var == 7)
    length = 28;
  if (var == 9)
    length = 36;
  if (m_version < 10)
    length -= 2;
  input->seek(length, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectColor(m_currentRecord+1, color);
}

void libfreehand::FHParser::readCompositePath(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHCompositePath compositePath;
  compositePath.m_graphicStyleId = _readRecordId(input);
  _readRecordId(input);
  if (m_version > 3)
    input->seek(4, librevenge::RVNG_SEEK_CUR);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  compositePath.m_elementsId = _readRecordId(input);
  if (collector)
    collector->collectCompositePath(m_currentRecord+1, compositePath);
}

void libfreehand::FHParser::readConeFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  /* It is actually a conic gradient that goes from a line on an angle phi
     to a line at an angle phi + M_PI both CW and CCW. We are unable though
   to map it to anything that exists in SVG or ODF :( The visually less
   lame approximation is a linear gradient. */

  FHLinearFill fill;
  fill.m_color1Id = _readRecordId(input);
  fill.m_color2Id = _readRecordId(input);
  fill.m_angle = 90.0;
  _readCoordinate(input); // cx
  _readCoordinate(input); // 1 - cy
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  fill.m_multiColorListId = _readRecordId(input);
  input->seek(14, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectLinearFill(m_currentRecord+1, fill);
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

void libfreehand::FHParser::readContourFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  if (m_version > 9)
  {
    FHRadialFill fill;
    fill.m_color1Id = _readRecordId(input);
    fill.m_color2Id = _readRecordId(input);
    fill.m_cx = _readCoordinate(input);
    fill.m_cy = 1.0 - _readCoordinate(input);
    input->seek(8, librevenge::RVNG_SEEK_CUR);
    fill.m_multiColorListId = _readRecordId(input);
    input->seek(2, librevenge::RVNG_SEEK_CUR);
    if (collector)
      collector->collectRadialFill(m_currentRecord+1, fill);
  }
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

void libfreehand::FHParser::readCustomProc(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHCustomProc line;
  unsigned short size = readU16(input);
  _readRecordId(input); // name
  input->seek(4, librevenge::RVNG_SEEK_CUR); // ?, and N
  for (int i=0; i<size; ++i)
  {
    int type=readU8(input);
    switch (type)
    {
    case 0:
      input->seek(7, librevenge::RVNG_SEEK_CUR);
      line.m_ids.push_back(_readRecordId(input));
      break;
    case 2:
    case 3:
    case 4:
      input->seek(3, librevenge::RVNG_SEEK_CUR);
      if (type==2)
        line.m_widths.push_back(_readCoordinate(input));
      else if (type==3)
        line.m_params.push_back(_readCoordinate(input));
      else
        line.m_angles.push_back(_readCoordinate(input));
      input->seek(2, librevenge::RVNG_SEEK_CUR);
      break;
    default:
      input->seek(9, librevenge::RVNG_SEEK_CUR);
      break;
    }
  }
  if (collector)
    collector->collectCustomProc(m_currentRecord+1, line);
}

void libfreehand::FHParser::readDataList(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned short size = readU16(input);
  FHDataList list;
  list.m_dataSize = readU32(input);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  if (size > getRemainingLength(input) / 2)
    size = getRemainingLength(input) / 2;
  list.m_elements.reserve(size);
  for (unsigned short i = 0; i < size; ++i)
    list.m_elements.push_back(_readRecordId(input));
  if (collector)
    collector->collectDataList(m_currentRecord+1, list);
}

void libfreehand::FHParser::readData(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned blockSize = readU16(input);
  unsigned dataSize = readU32(input);
  unsigned long numBytesRead = 0;
  const unsigned char *buffer = input->read(dataSize, numBytesRead);
  librevenge::RVNGBinaryData data(buffer, numBytesRead);
  input->seek(blockSize*4-dataSize, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectData(m_currentRecord+1, data);
}

void libfreehand::FHParser::readDateTime(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(14, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readDisplayText(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  FHDisplayText displayText;
  displayText.m_graphicStyleId = _readRecordId(input);
  _readRecordId(input);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  displayText.m_xFormId = _readRecordId(input);
  input->seek(16, librevenge::RVNG_SEEK_CUR);
  double dimR = _readCoordinate(input) / 72.0;
  double dimB = _readCoordinate(input) / 72.0;
  double dimL = _readCoordinate(input) / 72.0;
  double dimT = _readCoordinate(input) / 72.0;
  displayText.m_startX = dimL;
  displayText.m_startY = dimT;
  displayText.m_width = dimR - dimL;
  displayText.m_height = dimT - dimB;
  input->seek(32, librevenge::RVNG_SEEK_CUR);
  unsigned short textLength = readU16(input);
  displayText.m_justify=readU8(input);
  input->seek(1, librevenge::RVNG_SEEK_CUR);

  FH3CharProperties charProps;
  do
  {
    _readFH3CharProperties(input, charProps);
    displayText.m_charProps.push_back(charProps);
  }
  while (charProps.m_offset < textLength);
  FH3ParaProperties paraProps;
  do
  {
    _readFH3ParaProperties(input, paraProps);
    displayText.m_paraProps.push_back(paraProps);
  }
  while (paraProps.m_offset < textLength);
#ifdef DEBUG
  librevenge::RVNGString text;
#endif
  for (unsigned short i = 0; i <= textLength; ++i)
  {
    unsigned char character = readU8(input);
    displayText.m_characters.push_back(character);
#ifdef DEBUG
    _appendMacRoman(text, character);
#endif
  }
  if (collector)
    collector->collectDisplayText(m_currentRecord+1, displayText);
  FH_DEBUG_MSG(("FHParser::readDisplayText: %s\n", text.cstr()));
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

void libfreehand::FHParser::readElemPropLst(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  if (m_version > 8)
    input->seek(2, librevenge::RVNG_SEEK_CUR);
  unsigned short size = readU16(input);
  if (m_version <= 8)
    input->seek(2, librevenge::RVNG_SEEK_CUR);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  FHPropList propertyList;
  propertyList.m_parentId = _readRecordId(input);
  _readRecordId(input);
  _readPropLstElements(input, propertyList.m_elements, size);
  if (collector)
    collector->collectPropList(m_currentRecord+1, propertyList);
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
  FH_DEBUG_MSG(("Reading FHTail fake record\n"));
  FHTail fhTail;
  long startPosition = input->tell();
  fhTail.m_blockId = _readRecordId(input);
  fhTail.m_propLstId = _readRecordId(input);
  fhTail.m_fontId = _readRecordId(input);
  input->seek(0x1a+startPosition, librevenge::RVNG_SEEK_SET);
  fhTail.m_pageInfo.m_maxX = _readCoordinate(input) / 72.0;
  fhTail.m_pageInfo.m_maxY = _readCoordinate(input) / 72.0;
  input->seek(0x32+startPosition, librevenge::RVNG_SEEK_SET);
  fhTail.m_pageInfo.m_minX = 0.0;
  fhTail.m_pageInfo.m_minY = 0.0;

  if (collector)
    collector->collectFHTail(m_currentRecord+1, fhTail);
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

void libfreehand::FHParser::readFilterAttributeHolder(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHFilterAttributeHolder filterAttributeHolder;
  filterAttributeHolder.m_parentId = _readRecordId(input);
  filterAttributeHolder.m_filterId = _readRecordId(input);
  filterAttributeHolder.m_graphicStyleId = _readRecordId(input);
  if (collector)
    collector->collectFilterAttributeHolder(m_currentRecord+1, filterAttributeHolder);
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

void libfreehand::FHParser::readFWGlowFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FWGlowFilter filter;
  filter.m_colorId = _readRecordId(input);
  input->seek(3, librevenge::RVNG_SEEK_CUR);
  filter.m_inner = bool(readU8(input));
  filter.m_width = _readCoordinate(input) / 72.0;
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  filter.m_opacity = (double)(readU16(input)) / 100.0;
  filter.m_smoothness = _readCoordinate(input);
  filter.m_distribution = _readCoordinate(input) / 72.0;
  if (collector)
    collector->collectFWGlowFilter(m_currentRecord+1, filter);
}

void libfreehand::FHParser::readFWShadowFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FWShadowFilter filter;
  filter.m_colorId = _readRecordId(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  filter.m_knockOut = bool(readU8(input));
  filter.m_inner = !readU8(input);
  filter.m_distribution = _readCoordinate(input) / 72.0;
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  filter.m_opacity = (double)(readU16(input)) / 100.0;
  filter.m_smoothness = _readCoordinate(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  filter.m_angle = 360.0 - (double)(readU16(input));
  if (collector)
    collector->collectFWShadowFilter(m_currentRecord+1, filter);
}

void libfreehand::FHParser::readFWSharpenFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(16, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readGradientMaskFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  _readRecordId(input);
}

void libfreehand::FHParser::readGraphicStyle(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  unsigned short size = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  FHGraphicStyle graphicStyle;
  graphicStyle.m_parentId = _readRecordId(input);
  graphicStyle.m_attrId = _readRecordId(input);
  _readPropLstElements(input, graphicStyle.m_elements, size);
  if (collector)
    collector->collectGraphicStyle(m_currentRecord+1, graphicStyle);
}

void libfreehand::FHParser::readGroup(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHGroup group;
  group.m_graphicStyleId = _readRecordId(input);
  _readRecordId(input);
  if (m_version > 3)
    input->seek(4, librevenge::RVNG_SEEK_CUR);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  group.m_elementsId = _readRecordId(input);
  group.m_xFormId = _readRecordId(input);
  if (collector)
    collector->collectGroup(m_currentRecord+1, group);
}

void libfreehand::FHParser::readGuides(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  unsigned short size = readU16(input);
  _readRecordId(input);
  _readRecordId(input);
  /* osnola: todo, useme, there is one guide by "page", ... */
  if (m_version > 3)
    input->seek(4, librevenge::RVNG_SEEK_CUR);
  input->seek(12+size*8, librevenge::RVNG_SEEK_CUR);
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

void libfreehand::FHParser::readImageImport(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHImageImport image;
  image.m_graphicStyleId = _readRecordId(input);
  _readRecordId(input); // parent
  if (m_version > 3)
    input->seek(4, librevenge::RVNG_SEEK_CUR);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  if (m_version > 8)
    _readRecordId(input); // format name
  image.m_dataListId = _readRecordId(input);
  _readRecordId(input); // file descriptor
  image.m_xFormId = _readRecordId(input);
  image.m_startX = _readCoordinate(input) / 72.0;
  image.m_startY = _readCoordinate(input) / 72.0;
  image.m_width = _readCoordinate(input) / 72.0;
  image.m_height = _readCoordinate(input) / 72.0;
  input->seek(18, librevenge::RVNG_SEEK_CUR);
  if (m_version > 8)
  {
    unsigned char character(0);
    do
    {
      character = readU8(input);
      if (character)
        _appendMacRoman(image.m_format, character);
    }
    while (character);
  }

  if (m_version > 10)
    input->seek(2, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectImage(m_currentRecord+1, image);
}

void libfreehand::FHParser::readImport(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(34, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readLayer(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHLayer layer;
  layer.m_graphicStyleId = _readRecordId(input);
  if (m_version > 3)
    input->seek(4, librevenge::RVNG_SEEK_CUR);
  input->seek(6, librevenge::RVNG_SEEK_CUR);
  layer.m_elementsId = _readRecordId(input);
  _readRecordId(input);
  layer.m_visibility = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectLayer(m_currentRecord+1, layer);
}

void libfreehand::FHParser::readLensFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHLensFill fill;
  fill.m_colorId = _readRecordId(input);
  input->seek(6, librevenge::RVNG_SEEK_CUR);
  fill.m_value = _readCoordinate(input);
  input->seek(27, librevenge::RVNG_SEEK_CUR);
  fill.m_mode = readU8(input);
  if (collector)
    collector->collectLensFill(m_currentRecord+1, fill);
}

void libfreehand::FHParser::readLinearFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHLinearFill fill;
  fill.m_color1Id = _readRecordId(input);
  fill.m_color2Id = _readRecordId(input);
  fill.m_angle = _readCoordinate(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  fill.m_multiColorListId = _readRecordId(input);
  input->seek(16, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectLinearFill(m_currentRecord+1, fill);
}

void libfreehand::FHParser::readLinePat(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned short numStrokes = readU16(input);
  if (!numStrokes && m_version == 8)
  {
    // osnola: does not seems logical, what is that ?
    FH_DEBUG_MSG(("libfreehand::FHParser::readLinePat: checkme something is not right here"));
    input->seek(26, librevenge::RVNG_SEEK_CUR);
    return;
  }
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  FHLinePattern pattern;
  if (numStrokes > getRemainingLength(input) / 4)
    numStrokes = getRemainingLength(input) / 4;
  pattern.m_dashes.resize(size_t(numStrokes));
  for (unsigned short i=0; i<numStrokes; ++i)
    pattern.m_dashes[size_t(i)]=_readCoordinate(input);
  if (collector)
    collector->collectLinePattern(m_currentRecord+1, pattern);
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
  if (size > getRemainingLength(input) / 2)
    size = getRemainingLength(input) / 2;
  lst.m_elements.reserve(size);
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
    _appendMacRoman(name, character);
  FH_DEBUG_MSG(("FHParser::readMName %s\n", name.cstr()));
  input->seek(startPosition + (size+1)*4, librevenge::RVNG_SEEK_SET);
  if (collector)
  {
    collector->collectString(m_currentRecord+1, name);
    collector->collectName(m_currentRecord+1, name);
  }
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

void libfreehand::FHParser::readMString(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  long startPosition = input->tell();
  unsigned short size = readU16(input);
  unsigned short length = readU16(input);
  librevenge::RVNGString str;
  unsigned char character = 0;
  for (unsigned short i = 0; i < length && 0 != (character = readU8(input)); i++)
    _appendMacRoman(str, character);
  FH_DEBUG_MSG(("FHParser::readMString %s\n", str.cstr()));
  input->seek(startPosition + (size+1)*4, librevenge::RVNG_SEEK_SET);
  if (collector)
    collector->collectString(m_currentRecord+1, str);
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

void libfreehand::FHParser::readMultiColorList(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  std::vector<FHColorStop> colorStops;
  unsigned short num = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  if (num > getRemainingLength(input) / 10)
    num = getRemainingLength(input) / 10;
  colorStops.reserve(num);
  for (unsigned short i = 0; i < num; ++i)
  {
    FHColorStop colorStop;
    colorStop.m_colorId = _readRecordId(input);
    colorStop.m_position = _readCoordinate(input);
    input->seek(4, librevenge::RVNG_SEEK_CUR);
    colorStops.push_back(colorStop);
  }
  if (collector)
    collector->collectMultiColorList(m_currentRecord+1, colorStops);
}

void libfreehand::FHParser::readNewBlend(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHNewBlend newBlend;
  newBlend.m_graphicStyleId = _readRecordId(input);
  newBlend.m_parentId = _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  newBlend.m_list1Id = _readRecordId(input);
  newBlend.m_list2Id = _readRecordId(input);
  newBlend.m_list3Id = _readRecordId(input);
  input->seek(26, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectNewBlend(m_currentRecord+1, newBlend);
}

void libfreehand::FHParser::readNewContourFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHRadialFill fill;
  fill.m_color1Id = _readRecordId(input);
  fill.m_color2Id = _readRecordId(input);
  fill.m_cx = _readCoordinate(input);
  fill.m_cy = 1.0 - _readCoordinate(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  fill.m_multiColorListId = _readRecordId(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  _readCoordinate(input); // handle angle
  _readCoordinate(input); // handle wide
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectRadialFill(m_currentRecord+1, fill);
}

void libfreehand::FHParser::readNewRadialFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHRadialFill fill;
  fill.m_color1Id = _readRecordId(input);
  fill.m_color2Id = _readRecordId(input);
  fill.m_cx = _readCoordinate(input);
  fill.m_cy = 1.0 - _readCoordinate(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  fill.m_multiColorListId = _readRecordId(input);
  input->seek(23, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectRadialFill(m_currentRecord+1, fill);
}

void libfreehand::FHParser::readOpacityFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  _readRecordId(input);
  double opacity = (double)(readU16(input)) / 100.0;
  if (collector)
    collector->collectOpacityFilter(m_currentRecord+1, opacity);
}

void libfreehand::FHParser::readOval(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned short graphicStyle = _readRecordId(input);
  _readRecordId(input); // Layer
  if (m_version > 3)
    input->seek(4, librevenge::RVNG_SEEK_CUR);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
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
    arc2 = _readCoordinate(input) * M_PI / 180.0;
    arc1 = _readCoordinate(input) * M_PI / 180.0;
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
    double y0 = cy + ry*sin(arc1);

    double x1 = cx + rx*cos(arc2);
    double y1 = cy + ry*sin(arc2);

    bool largeArc = (arc2 - arc1 > M_PI);

    path.appendMoveTo(x0, y0);
    path.appendArcTo(rx, ry, 0.0, largeArc, true, x1, y1);
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
    double y0 = cy + ry*sin(arc1);

    double x1 = cx + rx*cos(arc2);
    double y1 = cy + ry*sin(arc2);

    path.appendMoveTo(x0, y0);
    path.appendArcTo(rx, ry, 0.0, false, true, x1, y1);
    path.appendArcTo(rx, ry, 0.0, true, true, x0, y0);
    path.appendClosePath();
  }
  path.setXFormId(xform);
  path.setGraphicStyleId(graphicStyle);
  path.setEvenOdd(true);
  if (collector && !path.empty())
    collector->collectPath(m_currentRecord+1, path);
}

void libfreehand::FHParser::readPantoneColor(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  _readRecordId(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  FHRGBColor color = _readRGBColor(input);
  input->seek(28, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectColor(m_currentRecord+1, color);
}

void libfreehand::FHParser::readParagraph(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  unsigned short size = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  FHParagraph paragraph;
  paragraph.m_paraStyleId = _readRecordId(input);
  paragraph.m_textBlokId = _readRecordId(input);
  if (size > getRemainingLength(input) / 24)
    size = getRemainingLength(input) / 24;
  paragraph.m_charStyleIds.reserve(size);
  for (unsigned short i = 0; i < size; ++i)
  {
    std::pair<unsigned, unsigned> charStyleId;
    charStyleId.first = readU16(input);
    charStyleId.second = _readRecordId(input);
    paragraph.m_charStyleIds.push_back(charStyleId);
    input->seek(20, librevenge::RVNG_SEEK_CUR);
  }
  if (collector)
    collector->collectParagraph(m_currentRecord+1, paragraph);
}

void libfreehand::FHParser::readPath(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned short size = readU16(input); // 0-2
  unsigned graphicStyle = _readRecordId(input);
  _readRecordId(input);
  if (m_version > 3)
    input->seek(4, librevenge::RVNG_SEEK_CUR);
  input->seek(9, librevenge::RVNG_SEEK_CUR);
  unsigned char flag = readU8(input);
  bool evenOdd = bool(flag&2);
  bool closed = bool(flag&1);
  unsigned short numPoints = readU16(input);
  if (m_version > 8)
    size = numPoints;

  std::vector<unsigned char> ptrTypes;
  std::vector<std::vector<std::pair<double, double> > > path;

  try
  {
    for (unsigned short i = 0; i < numPoints  && !input->isEnd(); ++i)
    {
      input->seek(1, librevenge::RVNG_SEEK_CUR);
      ptrTypes.push_back(readU8(input));
      input->seek(1, librevenge::RVNG_SEEK_CUR);
      std::vector<std::pair<double, double> > segment;
      for (unsigned short j = 0; j < 3 && !input->isEnd(); ++j)
      {
        double x = _readCoordinate(input);
        double y = _readCoordinate(input);
        std::pair<double, double> tmpPoint = std::make_pair(x, y);
        segment.push_back(tmpPoint);
      }
      if (segment.size() == 3)
        path.push_back(segment);
      segment.clear();
    }
    input->seek((size-numPoints)*27, librevenge::RVNG_SEEK_CUR);
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

  fhPath.setGraphicStyleId(graphicStyle);
  fhPath.setEvenOdd(evenOdd);
  if (collector && !fhPath.empty())
    collector->collectPath(m_currentRecord+1, fhPath);
}

void libfreehand::FHParser::readPathText(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHPathText group;
  group.m_elementsId=_readRecordId(input);
  group.m_layerId=_readRecordId(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR); // debText?
  group.m_textSize=readU16(input);
  input->seek(4, librevenge::RVNG_SEEK_CUR); // 0,0
  group.m_displayTextId=_readRecordId(input);
  group.m_shapeId=_readRecordId(input);

  if (collector)
    collector->collectPathText(m_currentRecord+1, group);
}

void libfreehand::FHParser::readPathTextLineInfo(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  // osnola: only try for N0=5, N1=2, N2=5
  input->seek(46, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readPatternFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHPatternFill fill;
  fill.m_colorId = _readRecordId(input);
  for (unsigned i = 0; i < 8; ++i)
    fill.m_pattern[i] = readU8(input);
  if (collector)
    collector->collectPatternFill(m_currentRecord+1, fill);
}

void libfreehand::FHParser::readPatternLine(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHPatternLine line;
  line.m_colorId = _readRecordId(input);
  int numOnes=0;
  for (size_t j=0; j < 8; ++j)
  {
    uint8_t val=static_cast<uint8_t>(readU8(input));
    for (int b=0; b < 8; b++)
    {
      if (val&1) ++numOnes;
      val = uint8_t(val>>1);
    }
  }
  line.m_percentPattern=float(numOnes)/64.f;
  line.m_mitter = _readCoordinate(input) / 72.0;
  line.m_width = _readCoordinate(input) / 72.0;
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectPatternLine(m_currentRecord+1, line);
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
  _readRecordId(input); // Layer
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
  path.setGraphicStyleId(graphicStyle);
  path.setEvenOdd(evenodd);
  if (collector && !path.empty())
    collector->collectPath(m_currentRecord+1, path);
}

void libfreehand::FHParser::readProcedure(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readProcessColor(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  _readRecordId(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  FHRGBColor color = _readRGBColor(input);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  if (color.black())
    color = _readCMYKColor(input);
  else
    input->seek(8, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectColor(m_currentRecord+1, color);
}

void libfreehand::FHParser::readPropLst(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned short size2 = readU16(input);
  unsigned short size = readU16(input);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  FHPropList propertyList;
  _readPropLstElements(input, propertyList.m_elements, size);
  /* osnola: TODO
     - if version>3, look if pages is defined, if yes, the zone defines the list of pages, */
  if (m_version < 9)
    input->seek((size2 - size)*4, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectPropList(m_currentRecord+1, propertyList);
}

void libfreehand::FHParser::readPSFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHBasicFill fill;
  fill.m_colorId = _readRecordId(input);
  _readRecordId(input); // command string
  if (collector)
    collector->collectBasicFill(m_currentRecord+1, fill);
}

void libfreehand::FHParser::readPSLine(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHBasicLine line;
  line.m_colorId = _readRecordId(input);
  _readRecordId(input); // command string
  line.m_width = _readCoordinate(input) / 72.0;
  if (collector)
    collector->collectBasicLine(m_currentRecord+1, line);
}

void libfreehand::FHParser::readRadialFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHRadialFill fill;
  fill.m_color1Id = _readRecordId(input);
  fill.m_color2Id = _readRecordId(input);
  if (m_version==3)   // 0,0 => center
  {
    fill.m_cx = 0.5+0.5*_readCoordinate(input);
    fill.m_cy = 0.5+0.5*_readCoordinate(input);
  }
  else
  {
    fill.m_cx = _readCoordinate(input);
    fill.m_cy = 1.0 - _readCoordinate(input);
  }
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectRadialFill(m_currentRecord+1, fill);
}

void libfreehand::FHParser::readRadialFillX(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHRadialFill fill;
  fill.m_color1Id = _readRecordId(input);
  fill.m_color2Id = _readRecordId(input);
  fill.m_cx = _readCoordinate(input);
  fill.m_cy = 1.0 - _readCoordinate(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  fill.m_multiColorListId = _readRecordId(input);
  if (collector)
    collector->collectRadialFill(m_currentRecord+1, fill);
}

void libfreehand::FHParser::readRaggedFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(16, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readRectangle(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned graphicStyle = _readRecordId(input);
  _readRecordId(input);
  if (m_version > 3)
    input->seek(4, librevenge::RVNG_SEEK_CUR);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  unsigned xform = _readRecordId(input);
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
    path.appendMoveTo(x1 + rblb, y1);
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
    path.appendLineTo(x1 + rblb, y1);
  path.appendClosePath();

  path.setXFormId(xform);
  path.setGraphicStyleId(graphicStyle);
  path.setEvenOdd(true);
  if (collector && !path.empty())
    collector->collectPath(m_currentRecord+1, path);
}

void libfreehand::FHParser::readSketchFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(11, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readSpotColor(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  _readRecordId(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  FHRGBColor color = _readRGBColor(input);
  input->seek(16, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectColor(m_currentRecord+1, color);
}

void libfreehand::FHParser::readSpotColor6(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned short size = readU16(input);
  _readRecordId(input);
  FHRGBColor color = _readRGBColor(input);
  if (m_version < 10)
    input->seek(16, librevenge::RVNG_SEEK_CUR);
  else
    input->seek(18, librevenge::RVNG_SEEK_CUR);
  input->seek(size*4, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectColor(m_currentRecord+1, color);
}

void libfreehand::FHParser::readStylePropLst(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  if (m_version > 8)
    input->seek(2, librevenge::RVNG_SEEK_CUR);
  unsigned short size = readU16(input);
  if (m_version <= 8)
    input->seek(2, librevenge::RVNG_SEEK_CUR);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  FHPropList propertyList;
  propertyList.m_parentId = _readRecordId(input);
  _readRecordId(input);
  _readPropLstElements(input, propertyList.m_elements, size);
  if (collector)
    collector->collectPropList(m_currentRecord+1, propertyList);
}

void libfreehand::FHParser::readSwfImport(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHImageImport image;
  image.m_graphicStyleId = _readRecordId(input);
  _readRecordId(input); // parent
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input); // format name
  image.m_dataListId = _readRecordId(input);
  _readRecordId(input); // file descriptor
  image.m_xFormId = _readRecordId(input);
  image.m_startX = _readCoordinate(input) / 72.0;
  image.m_startY = _readCoordinate(input) / 72.0;
  image.m_width = _readCoordinate(input) / 72.0;
  image.m_height = _readCoordinate(input) / 72.0;
  input->seek(7, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectImage(m_currentRecord+1, image);
}

void libfreehand::FHParser::readSymbolClass(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHSymbolClass symbolClass;
  symbolClass.m_nameId = _readRecordId(input);
  symbolClass.m_groupId = _readRecordId(input);
  symbolClass.m_dateTimeId = _readRecordId(input);
  symbolClass.m_symbolLibraryId = _readRecordId(input);
  symbolClass.m_listId = _readRecordId(input);
  if (collector)
    collector->collectSymbolClass(m_currentRecord+1, symbolClass);
}

void libfreehand::FHParser::readSymbolInstance(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHSymbolInstance symbolInstance;
  symbolInstance.m_graphicStyleId = _readRecordId(input);
  symbolInstance.m_parentId = _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  symbolInstance.m_symbolClassId = _readRecordId(input);
  unsigned char var1 = readU8(input);
  unsigned char var2 = readU8(input);
  if (!(var1&0x4))
  {
    if (!(var1&0x10))
      symbolInstance.m_xForm.m_m11 = _readCoordinate(input);
    if (var2&0x40)
      symbolInstance.m_xForm.m_m21 = _readCoordinate(input);
    if (var2&0x20)
      symbolInstance.m_xForm.m_m12 = _readCoordinate(input);
    if (!(var1&0x20))
      symbolInstance.m_xForm.m_m22 = _readCoordinate(input);
    if (var1&0x1)
      symbolInstance.m_xForm.m_m13 = _readCoordinate(input) / 72.0;
    if (var1&0x2)
      symbolInstance.m_xForm.m_m23 = _readCoordinate(input) / 72.0;
  }
  if (collector)
    collector->collectSymbolInstance(m_currentRecord+1, symbolInstance);
}

void libfreehand::FHParser::readSymbolLibrary(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  unsigned short size = readU16(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  for (unsigned short i = 0; i < size+3; ++i)
    _readRecordId(input);
}

void libfreehand::FHParser::readTabTable(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned short size = readU16(input);
  unsigned short n = readU16(input);
  long endPos=input->tell()+6*size;
  if (n>size)
  {
    FH_DEBUG_MSG(("libfreehand::FHParser::readTabTable: the number of tabs seems bad\n"));
    input->seek(endPos, librevenge::RVNG_SEEK_SET);
    return;
  }
  std::vector<FHTab> tabs;
  tabs.resize(size_t(n));
  for (size_t i=0; i<size_t(n); ++i)
  {
    tabs[i].m_type=readU16(input);
    tabs[i].m_position=_readCoordinate(input);
  }
  if (collector)
    collector->collectTabTable(m_currentRecord+1, tabs);
  input->seek(endPos, librevenge::RVNG_SEEK_SET);
}

void libfreehand::FHParser::readTaperedFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHLinearFill fill;
  fill.m_color1Id = _readRecordId(input);
  fill.m_color2Id = _readRecordId(input);
  fill.m_angle = -_readCoordinate(input);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectLinearFill(m_currentRecord+1, fill);
}

void libfreehand::FHParser::readTaperedFillX(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHLinearFill fill;
  fill.m_color1Id = _readRecordId(input);
  fill.m_color2Id = _readRecordId(input);
  fill.m_angle = _readCoordinate(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  fill.m_multiColorListId = _readRecordId(input);
  if (collector)
    collector->collectLinearFill(m_currentRecord+1, fill);
}

void libfreehand::FHParser::readTEffect(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHTEffect eff;
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  unsigned short num = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  for (unsigned i = 0; i < num; ++i)
  {
    unsigned short key = readU16(input);
    unsigned short rec = readU16(input);
    if (key == 2)
    {
      unsigned id=_readRecordId(input);
      switch (rec)
      {
      case 0x1a91:
        eff.m_nameId=id;
        break;
      case 0x1ab9:
        eff.m_colorId[0]=id;
        break;
      case 0x1ac1:
        eff.m_colorId[1]=id;
        break;
      default:
        break;
      }
    }
    else
      input->seek(4, librevenge::RVNG_SEEK_CUR);
  }
  if (collector)
    collector->collectTEffect(m_currentRecord+1, eff);
}

void libfreehand::FHParser::readTextBlok(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned short size = readU16(input);
  unsigned short length = readU16(input);
  if (length > getRemainingLength(input) / 2)
    length = getRemainingLength(input) / 2;
  std::vector<unsigned short> characters;
  characters.reserve(length);
  for (unsigned i = 0; i < length; ++i)
    characters.push_back(readU16(input));
  input->seek(size*4 - length*2, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectTextBlok(m_currentRecord+1, characters);
#ifdef DEBUG
  librevenge::RVNGString text;
  _appendUTF16(text, characters);
  FH_DEBUG_MSG(("FHParser::readTextBlock %s\n", text.cstr()));
#endif
}

void libfreehand::FHParser::readTextEffs(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned num = readU16(input);
  FHTEffect eff;
  eff.m_nameId=_readRecordId(input);
  eff.m_shortNameId=_readRecordId(input);
  input->seek(num==0 ? 16 : 18, librevenge::RVNG_SEEK_CUR);
  int numId=0;
  for (unsigned i = 0; i < num; ++i)
  {
    readU16(input);
    unsigned rec = readU16(input);
    if (rec == 7)
    {
      input->seek(6, librevenge::RVNG_SEEK_CUR);
      unsigned id=_readRecordId(input);
      if (readU32(input))
      {
        input->seek(-4, librevenge::RVNG_SEEK_CUR);
        if (numId<2)
          eff.m_colorId[numId++]=id;
      }
    }
    else
      input->seek(12, librevenge::RVNG_SEEK_CUR);
  }
  if (collector)
    collector->collectTEffect(m_currentRecord+1, eff);
}

void libfreehand::FHParser::readTextObject(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  unsigned short num = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  FHTextObject textObject;
  textObject.m_graphicStyleId = _readRecordId(input);
  _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  textObject.m_xFormId = _readRecordId(input);
  textObject.m_tStringId = _readRecordId(input);
  textObject.m_vmpObjId = _readRecordId(input);

  for (unsigned short i = 0; i < num; ++i)
  {
    unsigned key = readU32(input);
    switch (key & 0xffff)
    {
    case FH_DIMENSION_HEIGHT:
      textObject.m_height = _readCoordinate(input) / 72.0;
      break;
    case FH_DIMENSION_LEFT:
      textObject.m_startX = _readCoordinate(input) / 72.0;
      break;
    case FH_DIMENSION_TOP:
      textObject.m_startY = _readCoordinate(input) / 72.0;
      break;
    case FH_DIMENSION_WIDTH:
      textObject.m_width = _readCoordinate(input) / 72.0;
      break;
    case FH_ROWBREAK_FIRST:
      textObject.m_rowBreakFirst = readU32(input);
      break;
    case FH_COL_SEPARATOR:
      textObject.m_colSep = _readCoordinate(input) / 72.0;
      break;
    case FH_COL_NUM:
      textObject.m_colNum = readU32(input);
      break;
    case FH_ROW_SEPARATOR:
      textObject.m_rowSep = _readCoordinate(input) / 72.0;
      break;
    case FH_ROW_NUM:
      textObject.m_rowNum = readU32(input);
      break;
    case FH_TEXT_PATH_ID:
      textObject.m_pathId=_readRecordId(input);
      break;
    case FH_TEXT_BEGIN_POS:
      textObject.m_beginPos=readU32(input);
      break;
    case FH_TEXT_END_POS:
      textObject.m_endPos=readU32(input);
      break;
    default:
      if ((key >> 16) == 2)
        _readRecordId(input);
      else
        readU32(input);
    }
  }
  if (collector)
    collector->collectTextObject(m_currentRecord+1, textObject);
}

void libfreehand::FHParser::readTileFill(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  FHTileFill fill;
  fill.m_xFormId = _readRecordId(input);
  fill.m_groupId = _readRecordId(input);
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  fill.m_scaleX = _readCoordinate(input);
  fill.m_scaleY = _readCoordinate(input);
  fill.m_offsetX = _readCoordinate(input);
  fill.m_offsetY = _readCoordinate(input);
  fill.m_angle = _readCoordinate(input);
  if (collector)
    collector->collectTileFill(m_currentRecord+1, fill);
}

void libfreehand::FHParser::readTintColor(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  _readRecordId(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  FHRGBColor color = _readRGBColor(input);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  if (color.black())
  {
    FHTintColor tint;
    tint.m_baseColorId = _readRecordId(input);
    tint.m_tint = readU16(input);
    input->seek(2, librevenge::RVNG_SEEK_CUR);
    if (collector)
      collector->collectTintColor(m_currentRecord+1, tint);
  }
  else
  {
    _readRecordId(input);
    input->seek(4, librevenge::RVNG_SEEK_CUR);
    if (collector)
      collector->collectColor(m_currentRecord+1, color);
  }
}

void libfreehand::FHParser::readTintColor6(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  _readRecordId(input);
  FHRGBColor color = _readRGBColor(input);
  if (m_version < 10)
    input->seek(26, librevenge::RVNG_SEEK_CUR);
  else
    input->seek(28, librevenge::RVNG_SEEK_CUR);
  if (collector)
    collector->collectColor(m_currentRecord+1, color);
}

void libfreehand::FHParser::readTransformFilter(librevenge::RVNGInputStream *input, libfreehand::FHCollector * /* collector */)
{
  input->seek(39, librevenge::RVNG_SEEK_CUR);
}

void libfreehand::FHParser::readTString(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  unsigned short size2 = readU16(input);
  unsigned short size = readU16(input);
  input->seek(16, librevenge::RVNG_SEEK_CUR);
  if (size > getRemainingLength(input) / 2)
    size = getRemainingLength(input) / 2;
  std::vector<unsigned> elements;
  elements.reserve(size);
  for (unsigned short i = 0; i < size; ++i)
    elements.push_back(_readRecordId(input));
  if (m_version < 9)
    input->seek((size2-size)*2, librevenge::RVNG_SEEK_CUR);
  if (collector && !elements.empty())
    collector->collectTString(m_currentRecord+1, elements);
}

void libfreehand::FHParser::readUString(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  long startPosition = input->tell();
  unsigned short size = readU16(input);
  unsigned short length = readU16(input);
  if (length > getRemainingLength(input) / 2)
    length = getRemainingLength(input) / 2;
  std::vector<unsigned short> ustr;
  ustr.reserve(length);
  for (unsigned short i = 0; i < length; i++)
  {
    unsigned short character = 0;
    character = readU16(input);
    if (!character)
      break;
    ustr.push_back(character);
  }

  librevenge::RVNGString str;
  _appendUTF16(str, ustr);

  FH_DEBUG_MSG(("FHParser::readUString %i (%s)\n", length, str.cstr()));
  input->seek(startPosition + (size+1)*4, librevenge::RVNG_SEEK_SET);
  if (collector)
    collector->collectString(m_currentRecord+1, str);
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

void libfreehand::FHParser::readVMpObj(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  unsigned short num = readU16(input);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  double minX = 0.0;
  double minY = 0.0;
  double maxX = 0.0;
  double maxY = 0.0;
  FHParagraphProperties paraProps;
  std::unique_ptr<libfreehand::FHCharProperties> charProps;
  for (unsigned short i = 0; i < num; ++i)
  {
    unsigned short key = readU16(input);
    unsigned short rec = readU16(input);
    switch (rec)
    {
    case FH_PAGE_START_X:
    case FH_PAGE_START_X2:
    {
      minX = _readCoordinate(input) / 72.0;
      if (m_pageInfo.m_minX > 0.0)
        m_pageInfo.m_minX=std::min(minX,m_pageInfo.m_minX);
      else
        m_pageInfo.m_minX = minX;
      break;
    }
    case FH_PAGE_START_Y:
    case FH_PAGE_START_Y2:
    {
      minY = _readCoordinate(input) / 72.0;
      if (m_pageInfo.m_minY > 0.0)
        m_pageInfo.m_minY=std::min(minY,m_pageInfo.m_minY);
      else
        m_pageInfo.m_minY = minY;
      break;
    }
    case FH_PAGE_WIDTH:
    {
      maxX = minX + _readCoordinate(input) / 72.0;
      m_pageInfo.m_maxX = std::max(maxX,m_pageInfo.m_maxX);
      break;
    }
    case FH_PAGE_HEIGHT:
    {
      maxY = minY + _readCoordinate(input) / 72.0;
      m_pageInfo.m_maxY = std::max(maxY,m_pageInfo.m_maxY);
      break;
    }
    case FH_PARA_LEFT_INDENT:
    case FH_PARA_RIGHT_INDENT:
    case FH_PARA_TEXT_INDENT:
    case FH_PARA_SPC_ABOVE:
    case FH_PARA_SPC_BELLOW:
    case FH_PARA_LEADING:
      paraProps.m_idToDoubleMap[rec]=_readCoordinate(input);
      break;
    case FH_PARA_LINE_TOGETHER:
    case FH_PARA_TEXT_ALIGN:
    case FH_PARA_LEADING_TYPE:
    case FH_PARA_KEEP_SAME_LINE:
      paraProps.m_idToIntMap[rec]=readU32(input);
      break;
    case FH_PARA_TAB_TABLE_ID:
      paraProps.m_idToZoneIdMap[rec]=_readRecordId(input);
      break;
    case FH_TEFFECT_ID:
    {
      if (!charProps)
        charProps.reset(new libfreehand::FHCharProperties());
      charProps->m_tEffectId = _readRecordId(input);
      break;
    }
    case FH_TXT_COLOR_ID:
    {
      if (!charProps)
        charProps.reset(new libfreehand::FHCharProperties());
      charProps->m_textColorId = _readRecordId(input);
      break;
    }
    case FH_FONT_ID:
    {
      if (!charProps)
        charProps.reset(new libfreehand::FHCharProperties());
      charProps->m_fontId = _readRecordId(input);
      break;
    }
    case FH_FONT_SIZE:
    {
      if (!charProps)
        charProps.reset(new libfreehand::FHCharProperties());
      charProps->m_fontSize = _readCoordinate(input);
      break;
    }
    case FH_FONT_NAME:
    {
      if (!charProps)
        charProps.reset(new libfreehand::FHCharProperties());
      charProps->m_fontNameId = _readRecordId(input);
      break;
    }
    case FH_BASELN_SHIFT:
    case FH_HOR_SCALE:
    case FH_RNG_KERN:
      if (!charProps)
        charProps.reset(new libfreehand::FHCharProperties());
      charProps->m_idToDoubleMap[rec]=_readCoordinate(input);
      break;
    default:
      if (key == 2)
        _readRecordId(input);
      else
        input->seek(4, librevenge::RVNG_SEEK_CUR);
    }
  }
  if (collector)
  {
    if (charProps)
      collector->collectCharProps(m_currentRecord+1, *charProps);
    if (!paraProps.empty())
      collector->collectParagraphProps(m_currentRecord+1, paraProps);
  }
}

void libfreehand::FHParser::readXform(librevenge::RVNGInputStream *input, libfreehand::FHCollector *collector)
{
  double m11 = 1.0;
  double m21 = 0.0;
  double m12 = 0.0;
  double m22 = 1.0;
  double m13 = 0.0;
  double m23 = 0.0;
  if (m_version < 9)
  {
    input->seek(2, librevenge::RVNG_SEEK_CUR);
    m11 = _readCoordinate(input);
    m21 = _readCoordinate(input);
    m12 = _readCoordinate(input);
    m22 = _readCoordinate(input);
    m13 = _readCoordinate(input) / 72.0;
    m23 = _readCoordinate(input) / 72.0;
    input->seek(26, librevenge::RVNG_SEEK_CUR);
  }
  else
  {
    unsigned char var1 = readU8(input);
    unsigned char var2 = readU8(input);
    if (!(var1&0x4))
    {
      if (!(var1&0x10))
        m11 = _readCoordinate(input);
      if (var2&0x40)
        m21 = _readCoordinate(input);
      if (var2&0x20)
        m12 = _readCoordinate(input);
      if (!(var1&0x20))
        m22 = _readCoordinate(input);
      if (var1&0x1)
        m13 = _readCoordinate(input) / 72.0;
      if (var1&0x2)
        m23 = _readCoordinate(input) / 72.0;
    }
    var1 = readU8(input);
    var2 = readU8(input);
    input->seek(_xformCalc(var1, var2), librevenge::RVNG_SEEK_CUR);
  }
  if (collector)
    collector->collectXform(m_currentRecord+1, m11, m21, m12, m22, m13, m23);
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
  if (var1&0x4)
    return 0;
  unsigned length = 0;
  if (!(var1&0x20))
    length += 4;
  if (!(var1&0x10))
    length += 4;
  if (var1&0x2)
    length += 4;
  if (var1&0x1)
    length += 4;
  if (var2&0x40)
    length += 4;
  if (var2&0x20)
    length += 4;
  return length;
}

double libfreehand::FHParser::_readCoordinate(librevenge::RVNGInputStream *input)
{
  return (double)readS32(input)/65536.;
}

libfreehand::FHRGBColor libfreehand::FHParser::_readRGBColor(librevenge::RVNGInputStream *input)
{
  FHRGBColor tmpColor;
  tmpColor.m_red = readU16(input);
  tmpColor.m_green = readU16(input);
  tmpColor.m_blue = readU16(input);
  return tmpColor;
}

libfreehand::FHRGBColor libfreehand::FHParser::_readCMYKColor(librevenge::RVNGInputStream *input)
{
  unsigned short cmyk[4] = { 0, 0, 0, 0 };

  cmyk[3] = readU16(input); // K
  cmyk[0] = readU16(input); // C
  cmyk[1] = readU16(input); // M
  cmyk[2] = readU16(input); // Y

  unsigned short rgb[3] = { 0, 0, 0 };

  cmsDoTransform(m_colorTransform, cmyk, rgb, 1);

  FHRGBColor tmpColor;
  tmpColor.m_red = rgb[0];
  tmpColor.m_green = rgb[1];
  tmpColor.m_blue = rgb[2];
  return tmpColor;
}

void libfreehand::FHParser::_readBlockInformation(librevenge::RVNGInputStream *input, unsigned i, unsigned &layerListId)
{
  if (i == 5)
    layerListId = _readRecordId(input);
  else
    _readRecordId(input);
}

void libfreehand::FHParser::_readPropLstElements(librevenge::RVNGInputStream *input, std::map<unsigned, unsigned> &properties, unsigned size)
{
  for (unsigned i = 0; i < size; ++i)
  {
    unsigned nameId = _readRecordId(input);
    unsigned valueId = _readRecordId(input);
    if (nameId && valueId)
      properties[nameId] = valueId;
  }
}

void libfreehand::FHParser::_readFH3CharProperties(librevenge::RVNGInputStream *input, FH3CharProperties &charProps)
{
  charProps.m_offset = readU16(input);
  unsigned flags = readU16(input);
  if (flags & 0x1) // xPos
    _readCoordinate(input);
  if (flags & 0x2) // kerning
    _readCoordinate(input);
  if (flags & 0x4)
    charProps.m_fontNameId = _readRecordId(input);
  if (flags & 0x8)
    charProps.m_fontSize = _readCoordinate(input);
  if (flags & 0x10)
  {
    unsigned long leading=readU32(input);
    if (leading==0xFFFF0000)
      charProps.m_leading=-1;
    else if (leading==0xFFFE0000)
      charProps.m_leading=-1;
    else if (leading&0x80000000)
    {
      FH_DEBUG_MSG(("FHParser::_readFH3CharProperties: unexpected! %lx\n", leading));
    }
    else
      charProps.m_leading=double(leading)/65536.;
  }
  if (flags & 0x20)
    charProps.m_fontStyle = readU32(input);
  if (flags & 0x40)
    charProps.m_fontColorId = _readRecordId(input);
  if (flags & 0x80)
    charProps.m_textEffsId = _readRecordId(input);
  if (flags & 0x100)
    charProps.m_letterSpacing=_readCoordinate(input);
  if (flags & 0x200)
    charProps.m_wordSpacing=_readCoordinate(input);
  if (flags & 0x400) // in percent
    charProps.m_horizontalScale=_readCoordinate(input);
  if (flags & 0x800)
    charProps.m_baselineShift = _readCoordinate(input);
  if (flags & 0x1000)
  {
    FH_DEBUG_MSG(("FHParser::_readFH3CharProperties: NEW FLAG IN DISPLAY TEXT! %x\n", flags));
  }
}

void libfreehand::FHParser::_readFH3ParaProperties(librevenge::RVNGInputStream *input, FH3ParaProperties &paraProps)
{
  paraProps.m_offset = readU16(input);
  input->seek(28, librevenge::RVNG_SEEK_CUR);
  // osnola: todo
}


/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
