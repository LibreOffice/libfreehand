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

libfreehand::FHCollector::FHCollector() :
  m_pageInfo(), m_transforms(), m_paths(), m_uStrings(), m_mNames(), m_fhTailBlockId(0)
{
}

libfreehand::FHCollector::~FHCollector()
{
}

void libfreehand::FHCollector::collectUString(unsigned /* recordId */, const librevenge::RVNGString & /* str */)
{
}

void libfreehand::FHCollector::collectMName(unsigned /* recordId */, const librevenge::RVNGString & /* name */)
{
}

void libfreehand::FHCollector::collectPath(unsigned recordId, unsigned short /* graphicStyle */,
                                           unsigned short /* layer */, const libfreehand::FHPath &path, bool /* evenOdd */)
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

void libfreehand::FHCollector::collectFHTail(unsigned /* recordId */, unsigned blockId, unsigned /* propLstId */, unsigned /* fontId */)
{
  m_fhTailBlockId = blockId;
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
  _normalizePath(fhPath);

  librevenge::RVNGPropertyListVector propVec;
  fhPath.writeOut(propVec);

  librevenge::RVNGPropertyList pList;
  pList.insert("svg:d", propVec);
  painter->drawPath(pList);
}

void libfreehand::FHCollector::collectPageInfo(const FHPageInfo &pageInfo)
{
  m_pageInfo = pageInfo;
}

void libfreehand::FHCollector::outputContent(::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter)
    return;

  painter->startDocument(librevenge::RVNGPropertyList());
  librevenge::RVNGPropertyList propList;
  propList.insert("svg:height", m_pageInfo.m_maxY - m_pageInfo.m_minY);
  propList.insert("svg:width", m_pageInfo.m_maxX - m_pageInfo.m_minX);
  painter->startPage(propList);
  for (std::map<unsigned, FHPath>::const_iterator iter = m_paths.begin(); iter != m_paths.end(); ++iter)
  {
    _outputPath(iter->second, painter);
  }
  painter->endPage();
  painter->endDocument();
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
