/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a coymin of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <math.h>
#include <map>
#include <sstream>

#include "FHPath.h"
#include "FHTypes.h"
#include "FHTransform.h"
#include "libfreehand_utils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef DEBUG_SPLINES
#define DEBUG_SPLINES 0
#endif

namespace
{

static double getAngle(double bx, double by)
{
  return fmod(2*M_PI + (by > 0.0 ? 1.0 : -1.0) * acos(bx / sqrt(bx * bx + by * by)), 2*M_PI);
}

static void getEllipticalArcBBox(double x0, double y0,
                                 double rx, double ry, double phi, bool largeArc, bool sweep, double x, double y,
                                 double &xmin, double &ymin, double &xmax, double &ymax)
{
  phi *= M_PI/180;
  if (rx < 0.0)
    rx *= -1.0;
  if (ry < 0.0)
    ry *= -1.0;

  double const absError=1e-5;
  if ((rx>-absError && rx<absError) || (ry>-absError && ry<absError))
  {
    xmin = (x0 < x ? x0 : x);
    xmax = (x0 > x ? x0 : x);
    ymin = (y0 < y ? y0 : y);
    ymax = (y0 > y ? y0 : y);
    return;
  }

  // F.6.5.1
  const double x1prime = cos(phi)*(x0 - x)/2 + sin(phi)*(y0 - y)/2;
  const double y1prime = -sin(phi)*(x0 - x)/2 + cos(phi)*(y0 - y)/2;

  // F.6.5.2
  double radicant = (rx*rx*ry*ry - rx*rx*y1prime*y1prime - ry*ry*x1prime*x1prime)/(rx*rx*y1prime*y1prime + ry*ry*x1prime*x1prime);
  double cxprime = 0.0;
  double cyprime = 0.0;
  if (radicant < 0.0)
  {
    double ratio = rx/ry;
    radicant = y1prime*y1prime + x1prime*x1prime/(ratio*ratio);
    if (radicant < 0.0)
    {
      xmin = (x0 < x ? x0 : x);
      xmax = (x0 > x ? x0 : x);
      ymin = (y0 < y ? y0 : y);
      ymax = (y0 > y ? y0 : y);
      return;
    }
    ry=sqrt(radicant);
    rx=ratio*ry;
  }
  else
  {
    double factor = (largeArc==sweep ? -1.0 : 1.0)*sqrt(radicant);

    cxprime = factor*rx*y1prime/ry;
    cyprime = -factor*ry*x1prime/rx;
  }

  // F.6.5.3
  double cx = cxprime*cos(phi) - cyprime*sin(phi) + (x0 + x)/2;
  double cy = cxprime*sin(phi) + cyprime*cos(phi) + (y0 + y)/2;

  // now compute bounding box of the whole ellipse

  // Parametric equation of an ellipse:
  // x(theta) = cx + rx*cos(theta)*cos(phi) - ry*sin(theta)*sin(phi)
  // y(theta) = cy + rx*cos(theta)*sin(phi) + ry*sin(theta)*cos(phi)

  // Compute local extrems
  // 0 = -rx*sin(theta)*cos(phi) - ry*cos(theta)*sin(phi)
  // 0 = -rx*sin(theta)*sin(phi) - ry*cos(theta)*cos(phi)

  // Local extrems for X:
  // theta = -atan(ry*tan(phi)/rx)
  // and
  // theta = M_PI -atan(ry*tan(phi)/rx)

  // Local extrems for Y:
  // theta = atan(ry/(tan(phi)*rx))
  // and
  // theta = M_PI + atan(ry/(tan(phi)*rx))

  double txmin, txmax, tymin, tymax;

  // First handle special cases
  if ((phi > -absError&&phi < absError) || (phi > M_PI-absError && phi < M_PI+absError))
  {
    xmin = cx - rx;
    txmin = getAngle(-rx, 0);
    xmax = cx + rx;
    txmax = getAngle(rx, 0);
    ymin = cy - ry;
    tymin = getAngle(0, -ry);
    ymax = cy + ry;
    tymax = getAngle(0, ry);
  }
  else if ((phi > M_PI / 2.0-absError && phi < M_PI / 2.0+absError) ||
           (phi > 3.0*M_PI/2.0-absError && phi < 3.0*M_PI/2.0+absError))
  {
    xmin = cx - ry;
    txmin = getAngle(-ry, 0);
    xmax = cx + ry;
    txmax = getAngle(ry, 0);
    ymin = cy - rx;
    tymin = getAngle(0, -rx);
    ymax = cy + rx;
    tymax = getAngle(0, rx);
  }
  else
  {
    txmin = -atan(ry*tan(phi)/rx);
    txmax = M_PI - atan(ry*tan(phi)/rx);
    xmin = cx + rx*cos(txmin)*cos(phi) - ry*sin(txmin)*sin(phi);
    xmax = cx + rx*cos(txmax)*cos(phi) - ry*sin(txmax)*sin(phi);
    double tmpY = cy + rx*cos(txmin)*sin(phi) + ry*sin(txmin)*cos(phi);
    txmin = getAngle(xmin - cx, tmpY - cy);
    tmpY = cy + rx*cos(txmax)*sin(phi) + ry*sin(txmax)*cos(phi);
    txmax = getAngle(xmax - cx, tmpY - cy);

    tymin = atan(ry/(tan(phi)*rx));
    tymax = atan(ry/(tan(phi)*rx))+M_PI;
    ymin = cy + rx*cos(tymin)*sin(phi) + ry*sin(tymin)*cos(phi);
    ymax = cy + rx*cos(tymax)*sin(phi) + ry*sin(tymax)*cos(phi);
    double tmpX = cx + rx*cos(tymin)*cos(phi) - ry*sin(tymin)*sin(phi);
    tymin = getAngle(tmpX - cx, ymin - cy);
    tmpX = cx + rx*cos(tymax)*cos(phi) - ry*sin(tymax)*sin(phi);
    tymax = getAngle(tmpX - cx, ymax - cy);
  }
  if (xmin > xmax)
  {
    std::swap(xmin,xmax);
    std::swap(txmin,txmax);
  }
  if (ymin > ymax)
  {
    std::swap(ymin,ymax);
    std::swap(tymin,tymax);
  }
  double angle1 = getAngle(x0 - cx, y0 - cy);
  double angle2 = getAngle(x - cx, y - cy);

  // for sweep == 0 it is normal to have delta theta < 0
  // but we don't care about the rotation direction for bounding box
  if (!sweep)
    std::swap(angle1, angle2);

  // We cannot check directly for whether an angle is included in
  // an interval of angles that cross the 360/0 degree boundary
  // So here we will have to check for their absence in the complementary
  // angle interval
  bool otherArc = false;
  if (angle1 > angle2)
  {
    std::swap(angle1, angle2);
    otherArc = true;
  }

  // Check txmin
  if ((!otherArc && (angle1 > txmin || angle2 < txmin)) || (otherArc && !(angle1 > txmin || angle2 < txmin)))
    xmin = x0 < x ? x0 : x;
  // Check txmax
  if ((!otherArc && (angle1 > txmax || angle2 < txmax)) || (otherArc && !(angle1 > txmax || angle2 < txmax)))
    xmax = x0 > x ? x0 : x;
  // Check tymin
  if ((!otherArc && (angle1 > tymin || angle2 < tymin)) || (otherArc && !(angle1 > tymin || angle2 < tymin)))
    ymin = y0 < y ? y0 : y;
  // Check tymax
  if ((!otherArc && (angle1 > tymax || angle2 < tymax)) || (otherArc && !(angle1 > tymax || angle2 < tymax)))
    ymax = y0 > y ? y0 : y;
}

static double quadraticExtreme(double t, double a, double b, double c)
{
  return (1.0-t)*(1.0-t)*a + 2.0*(1.0-t)*t*b + t*t*c;
}

static double quadraticDerivative(double a, double b, double c)
{
  double denominator = a - 2.0*b + c;
  if (fabs(denominator)>1e-10*(a-b))
    return (a - b)/denominator;
  return -1.0;
}

static double cubicBase(double t, double a, double b, double c, double d)
{
  return (1.0-t)*(1.0-t)*(1.0-t)*a + 3.0*(1.0-t)*(1.0-t)*t*b + 3.0*(1.0-t)*t*t*c + t*t*t*d;
}

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}

namespace libfreehand
{

class FHMoveToElement : public FHPathElement
{
public:
  FHMoveToElement(double x, double y)
    : m_x(x),
      m_y(y) {}
  ~FHMoveToElement() override {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const override;
  void writeOut(std::ostream &o) const override;
  void transform(const FHTransform &trafo) override;
  FHPathElement *clone() override;
  void getBoundingBox(double x0, double y0, double &xmin, double &ymin, double &xmax, double &ymax) const override;
  double getX() const override
  {
    return m_x;
  }
  double getY() const override
  {
    return m_y;
  }
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
  ~FHLineToElement() override {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const override;
  void writeOut(std::ostream &o) const override;
  void transform(const FHTransform &trafo) override;
  FHPathElement *clone() override;
  void getBoundingBox(double x0, double y0, double &xmin, double &ymin, double &xmax, double &ymax) const override;
  double getX() const override
  {
    return m_x;
  }
  double getY() const override
  {
    return m_y;
  }
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
  ~FHCubicBezierToElement() override {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const override;
  void writeOut(std::ostream &o) const override;
  void transform(const FHTransform &trafo) override;
  FHPathElement *clone() override;
  void getBoundingBox(double x0, double y0, double &xmin, double &ymin, double &xmax, double &ymax) const override;
  double getX() const override
  {
    return m_x;
  }
  double getY() const override
  {
    return m_y;
  }
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
  ~FHQuadraticBezierToElement() override {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const override;
  void writeOut(std::ostream &o) const override;
  void transform(const FHTransform &trafo) override;
  FHPathElement *clone() override;
  void getBoundingBox(double x0, double y0, double &xmin, double &ymin, double &xmax, double &ymax) const override;
  double getX() const override
  {
    return m_x;
  }
  double getY() const override
  {
    return m_y;
  }
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
  ~FHArcToElement() override {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const override;
  void writeOut(std::ostream &o) const override;
  void transform(const FHTransform &trafo) override;
  FHPathElement *clone() override;
  void getBoundingBox(double x0, double y0, double &xmin, double &ymin, double &xmax, double &ymax) const override;
  double getX() const override
  {
    return m_x;
  }
  double getY() const override
  {
    return m_y;
  }
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

void libfreehand::FHMoveToElement::writeOut(std::ostream &o) const
{
  o << "M " << int(35*m_x) << " " << int(35*m_y);
}

void libfreehand::FHMoveToElement::transform(const FHTransform &trafo)
{
  trafo.applyToPoint(m_x,m_y);
}

libfreehand::FHPathElement *libfreehand::FHMoveToElement::clone()
{
  return new FHMoveToElement(m_x, m_y);
}

void libfreehand::FHMoveToElement::getBoundingBox(double x0, double y0, double &xmin, double &ymin, double &xmax, double &ymax) const
{
  if (x0 < xmin) xmin = x0;
  if (m_x < xmin) xmin = m_x;

  if (y0 < ymin) ymin = y0;
  if (m_y < ymin) ymin = m_y;

  if (x0 > xmax) xmax = x0;
  if (m_x > xmax) xmax = m_x;

  if (y0 > ymax) ymax = y0;
  if (m_y > ymax) ymax = m_y;
}

void libfreehand::FHLineToElement::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  librevenge::RVNGPropertyList node;
  node.insert("librevenge:path-action", "L");
  node.insert("svg:x", m_x);
  node.insert("svg:y", m_y);
  vec.append(node);
}

void libfreehand::FHLineToElement::writeOut(std::ostream &o) const
{
  o << "L " << int(35*m_x) << " " << int(35*m_y);
}

void libfreehand::FHLineToElement::transform(const FHTransform &trafo)
{
  trafo.applyToPoint(m_x,m_y);
}

libfreehand::FHPathElement *libfreehand::FHLineToElement::clone()
{
  return new FHLineToElement(m_x, m_y);
}

void libfreehand::FHLineToElement::getBoundingBox(double x0, double y0, double &xmin, double &ymin, double &xmax, double &ymax) const
{
  if (x0 < xmin) xmin = x0;
  if (m_x < xmin) xmin = m_x;

  if (y0 < ymin) ymin = y0;
  if (m_y < ymin) ymin = m_y;

  if (x0 > xmax) xmax = x0;
  if (m_x > xmax) xmax = m_x;

  if (y0 > ymax) ymax = y0;
  if (m_y > ymax) ymax = m_y;
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

void libfreehand::FHCubicBezierToElement::writeOut(std::ostream &o) const
{
  o << "C " << int(35*m_x1) << " " << int(35*m_y1) << " "
    << int(35*m_x2) << " " << int(35*m_y2) << " " << int(35*m_x) << " " << int(35*m_y);
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

void libfreehand::FHCubicBezierToElement::getBoundingBox(double x0, double y0, double &xmin, double &ymin, double &xmax, double &ymax) const
{
  if (x0 < xmin) xmin = x0;
  if (m_x < xmin) xmin = m_x;

  if (y0 < ymin) ymin = y0;
  if (m_y < ymin) ymin = m_y;

  if (x0 > xmax) xmax = x0;
  if (m_x > xmax) xmax = m_x;

  if (y0 > ymax) ymax = y0;
  if (m_y > ymax) ymax = m_y;

  for (int i=0; i<=100; ++i)
  {
    double t=double(i)/100.;
    double tmpx = cubicBase(t, x0, m_x1, m_x2, m_x);
    if (tmpx < xmin) xmin = tmpx;
    if (tmpx > xmax) xmax = tmpx;
    double tmpy = cubicBase(t, y0, m_y1, m_y2, m_y);
    if (tmpy < ymin) ymin = tmpy;
    if (tmpy > ymax) ymax = tmpy;
  }
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

void libfreehand::FHQuadraticBezierToElement::writeOut(std::ostream &o) const
{
  o << "Q " << int(35*m_x1) << " " << int(35*m_y1) << " " << int(35*m_x) << " " << int(35*m_y);
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

void libfreehand::FHQuadraticBezierToElement::getBoundingBox(double x0, double y0, double &xmin, double &ymin, double &xmax, double &ymax) const
{
  if (x0 < xmin) xmin = x0;
  if (m_x < xmin) xmin = m_x;

  if (y0 < ymin) ymin = y0;
  if (m_y < ymin) ymin = m_y;

  if (x0 > xmax) xmax = x0;
  if (m_x > xmax) xmax = m_x;

  if (y0 > ymax) ymax = y0;
  if (m_y > ymax) ymax = m_y;

  double t = quadraticDerivative(x0, m_x1, m_x);
  if (t>=0 && t<=1)
  {
    double tmpx = quadraticExtreme(t, x0, m_x1, m_x);
    if (xmin > tmpx) xmin = tmpx;
    if (xmax < tmpx) xmax = tmpx;
  }

  t = quadraticDerivative(y0, m_y1, m_y);
  if (t>=0 && t<=1)
  {
    double tmpy = quadraticExtreme(t, y0, m_y1, m_y);
    if (ymin > tmpy) ymin = tmpy;
    if (ymax < tmpy) ymax = tmpy;
  }
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

void libfreehand::FHArcToElement::writeOut(std::ostream &o) const
{
  o << "A " << int(35*m_rx) << " " << int(35*m_ry) << " "
    << int(m_rotation * 180 / M_PI) << " " << m_largeArc << " " << m_sweep << " "
    << int(35*m_x) << " " << int(35*m_y);
}

void libfreehand::FHArcToElement::transform(const FHTransform &trafo)
{
  trafo.applyToArc(m_rx, m_ry, m_rotation, m_sweep, m_x, m_y);
}

libfreehand::FHPathElement *libfreehand::FHArcToElement::clone()
{
  return new FHArcToElement(m_rx, m_ry, m_rotation, m_largeArc, m_sweep, m_x, m_y);
}

void libfreehand::FHArcToElement::getBoundingBox(double x0, double y0, double &xmin, double &ymin, double &xmax, double &ymax) const
{
  double tmpXMin = m_x < x0 ? m_x : x0;
  double tmpXMax = m_x > x0 ? m_x : x0;
  double tmpYMin = m_y < y0 ? m_y : y0;
  double tmpYMax = m_y > y0 ? m_y : y0;

  getEllipticalArcBBox(x0, y0, m_rx, m_ry, m_rotation, m_largeArc, m_sweep, m_x, m_y, tmpXMin, tmpYMin, tmpXMax, tmpYMax);

  if (tmpXMin < xmin) xmin = tmpXMin;
  if (tmpXMax > xmax) xmax = tmpXMax;

  if (tmpYMin < ymin) ymin = tmpYMin;
  if (tmpYMax > ymax) ymax = tmpYMax;
}

void libfreehand::FHPath::appendMoveTo(double x, double y)
{
  m_elements.push_back(make_unique<libfreehand::FHMoveToElement>(x, y));
}

void libfreehand::FHPath::appendLineTo(double x, double y)
{
  m_elements.push_back(make_unique<libfreehand::FHLineToElement>(x, y));
}

void libfreehand::FHPath::appendCubicBezierTo(double x1, double y1, double x2, double y2, double x, double y)
{
  m_elements.push_back(make_unique<libfreehand::FHCubicBezierToElement>(x1, y1, x2, y2, x, y));
}

void libfreehand::FHPath::appendQuadraticBezierTo(double x1, double y1, double x, double y)
{
  m_elements.push_back(make_unique<libfreehand::FHQuadraticBezierToElement>(x1, y1, x, y));
}

void libfreehand::FHPath::appendArcTo(double rx, double ry, double rotation, bool longAngle, bool sweep, double x, double y)
{
  m_elements.push_back(make_unique<libfreehand::FHArcToElement>(rx, ry, rotation, longAngle, sweep, x, y));
}

void libfreehand::FHPath::appendClosePath()
{
  m_isClosed = true;
}

libfreehand::FHPath::FHPath(const libfreehand::FHPath &path)
  : m_elements(), m_isClosed(path.m_isClosed), m_xFormId(path.m_xFormId),
    m_graphicStyleId(path.m_graphicStyleId), m_evenOdd(path.m_evenOdd)
{
  appendPath(path);
}

libfreehand::FHPath &libfreehand::FHPath::operator=(const libfreehand::FHPath &path)
{
  // Check for self-assignment
  if (this == &path)
    return *this;
  clear();
  appendPath(path);
  m_isClosed = path.m_isClosed;
  m_xFormId = path.m_xFormId;
  m_graphicStyleId = path.m_graphicStyleId;
  return *this;
}


void libfreehand::FHPath::appendPath(const FHPath &path)
{
  for (const auto &element : path.m_elements)
    m_elements.push_back(std::unique_ptr<FHPathElement>(element->clone()));
}

libfreehand::FHPath::~FHPath()
{
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
  for (const auto &element : m_elements)
    element->writeOut(vec);
}

std::string libfreehand::FHPath::getPathString() const
{
  std::stringstream s;
  for (const auto &element : m_elements)
    element->writeOut(s);
  return s.str();
}

void libfreehand::FHPath::transform(const FHTransform &trafo)
{
  for (const auto &element : m_elements)
    element->transform(trafo);
}

void libfreehand::FHPath::clear()
{
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

double libfreehand::FHPath::getX() const
{
  if (empty())
    return 0.0;
  return m_elements.back()->getX();
}

double libfreehand::FHPath::getY() const
{
  if (empty())
    return 0.0;
  return m_elements.back()->getY();
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

void libfreehand::FHPath::getBoundingBox(double x0, double y0, double &xmin, double &ymin, double &xmax, double &ymax) const
{
  for (const auto &element : m_elements)
  {
    double x = element->getX();
    double y = element->getY();

    if (x0 < xmin) xmin = x0;
    if (x < xmin) xmin = x;

    if (y0 < ymin) ymin = y0;
    if (y < ymin) ymin = y;

    if (x0 > xmax) xmax = x0;
    if (x > xmax) xmax = x;

    if (y0 > ymax) ymax = y0;
    if (y > ymax) ymax = y;

    element->getBoundingBox(x0, y0, xmin, ymin, xmax, ymax);
    x0 = element->getX();
    y0 = element->getY();
  }
}

void libfreehand::FHPath::getBoundingBox(double &xmin, double &ymin, double &xmax, double &ymax) const
{
  if (m_elements.empty())
  {
    FH_DEBUG_MSG(("libfreehand::FHPath::getBoundingBox: get an empty path\n"));
    return;
  }
  double x0 = m_elements[0]->getX();
  double y0 = m_elements[0]->getY();
  xmin = xmax = x0;
  ymin = ymax = y0;
  getBoundingBox(x0, y0, xmin, ymin, xmax, ymax);
}



/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
