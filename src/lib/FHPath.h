/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __FHPATH_H__
#define __FHPATH_H__

#include <memory>
#include <vector>
#include <ostream>

#include <librevenge/librevenge.h>

namespace libfreehand
{

struct FHTransform;

class FHPathElement
{
public:
  FHPathElement() {}
  virtual ~FHPathElement() {}
  virtual void writeOut(librevenge::RVNGPropertyListVector &vec) const = 0;
  virtual void writeOut(std::ostream &s) const = 0;
  virtual void transform(const FHTransform &trafo) = 0;
  virtual FHPathElement *clone() = 0;
  virtual void getBoundingBox(double x0, double y0, double &px, double &py, double &qx, double &qy) const = 0;
  virtual double getX() const = 0;
  virtual double getY() const = 0;
};


class FHPath
{
public:
  FHPath() : m_elements(), m_isClosed(false), m_xFormId(0), m_graphicStyleId(0), m_evenOdd(false) {}
  FHPath(const FHPath &path);
  ~FHPath();

  FHPath &operator=(const FHPath &path);

  void appendMoveTo(double x, double y);
  void appendLineTo(double x, double y);
  void appendCubicBezierTo(double x1, double y1, double x2, double y2, double x, double y);
  void appendQuadraticBezierTo(double x1, double y1, double x, double y);
  void appendArcTo(double rx, double ry, double rotation, bool longAngle, bool sweep, double x, double y);
  void appendClosePath();
  void appendPath(const FHPath &path);
  void setXFormId(unsigned xFormId);
  void setGraphicStyleId(unsigned graphicStyleId);
  void setEvenOdd(bool evenOdd);

  void writeOut(librevenge::RVNGPropertyListVector &vec) const;
  std::string getPathString() const;
  void transform(const FHTransform &trafo);
  void getBoundingBox(double x0, double y0, double &xmin, double &ymin, double &xmax, double &ymax) const;
  double getX() const;
  double getY() const;

  void clear();
  bool empty() const;
  bool isClosed() const;
  unsigned getXFormId() const;
  unsigned getGraphicStyleId() const;
  bool getEvenOdd() const;
  void getBoundingBox(double &xmin, double &ymin, double &xmax, double &ymax) const;

private:
  std::vector<std::unique_ptr<FHPathElement>> m_elements;
  bool m_isClosed;
  unsigned m_xFormId;
  unsigned m_graphicStyleId;
  bool m_evenOdd;
};

} // namespace libfreehand

#endif /* __FHPATH_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
