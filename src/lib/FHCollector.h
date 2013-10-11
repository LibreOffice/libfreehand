/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __FHCOLLECTOR_H__
#define __FHCOLLECTOR_H__

#include <vector>
#include <libwpd/libwpd.h>

namespace libwpg
{

class WPGPaintInterface;

} // namespace libwpg

namespace libfreehand
{

class FHCollector
{
public:
  FHCollector() {}
  virtual ~FHCollector() {}

  // collector functions
  virtual void collectUString(unsigned recordId, const std::vector<unsigned short> &ustr) = 0;
  virtual void collectMName(unsigned recordId, const WPXString &name) = 0;
  virtual void collectPath(unsigned recordId, unsigned short graphicStyle,
                           const std::vector<std::vector<std::pair<double, double> > > &path,
                           bool evenOdd, bool closed) = 0;
  virtual void collectXform(unsigned recordId, double m11, double m21,
                            double m12, double m22,  double m13, double m23) = 0;
  virtual void collectOval(unsigned recordId, unsigned short graphicStyle, unsigned short layer,
                           unsigned short xform, double x, double y, double w, double h,
                           double arc1, double arc2, bool closed) = 0;
  virtual void collectRectangle(unsigned recordId, unsigned short graphicStyle, unsigned short layer,
                                unsigned short xform, double x1, double y1, double x2, double y2) = 0;

private:
  FHCollector(const FHCollector &);
  FHCollector &operator=(const FHCollector &);
};

} // namespace libfreehand

#endif /* __FHCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
