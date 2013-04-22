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

#ifndef __FHPARSER_H__
#define __FHPARSER_H__

#include <map>
#include <vector>
#include <libwpd/libwpd.h>
#include <libwpg/libwpg.h>

class WPXInputStream;

namespace libfreehand
{
class FHCollector;

class FHParser
{
public:
  explicit FHParser(WPXInputStream *input, FHCollector *collector);
  virtual ~FHParser();
  bool parse();
private:
  FHParser();
  FHParser(const FHParser &);
  FHParser &operator=(const FHParser &);

  void parseDictionary(WPXInputStream *input);
  void parseListOfRecords(WPXInputStream *input);
  void parseData(WPXInputStream *input);

  void readAGDFont(WPXInputStream *input);
  void readAGDSelection(WPXInputStream *input);
  void readArrowPath(WPXInputStream *input);
  void readAttributeHolder(WPXInputStream *input);
  void readBasicFill(WPXInputStream *input);
  void readBasicLine(WPXInputStream *input);
  void readBendFilter(WPXInputStream *input);
  void readBlock(WPXInputStream *input);
  void readBrushList(WPXInputStream *input);
  void readBrush(WPXInputStream *input);
  void readBrushStroke(WPXInputStream *input);
  void readBrushTip(WPXInputStream *input);
  void readCalligraphicStroke(WPXInputStream *input);
  void readCharacterFill(WPXInputStream *input);
  void readClipGroup(WPXInputStream *input);
  void readCollector(WPXInputStream *input);
  void readColor6(WPXInputStream *input);
  void readCompositePath(WPXInputStream *input);
  void readConeFill(WPXInputStream *input);
  void readConnectorLine(WPXInputStream *input);
  void readContentFill(WPXInputStream *input);
  void readContourFill(WPXInputStream *input);
  void readCustomProc(WPXInputStream *input);
  void readDataList(WPXInputStream *input);
  void readData(WPXInputStream *input);
  void readDateTime(WPXInputStream *input);
  void readDuetFilter(WPXInputStream *input);
  void readElement(WPXInputStream *input);
  void readElemList(WPXInputStream *input);
  void readElemPropLst(WPXInputStream *input);
  void readEnvelope(WPXInputStream *input);
  void readExpandFilter(WPXInputStream *input);
  void readExtrusion(WPXInputStream *input);
  void readFHDocHeader(WPXInputStream *input);
  void readFigure(WPXInputStream *input);
  void readFileDescriptor(WPXInputStream *input);
  void readFilterAttributeHolder(WPXInputStream *input);
  void readFWBevelFilter(WPXInputStream *input);
  void readFWBlurFilter(WPXInputStream *input);
  void readFWFeatherFilter(WPXInputStream *input);
  void readFWGlowFilter(WPXInputStream *input);
  void readFWShadowFilter(WPXInputStream *input);
  void readFWSharpenFilter(WPXInputStream *input);
  void readGradientMaskFilter(WPXInputStream *input);
  void readGraphicStyle(WPXInputStream *input);
  void readGroup(WPXInputStream *input);
  void readGuides(WPXInputStream *input);
  void readHalftone(WPXInputStream *input);
  void readImageFill(WPXInputStream *input);
  void readImageImport(WPXInputStream *input);
  void readLayer(WPXInputStream *input);
  void readLensFill(WPXInputStream *input);
  void readLinearFill(WPXInputStream *input);
  void readLinePat(WPXInputStream *input);
  void readLineTable(WPXInputStream *input);
  void readList(WPXInputStream *input);
  void readMasterPageDocMan(WPXInputStream *input);
  void readMasterPageElement(WPXInputStream *input);
  void readMasterPageLayerElement(WPXInputStream *input);
  void readMasterPageLayerInstance(WPXInputStream *input);
  void readMasterPageSymbolClass(WPXInputStream *input);
  void readMasterPageSymbolInstance(WPXInputStream *input);
  void readMDict(WPXInputStream *input);
  void readMList(WPXInputStream *input);
  void readMName(WPXInputStream *input);
  void readMpObject(WPXInputStream *input);
  void readMQuickDict(WPXInputStream *input);
  void readMString(WPXInputStream *input);
  void readMultiBlend(WPXInputStream *input);
  void readMultiColorList(WPXInputStream *input);
  void readNewBlend(WPXInputStream *input);
  void readNewContourFill(WPXInputStream *input);
  void readNewRadialFill(WPXInputStream *input);
  void readOpacityFilter(WPXInputStream *input);
  void readOval(WPXInputStream *input);
  void readParagraph(WPXInputStream *input);
  void readPath(WPXInputStream *input);
  void readPathTextLineInfo(WPXInputStream *input);
  void readPatternFill(WPXInputStream *input);
  void readPatternLine(WPXInputStream *input);
  void readPerspectiveEnvelope(WPXInputStream *input);
  void readPerspectiveGrid(WPXInputStream *input);
  void readPolygonFigure(WPXInputStream *input);
  void readProcedure(WPXInputStream *input);
  void readPropLst(WPXInputStream *input);
  void readPSLine(WPXInputStream *input);
  void readRadialFill(WPXInputStream *input);
  void readRadialFillX(WPXInputStream *input);
  void readRaggedFilter(WPXInputStream *input);
  void readRectangle(WPXInputStream *input);
  void readSketchFilter(WPXInputStream *input);
  void readSpotColor6(WPXInputStream *input);
  void readStylePropLst(WPXInputStream *input);
  void readSwfImport(WPXInputStream *input);
  void readSymbolClass(WPXInputStream *input);
  void readSymbolInstance(WPXInputStream *input);
  void readSymbolLibrary(WPXInputStream *input);
  void readTabTable(WPXInputStream *input);
  void readTaperedFill(WPXInputStream *input);
  void readTaperedFillX(WPXInputStream *input);
  void readTEffect(WPXInputStream *input);
  void readTextBlok(WPXInputStream *input);
  void readTextColumn(WPXInputStream *input);
  void readTextInPath(WPXInputStream *input);
  void readTFOnPath(WPXInputStream *input);
  void readTileFill(WPXInputStream *input);
  void readTintColor6(WPXInputStream *input);
  void readTransformFilter(WPXInputStream *input);
  void readTString(WPXInputStream *input);
  void readUString(WPXInputStream *input);
  void readVDict(WPXInputStream *input);
  void readVMpObj(WPXInputStream *input);
  void readXform(WPXInputStream *input);

  unsigned _readRecordId(WPXInputStream *input);

  WPXInputStream *m_input;
  FHCollector *m_collector;
  int m_version;
  std::map<unsigned short, int> m_dictionary;
  std::vector<unsigned short> m_records;
};

} // namespace libfreehand

#endif //  __FHRAPHICS_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
