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
    m_dictionary[id] = name;
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

void libvisio::FHParser::readAGDFont(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readAGDSelection(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readArrowPath(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readAttributeHolder(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readBasicFill(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readBasicLine(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readBendFilter(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readBlock(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readBrushList(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readBrush(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readBrushStroke(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readBrushTip(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readCalligraphicStroke(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readCharacterFill(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readClipGroup(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readCollector(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readColor6(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readCompositePath(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readConeFill(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readConnectorLine(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readContentFill(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readContourFill(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readCustomProc(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readDataList(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readData(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readDateTime(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readDuetFilter(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readElement(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readElemList(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readElemPropLst(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readEnvelope(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readExpandFilter(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readExtrusion(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readFHDocHeader(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readFigure(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readFileDescriptor(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readFilterAttributeHolder(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readFWBevelFilter(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readFWBlurFilter(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readFWFeatherFilter(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readFWGlowFilter(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readFWShadowFilter(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readFWSharpenFilter(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readGradientMaskFilter(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readGraphicStyle(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readGroup(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readGuides(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readHalftone(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readImageFill(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readImageImport(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readLayer(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readLensFill(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readLinearFill(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readLinePat(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readLineTable(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readList(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readMasterPageDocMan(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readMasterPageElement(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readMasterPageLayerElement(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readMasterPageLayerInstance(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readMasterPageSymbolClass(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readMasterPageSymbolInstance(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readMDict(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readMList(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readMName(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readMpObject(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readMQuickDict(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readMString(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readMultiBlend(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readMultiColorList(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readNewBlend(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readNewContourFill(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readNewRadialFill(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readOpacityFilter(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readOval(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readParagraph(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readPath(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readPathTextLineInfo(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readPatternFill(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readPatternLine(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readPerspectiveEnvelope(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readPerspectiveGrid(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readPolygonFigure(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readProcedure(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readPropLst(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readPSLine(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readRadialFill(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readRadialFillX(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readRaggedFilter(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readRectangle(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readSketchFilter(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readSpotColor6(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readStylePropLst(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readSwfImport(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readSymbolClass(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readSymbolInstance(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readSymbolLibrary(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readTabTable(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readTaperedFill(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readTaperedFillX(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readTEffect(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readTextBlok(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readTextColumn(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readTextInPath(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readTFOnPath(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readTileFill(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readTintColor6(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readTransformFilter(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readTString(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readUString(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readVDict(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readVMpObj(WPXInputStream * /* input */)
{
}

void libvisio::FHParser::readXform(WPXInputStream * /* input */)
{
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
