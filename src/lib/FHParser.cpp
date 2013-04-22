/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* libfreehand
 * Version: MPL 1.1 / GPLv2+ / LGPLv2+
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License or as specified alternatively below. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * Major Contributor(s):
 * Copyright (C) 2012 Fridrich Strba <fridrich.strba@bluewin.ch>
 *
 * All Rights Reserved.
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPLv2+"), or
 * the GNU Lesser General Public License Version 2 or later (the "LGPLv2+"),
 * in which case the provisions of the GPLv2+ or the LGPLv2+ are applicable
 * instead of those above.
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
