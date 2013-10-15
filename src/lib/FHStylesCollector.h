/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __FHSTYLESCOLLECTOR_H__
#define __FHSTYLESCOLLECTOR_H__

#include "FHCollector.h"
#include "FHTypes.h"

namespace libfreehand
{

class FHStylesCollector : public FHCollector
{
public:
  FHStylesCollector(FHPageInfo &pageInfo);
  ~FHStylesCollector();

  // collector functions
  void collectUString(unsigned, const std::vector<unsigned short> &) {}
  void collectMName(unsigned, const WPXString &) {}
  void collectPath(unsigned, unsigned short, const FHPath &, bool) {}
  void collectPath(unsigned, unsigned short, unsigned short,
                   unsigned short, const FHPath &) {}
  void collectXform(unsigned, double, double, double, double,  double, double) {}

  void collectOffsetX(double offsetX);
  void collectOffsetY(double offsetY);
  void collectPageWidth(double pageWidth);
  void collectPageHeight(double pageHeight);

private:
  FHStylesCollector(const FHStylesCollector &);
  FHStylesCollector &operator=(const FHStylesCollector &);

  FHPageInfo &m_pageInfo;

};

} // namespace libfreehand

#endif /* __FHSTYLESCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
