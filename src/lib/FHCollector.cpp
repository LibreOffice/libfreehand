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

libfreehand::FHCollector::FHCollector(librevenge::RVNGDrawingInterface *painter, const FHPageInfo &pageInfo) :
  m_painter(painter), m_pageInfo(pageInfo), m_transforms()
{
  librevenge::RVNGPropertyList propList;
  propList.insert("svg:height", m_pageInfo.m_maxY - m_pageInfo.m_minY);
  propList.insert("svg:width", m_pageInfo.m_maxX - m_pageInfo.m_minX);
  m_painter->startDocument(librevenge::RVNGPropertyList());
  m_painter->startPage(propList);
}

libfreehand::FHCollector::~FHCollector()
{
  m_painter->endPage();
  m_painter->endDocument();
}

void libfreehand::FHCollector::collectUString(unsigned /* recordId */, const librevenge::RVNGString & /* str */)
{
}

void libfreehand::FHCollector::collectMName(unsigned /* recordId */, const librevenge::RVNGString & /* name */)
{
}

void libfreehand::FHCollector::collectPath(unsigned /* recordId */, unsigned short /* graphicStyle */,
                                           unsigned short /* layer */, unsigned short xform, const libfreehand::FHPath &path, bool /* evenOdd */)
{
  if (path.empty())
    return;
  FHPath fhPath(path);
  if (xform)
  {
    std::map<unsigned, FHTransform>::const_iterator iter = m_transforms.find(xform);
    if (iter != m_transforms.end())
      fhPath.transform(iter->second);
  }
  _normalizePath(fhPath);

  librevenge::RVNGPropertyList propList;
  propList.insert("draw:fill", "none");
  propList.insert("draw:stroke", "solid");
  propList.insert("svg:stroke-width", 0.0);
  propList.insert("svg:stroke-color", "#000000");
  m_painter->setStyle(propList);

  librevenge::RVNGPropertyListVector propVec;
  fhPath.writeOut(propVec);

  librevenge::RVNGPropertyList pList;
  pList.insert("svg:d", propVec);
  m_painter->drawPath(pList);
}

void libfreehand::FHCollector::collectXform(unsigned recordId,
                                            double m11, double m21, double m12, double m22, double m13, double m23)
{
  m_transforms[recordId] = FHTransform(m11, m21, m12, m22, m13, m23);
}

void libfreehand::FHCollector::_normalizePath(libfreehand::FHPath &path)
{
  FHTransform trafo(1.0, 0.0, 0.0, -1.0, - m_pageInfo.m_minX, m_pageInfo.m_maxY);
  path.transform(trafo);
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
