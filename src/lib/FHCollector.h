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

#include <map>
#include "FHCollector.h"
#include "FHTransform.h"
#include "FHTypes.h"
#include "FHPath.h"

namespace libfreehand
{

class FHCollector
{
public:
  FHCollector(::libwpg::WPGPaintInterface *painter, const FHPageInfo &pageInfo);
  virtual ~FHCollector();

  // collector functions
  void collectUString(unsigned recordId, const std::vector<unsigned short> &ustr);
  void collectMName(unsigned recordId, const WPXString &name);
  void collectPath(unsigned recordId, unsigned short graphicStyle, unsigned short layer,
                   unsigned short xform, const FHPath &path, bool evenodd);
  void collectXform(unsigned recordId, double m11, double m21,
                    double m12, double m22, double m13, double m23);

  void collectOffsetX(double) {}
  void collectOffsetY(double) {}
  void collectPageWidth(double) {}
  void collectPageHeight(double) {}

private:
  FHCollector(const FHCollector &);
  FHCollector &operator=(const FHCollector &);

  void _normalizePath(FHPath &path);

  libwpg::WPGPaintInterface *m_painter;
  const FHPageInfo &m_pageInfo;
  std::map<unsigned, FHTransform> m_transforms;
};

} // namespace libfreehand

#endif /* __FHCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
