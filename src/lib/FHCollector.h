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
  FHCollector(::libwpg::WPGPaintInterface *painter);
  virtual ~FHCollector();

  // collector functions
  void collectUString(unsigned recordId, const std::vector<unsigned short> &ustr);
  void collectMName(unsigned recordId, const WPXString &name);
  void collectPath(unsigned recordId, unsigned short graphicStyle,
                   const std::vector<std::vector<std::pair<double, double> > > &path,
                   bool evenOdd, bool closed);
  void collectXform(unsigned recordId, double m11, double m21,
                    double m12, double m22,  double m13, double m23);

private:
  FHCollector(const FHCollector &);
  FHCollector &operator=(const FHCollector &);

  // helper functions

  libwpg::WPGPaintInterface *m_painter;
};

} // namespace libfreehand

#endif /* __FHCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
