/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <librevenge/librevenge.h>
#include "FHCollector.h"
#include "libfreehand_utils.h"

libfreehand::FHCollector::FHCollector() :
  m_pageInfo(), m_fhTail(), m_block(), m_transforms(), m_paths(), m_strings(), m_lists(),
  m_layers(), m_groups(), m_currentTransforms()
{
}

libfreehand::FHCollector::~FHCollector()
{
}

void libfreehand::FHCollector::collectPageInfo(const FHPageInfo &pageInfo)
{
  m_pageInfo = pageInfo;
}

void libfreehand::FHCollector::collectString(unsigned recordId, const librevenge::RVNGString &str)
{
  m_strings[recordId] = str;
}

void libfreehand::FHCollector::collectPath(unsigned recordId, unsigned /* graphicStyle */, unsigned /* layer */,
                                           const libfreehand::FHPath &path, bool /* evenOdd */)
{
  if (path.empty())
    return;

  m_paths[recordId] = path;
}

void libfreehand::FHCollector::collectXform(unsigned recordId,
                                            double m11, double m21, double m12, double m22, double m13, double m23)
{
  m_transforms[recordId] = FHTransform(m11, m21, m12, m22, m13, m23);
}

void libfreehand::FHCollector::collectFHTail(unsigned /* recordId */, unsigned blockId, unsigned propLstId, unsigned fontId)
{
  m_fhTail.m_blockId = blockId;
  m_fhTail.m_propLstId = propLstId;
  m_fhTail.m_fontId = fontId;
}

void libfreehand::FHCollector::collectBlock(unsigned recordId, const libfreehand::FHBlock &block)
{
  if (m_block.first && m_block.first != recordId)
  {
    FH_DEBUG_MSG(("FHCollector::collectBlock -- WARNING: Several \"Block\" records in the file\n"));
  }
  m_block = std::make_pair(recordId, block);
}

void libfreehand::FHCollector::collectList(unsigned recordId, const libfreehand::FHList &lst)
{
  m_lists[recordId] = lst;
}

void libfreehand::FHCollector::collectLayer(unsigned recordId, const libfreehand::FHLayer &layer)
{
  m_layers[recordId] = layer;
}

void libfreehand::FHCollector::collectGroup(unsigned recordId, const libfreehand::FHGroup &group)
{
  m_groups[recordId] = group;
}

void libfreehand::FHCollector::_normalizePath(libfreehand::FHPath &path)
{
  FHTransform trafo(1.0, 0.0, 0.0, -1.0, - m_pageInfo.m_minX, m_pageInfo.m_maxY);
  path.transform(trafo);
}

void libfreehand::FHCollector::_outputPath(const libfreehand::FHPath &path, ::librevenge::RVNGDrawingInterface *painter)
{
  if (path.empty())
    return;

  librevenge::RVNGPropertyList propList;
  propList.insert("draw:fill", "none");
  propList.insert("draw:stroke", "solid");
  propList.insert("svg:stroke-width", 0.0);
  propList.insert("svg:stroke-color", "#000000");
  painter->setStyle(propList);

  FHPath fhPath(path);
  unsigned short xform = fhPath.getXFormId();

  if (xform)
  {
    std::map<unsigned, FHTransform>::const_iterator iter = m_transforms.find(xform);
    if (iter != m_transforms.end())
      fhPath.transform(iter->second);
  }
  std::stack<FHTransform> groupTransforms = m_currentTransforms;
  while (!groupTransforms.empty())
  {
    fhPath.transform(groupTransforms.top());
    groupTransforms.pop();
  }
  _normalizePath(fhPath);

  librevenge::RVNGPropertyListVector propVec;
  fhPath.writeOut(propVec);

  librevenge::RVNGPropertyList pList;
  pList.insert("svg:d", propVec);
  painter->drawPath(pList);
}

void libfreehand::FHCollector::_outputGroup(const libfreehand::FHGroup &group, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter)
    return;

  if (group.m_xFormId)
  {
    std::map<unsigned, FHTransform>::const_iterator iterTransform = m_transforms.find(group.m_xFormId);
    if (iterTransform != m_transforms.end())
      m_currentTransforms.push(iterTransform->second);
    else
      m_currentTransforms.push(libfreehand::FHTransform());
  }
  else
    m_currentTransforms.push(libfreehand::FHTransform());

  std::vector<unsigned> elements;
  if (!_findListElements(elements, group.m_elementsId))
  {
    FH_DEBUG_MSG(("ERROR: The pointed element list does not exist\n"));
    return;
  }

  if (!elements.empty())
  {
    painter->openGroup(::librevenge::RVNGPropertyList());
    for (std::vector<unsigned>::const_iterator iterVec = elements.begin(); iterVec != elements.end(); ++iterVec)
    {
      std::map<unsigned, FHGroup>::const_iterator iterGroup = m_groups.find(*iterVec);
      if (iterGroup != m_groups.end())
      {
        _outputGroup(iterGroup->second, painter);
      }
      else
      {
        std::map<unsigned, FHPath>::const_iterator iter = m_paths.find(*iterVec);
        if (iter != m_paths.end())
        {
          _outputPath(iter->second, painter);
        }
      }
    }
    painter->closeGroup();
  }

  if (!m_currentTransforms.empty())
    m_currentTransforms.pop();
}

void libfreehand::FHCollector::outputContent(::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter)
    return;

  if (!m_fhTail.m_blockId || m_fhTail.m_blockId != m_block.first)
  {
    FH_DEBUG_MSG(("WARNING: FHTail points to an invalid Block ID\n"));
    m_fhTail.m_blockId = m_block.first;
  }
  if (!m_fhTail.m_blockId)
  {
    FH_DEBUG_MSG(("ERROR: Block record is absent from this file\n"));
    return;
  }

  painter->startDocument(librevenge::RVNGPropertyList());
  librevenge::RVNGPropertyList propList;
  propList.insert("svg:height", m_pageInfo.m_maxY - m_pageInfo.m_minY);
  propList.insert("svg:width", m_pageInfo.m_maxX - m_pageInfo.m_minX);
  painter->startPage(propList);

  unsigned layerListId = m_block.second.m_layerListId;

  std::vector<unsigned> elements;
  if (_findListElements(elements, layerListId))
  {
    for (std::vector<unsigned>::const_iterator iter = elements.begin(); iter != elements.end(); ++iter)
    {
      _outputLayer(*iter, painter);
    }
  }
  painter->endPage();
  painter->endDocument();
}

void libfreehand::FHCollector::_outputLayer(unsigned layerId, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter)
    return;

  std::map<unsigned, FHLayer>::const_iterator layerIter = m_layers.find(layerId);
  if (layerIter == m_layers.end())
  {
    FH_DEBUG_MSG(("ERROR: Could not find the referenced layer\n"));
    return;
  }

  if (layerIter->second.m_visibility != 3)
    return;

  unsigned layerElementsListId = layerIter->second.m_elementsId;
  if (!layerElementsListId)
  {
    FH_DEBUG_MSG(("ERROR: Layer points to invalid element list\n"));
    return;
  }

  std::vector<unsigned> elements;
  if (!_findListElements(elements, layerElementsListId))
  {
    FH_DEBUG_MSG(("ERROR: The pointed element list does not exist\n"));
    return;
  }

  for (std::vector<unsigned>::const_iterator iterVec = elements.begin(); iterVec != elements.end(); ++iterVec)
  {
    std::map<unsigned, FHGroup>::const_iterator iterGroup = m_groups.find(*iterVec);
    if (iterGroup != m_groups.end())
    {
      _outputGroup(iterGroup->second, painter);
    }
    else
    {
      std::map<unsigned, FHPath>::const_iterator iter = m_paths.find(*iterVec);
      if (iter != m_paths.end())
      {
        _outputPath(iter->second, painter);
      }
    }
  }
}

bool libfreehand::FHCollector::_findListElements(std::vector<unsigned> &elements, unsigned id)
{
  std::map<unsigned, FHList>::const_iterator iter = m_lists.find(id);
  if (iter == m_lists.end())
    return false;
  elements = iter->second.m_elements;
  return true;
}



/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
