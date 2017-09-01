/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __FHTYPES_H__
#define __FHTYPES_H__

#include <float.h>
#include <vector>
#include <map>
#include "FHPath.h"
#include "FHTransform.h"

namespace libfreehand
{

struct FHPageInfo
{
  double m_minX;
  double m_minY;
  double m_maxX;
  double m_maxY;
  FHPageInfo() : m_minX(0.0), m_minY(0.0), m_maxX(0.0), m_maxY(0.0) {}
};

struct FHBlock
{
  unsigned m_layerListId;
  FHBlock() : m_layerListId(0) {}
  FHBlock(unsigned layerListId) : m_layerListId(layerListId) {}
};

struct FHTab
{
  unsigned m_type;
  double m_position;
  FHTab() : m_type(0), m_position(0) {}
};

struct FHTail
{
  unsigned m_blockId;
  unsigned m_propLstId;
  unsigned m_fontId;
  FHPageInfo m_pageInfo;
  FHTail() : m_blockId(0), m_propLstId(0), m_fontId(0), m_pageInfo() {}
};

struct FHList
{
  unsigned m_listType;
  std::vector<unsigned> m_elements;
  FHList() : m_listType(0), m_elements() {}
};

struct FHLayer
{
  unsigned m_graphicStyleId;
  unsigned m_elementsId;
  unsigned m_visibility;
  FHLayer() : m_graphicStyleId(0), m_elementsId(0), m_visibility(0) {}
};

struct FHGroup
{
  unsigned m_graphicStyleId;
  unsigned m_elementsId;
  unsigned m_xFormId;
  FHGroup() : m_graphicStyleId(0), m_elementsId(0), m_xFormId(0) {}
};

struct FHPathText
{
  unsigned m_elementsId;
  unsigned m_layerId;
  unsigned m_displayTextId;
  unsigned m_shapeId;
  unsigned m_textSize;
  FHPathText() : m_elementsId(0), m_layerId(0), m_displayTextId(0), m_shapeId(0), m_textSize(0) {}
};

struct FHCompositePath
{
  unsigned m_graphicStyleId;
  unsigned m_elementsId;
  FHCompositePath() : m_graphicStyleId(0), m_elementsId(0) {}
};

struct FHParagraph
{
  unsigned m_paraStyleId;
  unsigned m_textBlokId;
  std::vector<std::pair<unsigned, unsigned> > m_charStyleIds;
  FHParagraph() : m_paraStyleId(0), m_textBlokId(0), m_charStyleIds() {}
};

struct FHAGDFont
{
  unsigned m_fontNameId;
  unsigned m_fontStyle;
  double m_fontSize;
  FHAGDFont() : m_fontNameId(0), m_fontStyle(0), m_fontSize(12.0) {}
};

struct FHTextObject
{
  unsigned m_graphicStyleId;
  unsigned m_xFormId;
  unsigned m_tStringId;
  unsigned m_vmpObjId;
  unsigned m_pathId;
  double m_startX;
  double m_startY;
  double m_width;
  double m_height;
  unsigned m_beginPos;
  unsigned m_endPos;
  unsigned m_colNum;
  unsigned m_rowNum;
  double m_colSep;
  double m_rowSep;
  unsigned m_rowBreakFirst;

  FHTextObject()
    : m_graphicStyleId(0), m_xFormId(0), m_tStringId(0), m_vmpObjId(0), m_pathId(0),
      m_startX(0.0), m_startY(0.0), m_width(0.0), m_height(0.0), m_beginPos(0), m_endPos(0xffff),
      m_colNum(1), m_rowNum(1), m_colSep(0.0), m_rowSep(0.0), m_rowBreakFirst(0) {}
};

struct FHParagraphProperties
{
  std::map<unsigned,unsigned> m_idToIntMap; // id to enum, int map
  std::map<unsigned,double> m_idToDoubleMap;
  std::map<unsigned,unsigned> m_idToZoneIdMap;
  FHParagraphProperties() : m_idToIntMap(), m_idToDoubleMap(), m_idToZoneIdMap()
  {}
  bool empty() const
  {
    return m_idToIntMap.empty() && m_idToDoubleMap.empty() && m_idToZoneIdMap.empty();
  }
};

struct FHCharProperties
{
  unsigned m_textColorId;
  double m_fontSize;
  unsigned m_fontNameId;
  unsigned m_fontId;
  unsigned m_tEffectId;
  std::map<unsigned,double> m_idToDoubleMap;
  FHCharProperties()
    : m_textColorId(0), m_fontSize(12.0), m_fontNameId(0), m_fontId(0), m_tEffectId(0), m_idToDoubleMap() {}
};

struct FHRGBColor
{
  unsigned short m_red;
  unsigned short m_green;
  unsigned short m_blue;
  FHRGBColor()
    : m_red(0), m_green(0), m_blue(0) {}
  bool black() const
  {
    return !m_red && !m_green && !m_blue;
  }
};

struct FHCMYKColor
{
  unsigned short m_cyan;
  unsigned short m_magenta;
  unsigned short m_yellow;
  unsigned short m_black;
  FHCMYKColor()
    : m_cyan(0), m_magenta(0), m_yellow(0), m_black(0xffff) {}
};

struct FHTintColor
{
  unsigned m_baseColorId;
  unsigned short m_tint;
  FHTintColor() : m_baseColorId(0), m_tint(1.0) {}
};

struct FHPropList
{
  unsigned m_parentId;
  std::map<unsigned, unsigned> m_elements;
  FHPropList()
    : m_parentId(0), m_elements() {}
};

struct FHBasicLine
{
  unsigned m_colorId;
  unsigned m_linePatternId;
  unsigned m_startArrowId;
  unsigned m_endArrowId;
  double m_mitter;
  double m_width;
  FHBasicLine()
    : m_colorId(0), m_linePatternId(0), m_startArrowId(0),
      m_endArrowId(0), m_mitter(0.0), m_width(0.0) {}
};

struct FHPatternLine
{
  unsigned m_colorId;
  double m_percentPattern; // percentage of 1 in the pattern
  double m_mitter;
  double m_width;
  FHPatternLine()
    : m_colorId(0), m_percentPattern(1), m_mitter(0.0), m_width(0.0) {}
};

struct FHCustomProc
{
  std::vector<unsigned> m_ids;
  std::vector<double> m_widths;
  std::vector<double> m_params;
  std::vector<double> m_angles;
  FHCustomProc() : m_ids(), m_widths(), m_params(), m_angles() {}
};

struct FHBasicFill
{
  unsigned m_colorId;
  FHBasicFill() : m_colorId(0) {}
};

struct FHLinearFill
{
  unsigned m_color1Id;
  unsigned m_color2Id;
  double m_angle;
  unsigned m_multiColorListId;
  FHLinearFill() : m_color1Id(0), m_color2Id(0), m_angle(0.0), m_multiColorListId(0) {}
};

struct FHRadialFill
{
  unsigned m_color1Id;
  unsigned m_color2Id;
  double m_cx;
  double m_cy;
  unsigned m_multiColorListId;
  FHRadialFill()
    : m_color1Id(0), m_color2Id(0), m_cx(0.5), m_cy(0.5), m_multiColorListId(0) {}
};

struct FHPatternFill
{
  unsigned m_colorId;
  std::vector<unsigned char> m_pattern;
  FHPatternFill() : m_colorId(0), m_pattern(8) {}
};

struct FH3CharProperties
{
  unsigned m_offset;
  unsigned m_fontNameId;
  double m_fontSize;
  unsigned m_fontStyle;
  unsigned m_fontColorId;
  unsigned m_textEffsId;
  double m_leading; // -1 solid, -2 auto, >0 interline in point
  double m_letterSpacing;
  double m_wordSpacing;
  double m_horizontalScale;
  double m_baselineShift;
  FH3CharProperties()
    : m_offset(0), m_fontNameId(0), m_fontSize(12.0), m_fontStyle(0),
      m_fontColorId(0), m_textEffsId(0), m_leading(-1), m_letterSpacing(0), m_wordSpacing(0), m_horizontalScale(1), m_baselineShift(0.0) {}
};

struct FH3ParaProperties
{
  unsigned m_offset;
  FH3ParaProperties() : m_offset(0) {}
};

struct FHTEffect
{
  unsigned m_nameId;
  unsigned m_shortNameId;
  unsigned m_colorId[2];
  FHTEffect() : m_nameId(0), m_shortNameId(0)
  {
    for (unsigned int &i : m_colorId) i=0;
  }
};
struct FHDisplayText
{
  unsigned m_graphicStyleId;
  unsigned m_xFormId;
  double m_startX;
  double m_startY;
  double m_width;
  double m_height;
  std::vector<FH3CharProperties> m_charProps;
  int m_justify;
  std::vector<FH3ParaProperties> m_paraProps;
  std::vector<unsigned char> m_characters;
  FHDisplayText()
    : m_graphicStyleId(0), m_xFormId(0),
      m_startX(0.0), m_startY(0.0), m_width(0.0), m_height(0.0),
      m_charProps(), m_justify(0), m_paraProps(), m_characters() {}
};

struct FHGraphicStyle
{
  unsigned m_parentId;
  unsigned m_attrId;
  std::map<unsigned, unsigned> m_elements;
  FHGraphicStyle() : m_parentId(0), m_attrId(0), m_elements() {}
};

struct FHAttributeHolder
{
  unsigned m_parentId;
  unsigned m_attrId;
  FHAttributeHolder() : m_parentId(0), m_attrId(0) {}
};

struct FHFilterAttributeHolder
{
  unsigned m_parentId;
  unsigned m_filterId;
  unsigned m_graphicStyleId;
  FHFilterAttributeHolder() : m_parentId(0), m_filterId(0), m_graphicStyleId(0) {}
};

struct FHDataList
{
  unsigned m_dataSize;
  std::vector<unsigned> m_elements;
  FHDataList() : m_dataSize(0), m_elements() {}
};

struct FHImageImport
{
  unsigned m_graphicStyleId;
  unsigned m_dataListId;
  unsigned m_xFormId;
  double m_startX;
  double m_startY;
  double m_width;
  double m_height;
  librevenge::RVNGString m_format;
  FHImageImport()
    : m_graphicStyleId(0), m_dataListId(0), m_xFormId(0),
      m_startX(0.0), m_startY(0.0), m_width(0.0), m_height(0.0),
      m_format() {}
};

struct FHColorStop
{
  unsigned m_colorId;
  double m_position;
  FHColorStop() : m_colorId(0), m_position(0.0) {}
};

struct FHLensFill
{
  unsigned m_colorId;
  double m_value;
  unsigned m_mode;
  FHLensFill() : m_colorId(0), m_value(0.0), m_mode(0) {}
};

struct FHNewBlend
{
  unsigned m_graphicStyleId;
  unsigned m_parentId;
  unsigned m_list1Id;
  unsigned m_list2Id;
  unsigned m_list3Id;
  FHNewBlend() : m_graphicStyleId(0), m_parentId(0), m_list1Id(0), m_list2Id(0), m_list3Id(0) {}
};

struct FWShadowFilter
{
  unsigned m_colorId;
  bool m_knockOut;
  bool m_inner;
  double m_distribution;
  double m_opacity;
  double m_smoothness;
  double m_angle;
  FWShadowFilter()
    : m_colorId(0), m_knockOut(false), m_inner(false),
      m_distribution(0.0), m_opacity(1.0), m_smoothness(1.0), m_angle(45.0) {}
};

struct FWGlowFilter
{
  unsigned m_colorId;
  bool m_inner;
  double m_width;
  double m_opacity;
  double m_smoothness;
  double m_distribution;
  FWGlowFilter()
    : m_colorId(0), m_inner(false), m_width(0.0), m_opacity(1.0),
      m_smoothness(1.0), m_distribution(0.0) {}
};

struct FHTileFill
{
  unsigned m_xFormId;
  unsigned m_groupId;
  double m_scaleX;
  double m_scaleY;
  double m_offsetX;
  double m_offsetY;
  double m_angle;
  FHTileFill()
    : m_xFormId(0), m_groupId(0), m_scaleX(0.0), m_scaleY(0.0),
      m_offsetX(0.0), m_offsetY(0.0), m_angle(0.0) {}
};

struct FHLinePattern
{
  std::vector<double> m_dashes;
  FHLinePattern()
    : m_dashes() {}
};

struct FHSymbolClass
{
  unsigned m_nameId;
  unsigned m_groupId;
  unsigned m_dateTimeId;
  unsigned m_symbolLibraryId;
  unsigned m_listId;
  FHSymbolClass() : m_nameId(0), m_groupId(0), m_dateTimeId(0), m_symbolLibraryId(0), m_listId(0) {}
};

struct FHSymbolInstance
{
  unsigned m_graphicStyleId;
  unsigned m_parentId;
  unsigned m_symbolClassId;
  FHTransform m_xForm;
  FHSymbolInstance() : m_graphicStyleId(0), m_parentId(0), m_symbolClassId(0), m_xForm() {}
};

struct FHBoundingBox
{
  double m_xmin;
  double m_ymin;
  double m_xmax;
  double m_ymax;
  FHBoundingBox() : m_xmin(DBL_MAX), m_ymin(DBL_MAX), m_xmax(-DBL_MAX), m_ymax(-DBL_MAX) {}
  FHBoundingBox(const FHBoundingBox &bBox)
    : m_xmin(bBox.m_xmin), m_ymin(bBox.m_ymin), m_xmax(bBox.m_xmax), m_ymax(bBox.m_ymax) {}
  void merge(const FHBoundingBox &bBox)
  {
    if (m_xmin > bBox.m_xmin) m_xmin = bBox.m_xmin;
    if (m_xmin > bBox.m_xmax) m_xmin = bBox.m_xmax;
    if (m_ymin > bBox.m_ymin) m_ymin = bBox.m_ymin;
    if (m_ymin > bBox.m_ymax) m_ymin = bBox.m_ymax;
    if (m_xmax < bBox.m_xmax) m_xmax = bBox.m_xmax;
    if (m_xmax < bBox.m_xmin) m_xmax = bBox.m_xmin;
    if (m_ymax < bBox.m_ymax) m_ymax = bBox.m_ymax;
    if (m_ymax < bBox.m_ymin) m_ymax = bBox.m_ymin;
  }
  bool isValid() const
  {
    return ((m_xmin < m_xmax) && (m_ymin < m_ymax));
  }
};

} // namespace libfreehand

#endif /* __FHTYPES_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
