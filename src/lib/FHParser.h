/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __FHPARSER_H__
#define __FHPARSER_H__

#include <map>
#include <vector>
#include <libwpd/libwpd.h>
#include <libwpg/libwpg.h>
#include "FHTypes.h"

#define FH_PAGE_START_X 0x1c24
#define FH_PAGE_START_Y 0x1c2c
#define FH_PAGE_WIDTH 0x1c34
#define FH_PAGE_HEIGHT 0x1c3c

class WPXInputStream;

namespace libfreehand
{
class FHCollector;

class FHParser
{
public:
  explicit FHParser();
  virtual ~FHParser();
  bool parse(WPXInputStream *input, libwpg::WPGPaintInterface *painter);
private:
  FHParser(const FHParser &);
  FHParser &operator=(const FHParser &);

  void parseDictionary(WPXInputStream *input);
  void parseRecordList(WPXInputStream *input);
  void parseRecords(WPXInputStream *input, FHCollector *collector);

  void readAGDFont(WPXInputStream *input, FHCollector *collector);
  void readAGDSelection(WPXInputStream *input, FHCollector *collector);
  void readArrowPath(WPXInputStream *input, FHCollector *collector);
  void readAttributeHolder(WPXInputStream *input, FHCollector *collector);
  void readBasicFill(WPXInputStream *input, FHCollector *collector);
  void readBasicLine(WPXInputStream *input, FHCollector *collector);
  void readBendFilter(WPXInputStream *input, FHCollector *collector);
  void readBlock(WPXInputStream *input, FHCollector *collector);
  void readBrushList(WPXInputStream *input, FHCollector *collector);
  void readBrush(WPXInputStream *input, FHCollector *collector);
  void readBrushStroke(WPXInputStream *input, FHCollector *collector);
  void readBrushTip(WPXInputStream *input, FHCollector *collector);
  void readCalligraphicStroke(WPXInputStream *input, FHCollector *collector);
  void readCharacterFill(WPXInputStream *input, FHCollector *collector);
  void readClipGroup(WPXInputStream *input, FHCollector *collector);
  void readCollector(WPXInputStream *input, FHCollector *collector);
  void readColor6(WPXInputStream *input, FHCollector *collector);
  void readCompositePath(WPXInputStream *input, FHCollector *collector);
  void readConeFill(WPXInputStream *input, FHCollector *collector);
  void readConnectorLine(WPXInputStream *input, FHCollector *collector);
  void readContentFill(WPXInputStream *input, FHCollector *collector);
  void readContourFill(WPXInputStream *input, FHCollector *collector);
  void readCustomProc(WPXInputStream *input, FHCollector *collector);
  void readDataList(WPXInputStream *input, FHCollector *collector);
  void readData(WPXInputStream *input, FHCollector *collector);
  void readDateTime(WPXInputStream *input, FHCollector *collector);
  void readDuetFilter(WPXInputStream *input, FHCollector *collector);
  void readElement(WPXInputStream *input, FHCollector *collector);
  void readElemList(WPXInputStream *input, FHCollector *collector);
  void readElemPropLst(WPXInputStream *input, FHCollector *collector);
  void readEnvelope(WPXInputStream *input, FHCollector *collector);
  void readExpandFilter(WPXInputStream *input, FHCollector *collector);
  void readExtrusion(WPXInputStream *input, FHCollector *collector);
  void readFHDocHeader(WPXInputStream *input, FHCollector *collector);
  void readFHTail(WPXInputStream *input, FHCollector *collector);
  void readFigure(WPXInputStream *input, FHCollector *collector);
  void readFileDescriptor(WPXInputStream *input, FHCollector *collector);
  void readFilterAttributeHolder(WPXInputStream *input, FHCollector *collector);
  void readFWBevelFilter(WPXInputStream *input, FHCollector *collector);
  void readFWBlurFilter(WPXInputStream *input, FHCollector *collector);
  void readFWFeatherFilter(WPXInputStream *input, FHCollector *collector);
  void readFWGlowFilter(WPXInputStream *input, FHCollector *collector);
  void readFWShadowFilter(WPXInputStream *input, FHCollector *collector);
  void readFWSharpenFilter(WPXInputStream *input, FHCollector *collector);
  void readGradientMaskFilter(WPXInputStream *input, FHCollector *collector);
  void readGraphicStyle(WPXInputStream *input, FHCollector *collector);
  void readGroup(WPXInputStream *input, FHCollector *collector);
  void readGuides(WPXInputStream *input, FHCollector *collector);
  void readHalftone(WPXInputStream *input, FHCollector *collector);
  void readImageFill(WPXInputStream *input, FHCollector *collector);
  void readImageImport(WPXInputStream *input, FHCollector *collector);
  void readLayer(WPXInputStream *input, FHCollector *collector);
  void readLensFill(WPXInputStream *input, FHCollector *collector);
  void readLinearFill(WPXInputStream *input, FHCollector *collector);
  void readLinePat(WPXInputStream *input, FHCollector *collector);
  void readLineTable(WPXInputStream *input, FHCollector *collector);
  void readList(WPXInputStream *input, FHCollector *collector);
  void readMasterPageDocMan(WPXInputStream *input, FHCollector *collector);
  void readMasterPageElement(WPXInputStream *input, FHCollector *collector);
  void readMasterPageLayerElement(WPXInputStream *input, FHCollector *collector);
  void readMasterPageLayerInstance(WPXInputStream *input, FHCollector *collector);
  void readMasterPageSymbolClass(WPXInputStream *input, FHCollector *collector);
  void readMasterPageSymbolInstance(WPXInputStream *input, FHCollector *collector);
  void readMDict(WPXInputStream *input, FHCollector *collector);
  void readMList(WPXInputStream *input, FHCollector *collector);
  void readMName(WPXInputStream *input, FHCollector *collector);
  void readMpObject(WPXInputStream *input, FHCollector *collector);
  void readMQuickDict(WPXInputStream *input, FHCollector *collector);
  void readMString(WPXInputStream *input, FHCollector *collector);
  void readMultiBlend(WPXInputStream *input, FHCollector *collector);
  void readMultiColorList(WPXInputStream *input, FHCollector *collector);
  void readNewBlend(WPXInputStream *input, FHCollector *collector);
  void readNewContourFill(WPXInputStream *input, FHCollector *collector);
  void readNewRadialFill(WPXInputStream *input, FHCollector *collector);
  void readOpacityFilter(WPXInputStream *input, FHCollector *collector);
  void readOval(WPXInputStream *input, FHCollector *collector);
  void readParagraph(WPXInputStream *input, FHCollector *collector);
  void readPath(WPXInputStream *input, FHCollector *collector);
  void readPathTextLineInfo(WPXInputStream *input, FHCollector *collector);
  void readPatternFill(WPXInputStream *input, FHCollector *collector);
  void readPatternLine(WPXInputStream *input, FHCollector *collector);
  void readPerspectiveEnvelope(WPXInputStream *input, FHCollector *collector);
  void readPerspectiveGrid(WPXInputStream *input, FHCollector *collector);
  void readPolygonFigure(WPXInputStream *input, FHCollector *collector);
  void readProcedure(WPXInputStream *input, FHCollector *collector);
  void readPropLst(WPXInputStream *input, FHCollector *collector);
  void readPSLine(WPXInputStream *input, FHCollector *collector);
  void readRadialFill(WPXInputStream *input, FHCollector *collector);
  void readRadialFillX(WPXInputStream *input, FHCollector *collector);
  void readRaggedFilter(WPXInputStream *input, FHCollector *collector);
  void readRectangle(WPXInputStream *input, FHCollector *collector);
  void readSketchFilter(WPXInputStream *input, FHCollector *collector);
  void readSpotColor(WPXInputStream *input, FHCollector *collector);
  void readSpotColor6(WPXInputStream *input, FHCollector *collector);
  void readStylePropLst(WPXInputStream *input, FHCollector *collector);
  void readSwfImport(WPXInputStream *input, FHCollector *collector);
  void readSymbolClass(WPXInputStream *input, FHCollector *collector);
  void readSymbolInstance(WPXInputStream *input, FHCollector *collector);
  void readSymbolLibrary(WPXInputStream *input, FHCollector *collector);
  void readTabTable(WPXInputStream *input, FHCollector *collector);
  void readTaperedFill(WPXInputStream *input, FHCollector *collector);
  void readTaperedFillX(WPXInputStream *input, FHCollector *collector);
  void readTEffect(WPXInputStream *input, FHCollector *collector);
  void readTextBlok(WPXInputStream *input, FHCollector *collector);
  void readTextColumn(WPXInputStream *input, FHCollector *collector);
  void readTextInPath(WPXInputStream *input, FHCollector *collector);
  void readTFOnPath(WPXInputStream *input, FHCollector *collector);
  void readTileFill(WPXInputStream *input, FHCollector *collector);
  void readTintColor(WPXInputStream *input, FHCollector *collector);
  void readTintColor6(WPXInputStream *input, FHCollector *collector);
  void readTransformFilter(WPXInputStream *input, FHCollector *collector);
  void readTString(WPXInputStream *input, FHCollector *collector);
  void readUString(WPXInputStream *input, FHCollector *collector);
  void readVDict(WPXInputStream *input, FHCollector *collector);
  void readVMpObj(WPXInputStream *input, FHCollector *collector);
  void readXform(WPXInputStream *input, FHCollector *collector);

  unsigned _readRecordId(WPXInputStream *input);

  unsigned _xformCalc(unsigned char var1, unsigned char var2);

  double _readCoordinate(WPXInputStream *input);

  WPXInputStream *m_input;
  FHCollector *m_collector;
  int m_version;
  std::map<unsigned short, int> m_dictionary;
  std::vector<unsigned short> m_records;
  std::vector<unsigned short>::size_type m_currentRecord;
  std::vector<long> m_offsets;
  long m_fhTailOffset;
  FHPageInfo m_pageInfo;
};

} // namespace libfreehand

#endif //  __FHRAPHICS_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
