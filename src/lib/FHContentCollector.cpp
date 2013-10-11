/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <libwpg/libwpg.h>
#include "FHContentCollector.h"

libfreehand::FHContentCollector::FHContentCollector(libwpg::WPGPaintInterface *painter) :
  m_painter(painter), m_transforms()
{
  m_painter->startGraphics(WPXPropertyList());
}

libfreehand::FHContentCollector::~FHContentCollector()
{
  m_painter->endGraphics();
}

void libfreehand::FHContentCollector::collectUString(unsigned /* recordId */, const std::vector<unsigned short> & /* ustr */)
{
}

void libfreehand::FHContentCollector::collectMName(unsigned /* recordId */, const WPXString & /* name */)
{
}

void libfreehand::FHContentCollector::collectPath(unsigned /* recordId */, unsigned short /* graphicStyle */,
    const std::vector<std::vector<std::pair<double, double> > > &path, bool /* evenOdd */, bool closed)
{
  if (path.empty())
    return;
  WPXPropertyListVector propVec;
  WPXPropertyList propList;
  propList.insert("draw:fill", "none");
  propList.insert("draw:stroke", "solid");
  propList.insert("svg:stroke-width", 0.0);
  propList.insert("svg:stroke-color", "#000000");
  m_painter->setStyle(propList, WPXPropertyListVector());

  propList.clear();
  propList.insert("libwpg:path-action", "M");
  propList.insert("svg:x", path[0][0].first / 72.0);
  propList.insert("svg:y", (1000.0 - path[0][0].second) / 72.0);
  propVec.append(propList);
  propList.clear();
  unsigned i = 0;
  for (i = 0; i<path.size()-1; ++i)
  {
    propList.insert("libwpg:path-action", "C");
    propList.insert("svg:x1", path[i][2].first / 72.0);
    propList.insert("svg:y1", (1000.0 - path[i][2].second) / 72.0);
    propList.insert("svg:x2", path[i+1][1].first / 72.0);
    propList.insert("svg:y2", (1000.0 - path[i+1][1].second) / 72.0);
    propList.insert("svg:x", path[i+1][0].first / 72.0);
    propList.insert("svg:y", (1000.0 - path[i+1][0].second) / 72.0);
    propVec.append(propList);
    propList.clear();
  }
  if (closed)
  {
    propList.insert("libwpg:path-action", "C");
    propList.insert("svg:x1", path[i][2].first / 72.0);
    propList.insert("svg:y1", (1000.0 - path[i][2].second) / 72.0);
    propList.insert("svg:x2", path[0][1].first / 72.0);
    propList.insert("svg:y2", (1000.0 - path[0][1].second) / 72.0);
    propList.insert("svg:x", path[0][0].first / 72.0);
    propList.insert("svg:y", (1000.0 - path[0][0].second) / 72.0);
    propVec.append(propList);
    propList.clear();
    propList.insert("libwpg:path-action", "Z");
    propVec.append(propList);
  }

  m_painter->drawPath(propVec);
}

void libfreehand::FHContentCollector::collectXform(unsigned recordId,
    double m11, double m21, double m12, double m22, double m13, double m23)
{
  m_transforms[recordId] = FHTransform(m11, m21, m12, m22, m13, m23);
}

void libfreehand::FHContentCollector::collectOval(unsigned recordId,
    unsigned short graphicStyle, unsigned short layer, unsigned short xform,
    double x, double y, double w, double h,double arc1, double arc2, bool closed)
{
}

void libfreehand::FHContentCollector::collectRectangle(unsigned recordId,
    unsigned short graphicStyle, unsigned short layer, unsigned short xform,
    double x1, double y1, double x2, double y2)
{
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
