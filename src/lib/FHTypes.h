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


} // namespace libfreehand

#endif /* __FHTYPES_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
