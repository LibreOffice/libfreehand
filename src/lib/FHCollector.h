/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __FHCOLLECTOR_H__
#define __FHCOLLECTOR_H__

#include <deque>
#include <map>
#include <stack>
#include <librevenge/librevenge.h>
#include "FHCollector.h"
#include "FHTransform.h"
#include "FHTypes.h"
#include "FHPath.h"

namespace libfreehand
{

class FHCollector
{
public:
  FHCollector();
  virtual ~FHCollector();

  // collector functions
  void collectString(unsigned recordId, const librevenge::RVNGString &str);
  void collectName(unsigned recordId, const librevenge::RVNGString &str);
  void collectPath(unsigned recordId, const FHPath &path);
  void collectXform(unsigned recordId, double m11, double m21,
                    double m12, double m22, double m13, double m23);
  void collectFHTail(unsigned recordId, const FHTail &fhTail);
  void collectBlock(unsigned recordId, const FHBlock &block);
  void collectList(unsigned recordId, const FHList &lst);
  void collectLayer(unsigned recordId, const FHLayer &layer);
  void collectGroup(unsigned recordId, const FHGroup &group);
  void collectClipGroup(unsigned recordId, const FHGroup &group);
  void collectPathText(unsigned recordId, const FHPathText &group);
  void collectCompositePath(unsigned recordId, const FHCompositePath &compositePath);
  void collectTString(unsigned recordId, const std::vector<unsigned> &elements);
  void collectAGDFont(unsigned recordId, const FHAGDFont &font);
  void collectTEffect(unsigned recordId, const FHTEffect &tEffect);
  void collectParagraph(unsigned recordId, const FHParagraph &paragraph);
  void collectTabTable(unsigned recordId, const std::vector<FHTab> &tabs);
  void collectTextBlok(unsigned recordId, const std::vector<unsigned short> &characters);
  void collectTextObject(unsigned recordId, const FHTextObject &textObject);
  void collectCharProps(unsigned recordId, const FHCharProperties &charProps);
  void collectParagraphProps(unsigned recordId, const FHParagraphProperties &paragraphProps);
  void collectPropList(unsigned recordId, const FHPropList &propertyList);
  void collectDisplayText(unsigned recordId, const FHDisplayText &displayText);
  void collectGraphicStyle(unsigned recordId, const FHGraphicStyle &graphicStyle);
  void collectAttributeHolder(unsigned recordId, const FHAttributeHolder &attributeHolder);
  void collectFilterAttributeHolder(unsigned recordId, const FHFilterAttributeHolder &filterAttributeHolder);
  void collectData(unsigned recordId, const librevenge::RVNGBinaryData &data);
  void collectDataList(unsigned recordId, const FHDataList &list);
  void collectImage(unsigned recordId, const FHImageImport &image);
  void collectMultiColorList(unsigned recordId, const std::vector<FHColorStop> &colorStops);
  void collectNewBlend(unsigned recordId, const FHNewBlend &newBlend);
  void collectOpacityFilter(unsigned recordId, double opacity);
  void collectFWShadowFilter(unsigned recordId, const FWShadowFilter &filter);
  void collectFWGlowFilter(unsigned recordId, const FWGlowFilter &filter);

  void collectPageInfo(const FHPageInfo &pageInfo);

  void collectColor(unsigned recordId, const FHRGBColor &color);
  void collectTintColor(unsigned recordId, const FHTintColor &color);
  void collectBasicFill(unsigned recordId, const FHBasicFill &fill);
  void collectLensFill(unsigned recordId, const FHLensFill &fill);
  void collectLinearFill(unsigned recordId, const FHLinearFill &fill);
  void collectRadialFill(unsigned recordId, const FHRadialFill &fill);
  void collectBasicLine(unsigned recordId, const FHBasicLine &line);
  void collectPatternLine(unsigned recordId, const FHPatternLine &line);
  void collectCustomProc(unsigned recordId, const FHCustomProc &lineFill);
  void collectTileFill(unsigned recordId, const FHTileFill &fill);
  void collectPatternFill(unsigned recordId, const FHPatternFill &fill);
  void collectLinePattern(unsigned recordId, const FHLinePattern &line);
  void collectArrowPath(unsigned recordId, const FHPath &path);

  void collectSymbolClass(unsigned recordId, const FHSymbolClass &symbolClass);
  void collectSymbolInstance(unsigned recordId, const FHSymbolInstance &symbolInstance);

  void outputDrawing(librevenge::RVNGDrawingInterface *painter);

private:
  FHCollector(const FHCollector &);
  FHCollector &operator=(const FHCollector &);

  void _normalizePath(FHPath &path);
  void _normalizePoint(double &x, double &y);

  void _outputPath(const FHPath *path, librevenge::RVNGDrawingInterface *painter);
  void _outputLayer(unsigned layerId, librevenge::RVNGDrawingInterface *painter);
  void _outputGroup(const FHGroup *group, librevenge::RVNGDrawingInterface *painter);
  void _outputClipGroup(const FHGroup *group, librevenge::RVNGDrawingInterface *painter);
  void _outputCompositePath(const FHCompositePath *compositePath, librevenge::RVNGDrawingInterface *painter);
  void _outputPathText(const FHPathText *pathText, librevenge::RVNGDrawingInterface *painter);
  void _outputTextObject(const FHTextObject *textObject, librevenge::RVNGDrawingInterface *painter);
  void _outputParagraph(const FHParagraph *paragraph, librevenge::RVNGDrawingInterface *painter, unsigned &actPos, unsigned minPos, unsigned maxPos);
  void _outputTextRun(const std::vector<unsigned short> *characters, unsigned offset, unsigned length,
                      unsigned charStyleId, librevenge::RVNGDrawingInterface *painter);
  void _outputDisplayText(const FHDisplayText *displayText, librevenge::RVNGDrawingInterface *painter);
  void _outputImageImport(const FHImageImport *image, librevenge::RVNGDrawingInterface *painter);
  void _outputNewBlend(const FHNewBlend *newBlend, librevenge::RVNGDrawingInterface *painter);
  void _outputSymbolInstance(const FHSymbolInstance *symbolInstance, librevenge::RVNGDrawingInterface *painter);
  void _outputSomething(unsigned somethingId, librevenge::RVNGDrawingInterface *painter);

  void _getBBofPath(const FHPath *path,FHBoundingBox &bBox);
  void _getBBofLayer(unsigned layerId,FHBoundingBox &bBox);
  void _getBBofGroup(const FHGroup *group,FHBoundingBox &bBox);
  void _getBBofClipGroup(const FHGroup *group,FHBoundingBox &bBox);
  void _getBBofPathText(const FHPathText *pathText,FHBoundingBox &bBox);
  void _getBBofCompositePath(const FHCompositePath *compositePath,FHBoundingBox &bBox);
  void _getBBofTextObject(const FHTextObject *textObject,FHBoundingBox &bBox);
  void _getBBofDisplayText(const FHDisplayText *displayText,FHBoundingBox &bBox);
  void _getBBofImageImport(const FHImageImport *image,FHBoundingBox &bBox);
  void _getBBofNewBlend(const FHNewBlend *newBlend,FHBoundingBox &bBox);
  void _getBBofSymbolInstance(const FHSymbolInstance *symbolInstance,FHBoundingBox &bBox);
  void _getBBofSomething(unsigned somethingId,FHBoundingBox &bBox);

  const std::vector<unsigned> *_findListElements(unsigned id);
  void _appendParagraphProperties(librevenge::RVNGPropertyList &propList, unsigned paraPropsId);
  void _appendParagraphProperties(librevenge::RVNGPropertyList &propList, const FH3ParaProperties &paraProps);
  void _appendCharacterProperties(librevenge::RVNGPropertyList &propList, unsigned charPropsId);
  void _appendCharacterProperties(librevenge::RVNGPropertyList &propList, const FH3CharProperties &charProps);
  void _appendFontProperties(librevenge::RVNGPropertyList &propList, unsigned agdFontId);
  void _appendTabProperties(librevenge::RVNGPropertyList &propList, const FHTab &tab);
  void _appendFillProperties(librevenge::RVNGPropertyList &propList, unsigned graphicStyleId);
  void _appendStrokeProperties(librevenge::RVNGPropertyList &propList, unsigned graphicStyleId);
  void _appendBasicFill(librevenge::RVNGPropertyList &propList, const FHBasicFill *basicFill);
  void _appendBasicLine(librevenge::RVNGPropertyList &propList, const FHBasicLine *basicLine);
  void _appendPatternLine(librevenge::RVNGPropertyList &propList, const FHPatternLine *basicLine);
  void _appendCustomProcFill(librevenge::RVNGPropertyList &propList, const FHCustomProc *customProc);
  void _appendCustomProcLine(librevenge::RVNGPropertyList &propList, const FHCustomProc *customProc);
  void _appendLinearFill(librevenge::RVNGPropertyList &propList, const FHLinearFill *linearFill);
  void _appendLensFill(librevenge::RVNGPropertyList &propList, const FHLensFill *lensFill);
  void _appendRadialFill(librevenge::RVNGPropertyList &propList, const FHRadialFill *radialFill);
  void _appendTileFill(librevenge::RVNGPropertyList &propList, const FHTileFill *tileFill);
  void _appendPatternFill(librevenge::RVNGPropertyList &propList, const FHPatternFill *patternFill);
  void _appendLinePattern(librevenge::RVNGPropertyList &propList, const FHLinePattern *linePattern);
  void _appendArrowPath(librevenge::RVNGPropertyList &propList, const FHPath *arrow, bool startArrow);
  void _appendOpacity(librevenge::RVNGPropertyList &propList, const double *opacity);
  void _appendShadow(librevenge::RVNGPropertyList &propList, const FWShadowFilter *filter);
  void _appendGlow(librevenge::RVNGPropertyList &propList, const FWGlowFilter *filter);
  void _applyFilter(librevenge::RVNGPropertyList &propList, unsigned filterId);
  const std::vector<unsigned> *_findTStringElements(unsigned id);

  const FHPath *_findPath(unsigned id);
  const FHGroup *_findGroup(unsigned id);
  const FHGroup *_findClipGroup(unsigned id);
  const FHCompositePath *_findCompositePath(unsigned id);
  const FHPathText *_findPathText(unsigned id);
  const FHTextObject *_findTextObject(unsigned id);
  const FHTransform *_findTransform(unsigned id);
  const FHTEffect *_findTEffect(unsigned id);
  const FHParagraph *_findParagraph(unsigned id);
  const std::vector<FHTab> *_findTabTable(unsigned id);
  const FHPropList *_findPropList(unsigned id);
  const FHGraphicStyle *_findGraphicStyle(unsigned id);
  const std::vector<unsigned short> *_findTextBlok(unsigned id);
  const FHBasicFill *_findBasicFill(unsigned id);
  const FHLinearFill *_findLinearFill(unsigned id);
  const FHLensFill *_findLensFill(unsigned id);
  const FHRadialFill *_findRadialFill(unsigned id);
  const FHTileFill *_findTileFill(unsigned id);
  const FHPatternFill *_findPatternFill(unsigned id);
  const FHLinePattern *_findLinePattern(unsigned id);
  const FHPath *_findArrowPath(unsigned id);
  const FHBasicLine *_findBasicLine(unsigned id);
  const FHCustomProc *_findCustomProc(unsigned id);
  const FHPatternLine *_findPatternLine(unsigned id);
  const FHRGBColor *_findRGBColor(unsigned id);
  const FHTintColor *_findTintColor(unsigned id);
  const FHDisplayText *_findDisplayText(unsigned id);
  const FHImageImport *_findImageImport(unsigned id);
  const FHNewBlend *_findNewBlend(unsigned id);
  const double *_findOpacityFilter(unsigned id);
  const FWShadowFilter *_findFWShadowFilter(unsigned id);
  const FWGlowFilter *_findFWGlowFilter(unsigned id);
  const FHFilterAttributeHolder *_findFilterAttributeHolder(unsigned id);
  const librevenge::RVNGBinaryData *_findData(unsigned id);
  librevenge::RVNGString getColorString(unsigned id, double tint=1);
  unsigned _findFillId(const FHGraphicStyle &graphicStyle);
  unsigned _findStrokeId(const FHGraphicStyle &graphicStyle);
  const FHFilterAttributeHolder *_findFilterAttributeHolder(const FHGraphicStyle &graphicStyle);
  unsigned _findValueFromAttribute(unsigned id);
  const FHSymbolClass *_findSymbolClass(unsigned id);
  const FHSymbolInstance *_findSymbolInstance(unsigned id);
  unsigned _findContentId(unsigned graphicStyleId);
  const std::vector<FHColorStop> *_findMultiColorList(unsigned id);
  librevenge::RVNGBinaryData getImageData(unsigned id);
  FHRGBColor getRGBFromTint(const FHTintColor &tint);
  void _generateBitmapFromPattern(librevenge::RVNGBinaryData &bitmap, unsigned colorId, const std::vector<unsigned char> &pattern);

  FHPageInfo m_pageInfo;
  FHTail m_fhTail;
  std::pair<unsigned, FHBlock> m_block;
  std::map<unsigned, FHTransform> m_transforms;
  std::map<unsigned, FHPath> m_paths;
  std::map<unsigned, librevenge::RVNGString> m_strings;
  std::map<librevenge::RVNGString, unsigned> m_names;
  std::map<unsigned, FHList> m_lists;
  std::map<unsigned, FHLayer> m_layers;
  std::map<unsigned, FHGroup> m_groups;
  std::map<unsigned, FHGroup> m_clipGroups;
  std::stack<FHTransform> m_currentTransforms;
  std::vector<FHTransform> m_fakeTransforms;
  std::map<unsigned, FHCompositePath> m_compositePaths;
  std::map<unsigned, FHPathText> m_pathTexts;
  std::map<unsigned, std::vector<unsigned> > m_tStrings;
  std::map<unsigned, FHAGDFont> m_fonts;
  std::map<unsigned, FHTEffect> m_tEffects;
  std::map<unsigned, FHParagraph> m_paragraphs;
  std::map<unsigned, std::vector<FHTab> > m_tabs;
  std::map<unsigned, std::vector<unsigned short> > m_textBloks;
  std::map<unsigned, FHTextObject> m_textObjects;
  std::map<unsigned, FHCharProperties> m_charProperties;
  std::map<unsigned, FHParagraphProperties> m_paragraphProperties;
  std::map<unsigned, FHRGBColor> m_rgbColors;
  std::map<unsigned, FHBasicFill> m_basicFills;
  std::map<unsigned, FHPropList> m_propertyLists;
  std::map<unsigned, FHBasicLine> m_basicLines;
  std::map<unsigned, FHCustomProc> m_customProcs;
  std::map<unsigned, FHPatternLine> m_patternLines;
  std::map<unsigned, FHDisplayText> m_displayTexts;
  std::map<unsigned, FHGraphicStyle> m_graphicStyles;
  std::map<unsigned, FHAttributeHolder> m_attributeHolders;
  std::map<unsigned, librevenge::RVNGBinaryData> m_data;
  std::map<unsigned, FHDataList> m_dataLists;
  std::map<unsigned, FHImageImport> m_images;
  std::map<unsigned, std::vector<FHColorStop> > m_multiColorLists;
  std::map<unsigned, FHLinearFill> m_linearFills;
  std::map<unsigned, FHTintColor> m_tints;
  std::map<unsigned, FHLensFill> m_lensFills;
  std::map<unsigned, FHRadialFill> m_radialFills;
  std::map<unsigned, FHNewBlend> m_newBlends;
  std::map<unsigned, FHFilterAttributeHolder> m_filterAttributeHolders;
  std::map<unsigned, double> m_opacityFilters;
  std::map<unsigned, FWShadowFilter> m_shadowFilters;
  std::map<unsigned, FWGlowFilter> m_glowFilters;
  std::map<unsigned, FHTileFill> m_tileFills;
  std::map<unsigned, FHSymbolClass> m_symbolClasses;
  std::map<unsigned, FHSymbolInstance> m_symbolInstances;
  std::map<unsigned, FHPatternFill> m_patternFills;
  std::map<unsigned, FHLinePattern> m_linePatterns;
  std::map<unsigned, FHPath> m_arrowPaths;

  unsigned m_strokeId;
  unsigned m_fillId;
  unsigned m_contentId;
  unsigned m_textBoxNumberId;
  std::deque<unsigned> m_visitedObjects;
};

} // namespace libfreehand

#endif /* __FHCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
