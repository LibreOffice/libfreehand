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

#include <vector>
#include <librevenge/librevenge.h>

namespace libfreehand
{

class FHTransform;

class FHPathElement
{
public:
  FHPathElement() {}
  virtual ~FHPathElement() {}
  virtual void writeOut(librevenge::RVNGPropertyListVector &vec) const = 0;
  virtual void transform(const FHTransform &trafo) = 0;
  virtual FHPathElement *clone() = 0;
};


class FHPath : public FHPathElement
{
public:
  FHPath() : m_elements(), m_isClosed(false) {}
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

  void writeOut(librevenge::RVNGPropertyListVector &vec) const;
  void transform(const FHTransform &trafo);
  FHPathElement *clone();

  void clear();
  bool empty() const;
  bool isClosed() const;

private:
  std::vector<FHPathElement *> m_elements;
  bool m_isClosed;
};

} // namespace libfreehand

#endif /* __FHPATH_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
