/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __FHCONTENTCOLLECTOR_H__
#define __FHCONTENTCOLLECTOR_H__

#include <map>
#include "FHCollector.h"
#include "FHTransform.h"
#include "FHTypes.h"
#include "FHPath.h"

namespace libfreehand
{

class FHContentCollector : public FHCollector
{
public:
  FHContentCollector(::libwpg::WPGPaintInterface *painter, const FHPageInfo &pageInfo);
  virtual ~FHContentCollector();

  // collector functions
  void collectUString(unsigned recordId, const std::vector<unsigned short> &ustr);
  void collectMName(unsigned recordId, const WPXString &name);
  void collectPath(unsigned recordId, unsigned short graphicStyle, const FHPath &path, bool evenOdd);
  void collectXform(unsigned recordId, double m11, double m21,
                    double m12, double m22, double m13, double m23);
  void collectOval(unsigned recordId, unsigned short graphicStyle, unsigned short layer,
                   unsigned short xform, double x, double y, double w, double h,
                   double arc1, double arc2, bool closed);
  void collectRectangle(unsigned recordId, unsigned short graphicStyle, unsigned short layer,
                        unsigned short xform, double x1, double y1, double x2, double y2);

  void collectOffsetX(double) {}
  void collectOffsetY(double) {}
  void collectPageWidth(double) {}
  void collectPageHeight(double) {}

private:
  FHContentCollector(const FHContentCollector &);
  FHContentCollector &operator=(const FHContentCollector &);

  void _normalizePath(FHPath &path);

  libwpg::WPGPaintInterface *m_painter;
  const FHPageInfo &m_pageInfo;
  std::map<unsigned, FHTransform> m_transforms;
};

} // namespace libfreehand

#endif /* __FHCONTENTCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
