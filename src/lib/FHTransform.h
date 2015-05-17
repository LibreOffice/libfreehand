/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __FHTRANSFORM_H__
#define __FHTRANSFORM_H__

#include <math.h>

namespace libfreehand
{

struct FHTransform
{
  FHTransform();
  FHTransform(double m11, double m21, double m12, double m22, double m13, double m23);
  FHTransform(const FHTransform &trafo);

  void applyToPoint(double &x, double &y) const;
  void applyToArc(double &rx, double &ry, double &rotation, bool &sweep, double &endx, double &endy) const;

  double m_m11;
  double m_m21;
  double m_m12;
  double m_m22;
  double m_m13;
  double m_m23;
};

} // namespace libfreehand

#endif /* __FHTRANSFORM_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
