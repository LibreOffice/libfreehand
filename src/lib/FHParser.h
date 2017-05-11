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
#include <lcms2.h>
#include <librevenge/librevenge.h>
#include "FHTypes.h"

namespace libfreehand
{

class FHCollector;

class FHParser
{
public:
  explicit FHParser();
  virtual ~FHParser();
  bool parse(librevenge::RVNGInputStream *input, librevenge::RVNGDrawingInterface *painter);
private:
  FHParser(const FHParser &);
  FHParser &operator=(const FHParser &);

  void parseDictionary(librevenge::RVNGInputStream *input);
  void parseRecordList(librevenge::RVNGInputStream *input);
  void parseRecord(librevenge::RVNGInputStream *input, FHCollector *collector, int recordId);
  void parseRecords(librevenge::RVNGInputStream *input, FHCollector *collector);
  void parseDocument(librevenge::RVNGInputStream *input, FHCollector *collector);

  void readAGDFont(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readAGDSelection(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readArrowPath(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readAttributeHolder(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readBasicFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readBasicLine(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readBendFilter(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readBlendObject(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readBlock(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readBrush(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readBrushStroke(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readBrushTip(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readCalligraphicStroke(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readCharacterFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readClipGroup(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readCollector(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readColor6(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readCompositePath(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readConeFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readConnectorLine(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readContentFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readContourFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readCustomProc(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readDataList(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readData(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readDateTime(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readDisplayText(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readDuetFilter(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readElement(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readElemList(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readElemPropLst(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readEnvelope(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readExpandFilter(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readExtrusion(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readFHDocHeader(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readFHTail(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readFigure(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readFileDescriptor(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readFilterAttributeHolder(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readFWBevelFilter(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readFWBlurFilter(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readFWFeatherFilter(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readFWGlowFilter(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readFWShadowFilter(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readFWSharpenFilter(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readGradientMaskFilter(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readGraphicStyle(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readGroup(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readGuides(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readHalftone(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readImageFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readImageImport(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readImport(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readLayer(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readLensFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readLinearFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readLinePat(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readLineTable(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readList(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readMasterPageDocMan(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readMasterPageElement(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readMasterPageLayerElement(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readMasterPageLayerInstance(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readMasterPageSymbolClass(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readMasterPageSymbolInstance(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readMDict(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readMName(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readMpObject(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readMQuickDict(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readMString(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readMultiBlend(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readMultiColorList(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readNewBlend(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readNewContourFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readNewRadialFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readOpacityFilter(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readOval(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readPantoneColor(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readParagraph(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readPath(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readPathText(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readPathTextLineInfo(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readPatternFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readPatternLine(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readPerspectiveEnvelope(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readPerspectiveGrid(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readPolygonFigure(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readProcedure(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readProcessColor(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readPropLst(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readPSFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readPSLine(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readRadialFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readRadialFillX(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readRaggedFilter(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readRectangle(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readSketchFilter(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readSpotColor(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readSpotColor6(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readStylePropLst(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readSwfImport(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readSymbolClass(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readSymbolInstance(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readSymbolLibrary(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readTabTable(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readTaperedFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readTaperedFillX(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readTEffect(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readTextBlok(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readTextEffs(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readTextObject(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readTileFill(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readTintColor(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readTintColor6(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readTransformFilter(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readTString(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readUString(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readVDict(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readVMpObj(librevenge::RVNGInputStream *input, FHCollector *collector);
  void readXform(librevenge::RVNGInputStream *input, FHCollector *collector);

  unsigned _readRecordId(librevenge::RVNGInputStream *input);

  unsigned _xformCalc(unsigned char var1, unsigned char var2);

  double _readCoordinate(librevenge::RVNGInputStream *input);
  FHRGBColor _readRGBColor(librevenge::RVNGInputStream *input);
  FHRGBColor _readCMYKColor(librevenge::RVNGInputStream *input);
  void _readPropLstElements(librevenge::RVNGInputStream *input, std::map<unsigned, unsigned> &properties, unsigned size);
  void _readBlockInformation(librevenge::RVNGInputStream *input, unsigned i, unsigned &layerListId);
  void _readFH3CharProperties(librevenge::RVNGInputStream *input, FH3CharProperties &charProps);
  void _readFH3ParaProperties(librevenge::RVNGInputStream *input, FH3ParaProperties &paraProps);

  librevenge::RVNGInputStream *m_input;
  FHCollector *m_collector;
  int m_version;
  std::map<unsigned short, int> m_dictionary;
  std::vector<unsigned short> m_records;
  std::vector<unsigned short>::size_type m_currentRecord;
  FHPageInfo m_pageInfo;
  cmsHTRANSFORM m_colorTransform;
};

} // namespace libfreehand

#endif //  __FHPARSER_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
