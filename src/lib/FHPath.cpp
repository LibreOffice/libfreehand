/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <math.h>
#include <map>
#include "FHPath.h"
#include "FHTypes.h"
#include "FHTransform.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef DEBUG_SPLINES
#define DEBUG_SPLINES 0
#endif

namespace libfreehand
{

class FHMoveToElement : public FHPathElement
{
public:
  FHMoveToElement(double x, double y)
    : m_x(x),
      m_y(y) {}
  ~FHMoveToElement() {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const;
  void transform(const FHTransform &trafo);
  FHPathElement *clone();
private:
  double m_x;
  double m_y;
};

class FHLineToElement : public FHPathElement
{
public:
  FHLineToElement(double x, double y)
    : m_x(x),
      m_y(y) {}
  ~FHLineToElement() {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const;
  void transform(const FHTransform &trafo);
  FHPathElement *clone();
private:
  double m_x;
  double m_y;
};

class FHCubicBezierToElement : public FHPathElement
{
public:
  FHCubicBezierToElement(double x1, double y1, double x2, double y2, double x, double y)
    : m_x1(x1),
      m_y1(y1),
      m_x2(x2),
      m_y2(y2),
      m_x(x),
      m_y(y) {}
  ~FHCubicBezierToElement() {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const;
  void transform(const FHTransform &trafo);
  FHPathElement *clone();
private:
  double m_x1;
  double m_y1;
  double m_x2;
  double m_y2;
  double m_x;
  double m_y;
};

class FHQuadraticBezierToElement : public FHPathElement
{
public:
  FHQuadraticBezierToElement(double x1, double y1, double x, double y)
    : m_x1(x1),
      m_y1(y1),
      m_x(x),
      m_y(y) {}
  ~FHQuadraticBezierToElement() {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const;
  void transform(const FHTransform &trafo);
  FHPathElement *clone();
private:
  double m_x1;
  double m_y1;
  double m_x;
  double m_y;
};

class FHArcToElement : public FHPathElement
{
public:
  FHArcToElement(double rx, double ry, double rotation, bool largeArc, bool sweep, double x, double y)
    : m_rx(rx),
      m_ry(ry),
      m_rotation(rotation),
      m_largeArc(largeArc),
      m_sweep(sweep),
      m_x(x),
      m_y(y) {}
  ~FHArcToElement() {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const;
  void transform(const FHTransform &trafo);
  FHPathElement *clone();
private:
  double m_rx;
  double m_ry;
  double m_rotation;
  bool m_largeArc;
  bool m_sweep;
  double m_x;
  double m_y;
};

} // namespace libfreehand


void libfreehand::FHMoveToElement::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  librevenge::RVNGPropertyList node;
  node.insert("librevenge:path-action", "M");
  node.insert("svg:x", m_x);
  node.insert("svg:y", m_y);
  vec.append(node);
}

void libfreehand::FHMoveToElement::transform(const FHTransform &trafo)
{
  trafo.applyToPoint(m_x,m_y);
}

libfreehand::FHPathElement *libfreehand::FHMoveToElement::clone()
{
  return new FHMoveToElement(m_x, m_y);
}

void libfreehand::FHLineToElement::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  librevenge::RVNGPropertyList node;
  node.insert("librevenge:path-action", "L");
  node.insert("svg:x", m_x);
  node.insert("svg:y", m_y);
  vec.append(node);
}

void libfreehand::FHLineToElement::transform(const FHTransform &trafo)
{
  trafo.applyToPoint(m_x,m_y);
}

libfreehand::FHPathElement *libfreehand::FHLineToElement::clone()
{
  return new FHLineToElement(m_x, m_y);
}

void libfreehand::FHCubicBezierToElement::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  librevenge::RVNGPropertyList node;
  node.insert("librevenge:path-action", "C");
  node.insert("svg:x1", m_x1);
  node.insert("svg:y1", m_y1);
  node.insert("svg:x2", m_x2);
  node.insert("svg:y2", m_y2);
  node.insert("svg:x", m_x);
  node.insert("svg:y", m_y);
  vec.append(node);
}

void libfreehand::FHCubicBezierToElement::transform(const FHTransform &trafo)
{
  trafo.applyToPoint(m_x1,m_y1);
  trafo.applyToPoint(m_x2,m_y2);
  trafo.applyToPoint(m_x,m_y);
}

libfreehand::FHPathElement *libfreehand::FHCubicBezierToElement::clone()
{
  return new FHCubicBezierToElement(m_x1, m_y1, m_x2, m_y2, m_x, m_y);
}

void libfreehand::FHQuadraticBezierToElement::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  librevenge::RVNGPropertyList node;
  node.insert("librevenge:path-action", "Q");
  node.insert("svg:x1", m_x1);
  node.insert("svg:y1", m_y1);
  node.insert("svg:x", m_x);
  node.insert("svg:y", m_y);
  vec.append(node);
}

void libfreehand::FHQuadraticBezierToElement::transform(const FHTransform &trafo)
{
  trafo.applyToPoint(m_x1,m_y1);
  trafo.applyToPoint(m_x,m_y);
}

libfreehand::FHPathElement *libfreehand::FHQuadraticBezierToElement::clone()
{
  return new FHQuadraticBezierToElement(m_x1, m_y1, m_x, m_y);
}

void libfreehand::FHArcToElement::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  librevenge::RVNGPropertyList node;
  node.insert("librevenge:path-action", "A");
  node.insert("svg:rx", m_rx);
  node.insert("svg:ry", m_ry);
  node.insert("librevenge:rotate", m_rotation * 180 / M_PI, librevenge::RVNG_GENERIC);
  node.insert("librevenge:large-arc", m_largeArc);
  node.insert("librevenge:sweep", m_sweep);
  node.insert("svg:x", m_x);
  node.insert("svg:y", m_y);
  vec.append(node);
}

void libfreehand::FHArcToElement::transform(const FHTransform &trafo)
{
  trafo.applyToArc(m_rx, m_ry, m_rotation, m_sweep, m_x, m_y);
}

libfreehand::FHPathElement *libfreehand::FHArcToElement::clone()
{
  return new FHArcToElement(m_rx, m_ry, m_rotation, m_largeArc, m_sweep, m_x, m_y);
}

void libfreehand::FHPath::appendMoveTo(double x, double y)
{
  m_elements.push_back(new libfreehand::FHMoveToElement(x, y));
}

void libfreehand::FHPath::appendLineTo(double x, double y)
{
  m_elements.push_back(new libfreehand::FHLineToElement(x, y));
}

void libfreehand::FHPath::appendCubicBezierTo(double x1, double y1, double x2, double y2, double x, double y)
{
  m_elements.push_back(new libfreehand::FHCubicBezierToElement(x1, y1, x2, y2, x, y));
}

void libfreehand::FHPath::appendQuadraticBezierTo(double x1, double y1, double x, double y)
{
  m_elements.push_back(new libfreehand::FHQuadraticBezierToElement(x1, y1, x, y));
}

void libfreehand::FHPath::appendArcTo(double rx, double ry, double rotation, bool longAngle, bool sweep, double x, double y)
{
  m_elements.push_back(new libfreehand::FHArcToElement(rx, ry, rotation, longAngle, sweep, x, y));
}

void libfreehand::FHPath::appendClosePath()
{
  m_isClosed = true;
}

libfreehand::FHPath::FHPath(const libfreehand::FHPath &path) : m_elements(), m_isClosed(path.m_isClosed), m_xFormId(path.m_xFormId), m_graphicStyleId(path.m_graphicStyleId)
{
  for (std::vector<FHPathElement *>::const_iterator iter = path.m_elements.begin(); iter != path.m_elements.end(); ++iter)
    m_elements.push_back((*iter)->clone());
}

libfreehand::FHPath &libfreehand::FHPath::operator=(const libfreehand::FHPath &path)
{
  // Check for self-assignment
  if (this == &path)
    return *this;
  clear();
  for (std::vector<FHPathElement *>::const_iterator iter = path.m_elements.begin(); iter != path.m_elements.end(); ++iter)
    m_elements.push_back((*iter)->clone());
  m_isClosed = path.m_isClosed;
  m_xFormId = path.m_xFormId;
  m_graphicStyleId = path.m_graphicStyleId;
  return *this;
}


libfreehand::FHPath::~FHPath()
{
  clear();
}

void libfreehand::FHPath::appendPath(const FHPath &path)
{
  for (std::vector<FHPathElement *>::const_iterator iter = path.m_elements.begin(); iter != path.m_elements.end(); ++iter)
    m_elements.push_back((*iter)->clone());
}

void libfreehand::FHPath::setXFormId(unsigned xFormId)
{
  m_xFormId = xFormId;
}

void libfreehand::FHPath::setGraphicStyleId(unsigned graphicStyleId)
{
  m_graphicStyleId = graphicStyleId;
}

void libfreehand::FHPath::setEvenOdd(bool evenOdd)
{
  m_evenOdd = evenOdd;
}

void libfreehand::FHPath::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  for (std::vector<FHPathElement *>::const_iterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
    (*iter)->writeOut(vec);
}

void libfreehand::FHPath::transform(const FHTransform &trafo)
{
  for (std::vector<FHPathElement *>::iterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
    (*iter)->transform(trafo);
}

libfreehand::FHPathElement *libfreehand::FHPath::clone()
{
  return new FHPath(*this);
}

void libfreehand::FHPath::clear()
{
  for (std::vector<FHPathElement *>::iterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
    if (*iter)
      delete(*iter);
  m_elements.clear();
  m_isClosed = false;
  m_xFormId = 0;
  m_graphicStyleId = 0;
}

bool libfreehand::FHPath::empty() const
{
  return m_elements.empty();
}

bool libfreehand::FHPath::isClosed() const
{
  return m_isClosed;
}

unsigned libfreehand::FHPath::getXFormId() const
{
  return m_xFormId;
}

unsigned libfreehand::FHPath::getGraphicStyleId() const
{
  return m_graphicStyleId;
}

bool libfreehand::FHPath::getEvenOdd() const
{
  return m_evenOdd;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
