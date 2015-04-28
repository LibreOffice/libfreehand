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

#include <vector>
#include "FHPath.h"

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
  double m_startX;
  double m_startY;
  double m_width;
  double m_height;

  FHTextObject()
    : m_graphicStyleId(0), m_xFormId(0), m_tStringId(0), m_vmpObjId(0),
      m_startX(0.0), m_startY(0.0), m_width(0.0), m_height(0.0) {}
};

struct FHParaProperties
{
};

struct FHCharProperties
{
  unsigned m_textColorId;
  double m_fontSize;
  unsigned m_fontNameId;
  unsigned m_fontId;
  double m_horizontalScale;
  FHCharProperties()
    : m_textColorId(0), m_fontSize(12.0), m_fontNameId(0), m_fontId(0), m_horizontalScale(1.0) {}
};

struct FHRGBColor
{
  unsigned short m_red;
  unsigned short m_green;
  unsigned short m_blue;
  FHRGBColor()
    : m_red(0), m_green(0), m_blue(0) {}
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

struct FHBasicFill
{
  unsigned m_colorId;
  FHBasicFill() : m_colorId(0) {}
};

} // namespace libfreehand

#endif /* __FHTYPES_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
