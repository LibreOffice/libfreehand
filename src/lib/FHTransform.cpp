/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FHTransform.h"
#include "libfreehand_utils.h"


libfreehand::FHTransform::FHTransform()
  : m_m11(1.0), m_m12(0.0), m_m13(0.0),
    m_m21(0.0), m_m22(1.0), m_m23(0.0)
{
}

libfreehand::FHTransform::FHTransform(double m11, double m21, double m12, double m22, double m13, double m23)
  : m_m11(m11), m_m21(m21), m_m12(m12), m_m22(m22), m_m13(m13), m_m23(m23)
{
}

libfreehand::FHTransform::FHTransform(const FHTransform &trafo)
  : m_m11(trafo.m_m11), m_m21(trafo.m_m21), m_m12(trafo.m_m12),
    m_m22(trafo.m_m22), m_m13(trafo.m_m13), m_m23(trafo.m_m23) {}


void libfreehand::FHTransform::applyToPoint(double &x, double &y) const
{
  double tmpX = m_m11*x + m_m12*y+m_m13;
  y = m_m21*x + m_m22*y+m_m23;
  x = tmpX;
}

void libfreehand::FHTransform::applyToArc(double &rx, double &ry, double &rotation, bool &sweep, double &endx, double &endy) const
{
  // Transform the end-point, which is the easiest
  applyToPoint(endx, endy);

  // Determine whether sweep should change
  double determinant = m_m11*m_m22 - m_m12*m_m21;
  if (determinant < 0.0)
    sweep = !sweep;

  // Transform a centered ellipse

  // rx = 0 and ry = 0
  if (FH_ALMOST_ZERO(rx) && FH_ALMOST_ZERO(ry))
  {
    rotation = rx = ry = 0.0;
    return;
  }

  // rx > 0, ry = 0
  if (FH_ALMOST_ZERO(ry))
  {
    double x = m_m11*cos(rotation) + m_m12*sin(rotation);
    double y = m_m21*cos(rotation) + m_m22*sin(rotation);
    rx *= sqrt(x*x + y*y);
    if (FH_ALMOST_ZERO(rx))
    {
      rotation = rx = ry = 0;
      return;
    }
    rotation = atan2(y, x);
    return;
  }

  // rx = 0, ry > 0
  if (FH_ALMOST_ZERO(rx))
  {
    double x = -m_m11*sin(rotation) + m_m12*cos(rotation);
    double y = -m_m21*sin(rotation) + m_m22*cos(rotation);
    ry *= sqrt(x*x + y*y);
    if (FH_ALMOST_ZERO(ry))
    {
      rotation = rx = ry = 0;
      return;
    }
    rotation = atan2(y, x) - M_PI/2.0;
    return;
  }

  double m11, m12, v2, m21;
  if (!FH_ALMOST_ZERO(determinant))
  {
    m11 = ry*(m_m22*cos(rotation)- m_m21*sin(rotation));
    m12 = ry*(m_m11*sin(rotation)- m_m12*cos(rotation));
    v2 = -rx*(m_m22*sin(rotation) + m_m21*cos(rotation));
    m21 = rx*(m_m12*sin(rotation) + m_m11*cos(rotation));

    // Transformed ellipse
    double A = m11*m11 + v2*v2;
    double B = 2.0*(m11*m12 + v2*m21);
    double C = m12*m12 + m21*m21;

    // Rotate the transformed ellipse
    if (FH_ALMOST_ZERO(B))
      rotation = 0;
    else
    {
      rotation = atan2(B, A-C) / 2.0;
      double c = cos(rotation);
      double s = sin(rotation);
      double cc = c*c;
      double ss = s*s;
      double sc = B*s*c;
      B = A;
      A = B*cc + sc + C*ss;
      C = B*ss - sc + C*cc;
    }

    // Compute rx and ry from A and C
    if (!FH_ALMOST_ZERO(A) && !FH_ALMOST_ZERO(C))
    {
      double abdet = fabs(rx*ry*determinant);
      A = sqrt(fabs(A));
      C = sqrt(fabs(C));
      rx = abdet / A;
      ry = abdet / C;
      return;
    }
  }

  // Special case of a close to singular transformation
  m11 = ry*(m_m22*cos(rotation) - m_m21*sin(rotation));
  m12 = ry*(m_m12*cos(rotation) - m_m11*sin(rotation));
  v2 = rx*(m_m21*cos(rotation) + m_m22*sin(rotation));
  m21 = rx*(m_m11*cos(rotation) + m_m12*sin(rotation));

  // The result of transformation is a point
  if (FH_ALMOST_ZERO(m21*m21 + m12*m12) && FH_ALMOST_ZERO(v2*v2 + m11*m11))
  {
    rotation = rx = ry = 0;
    return;
  }
  else // The result of transformation is a line
  {
    double x = sqrt(m21*m21 + m12*m12);
    double y = sqrt(v2*v2 + m11*m11);
    if (m21*m21 + m12*m12 >= v2*v2 + m11*m11)
      y = (v2*v2 + m11*m11) / x;
    else
      x = (m21*m21 + m12*m12) / y;
    rx = sqrt(x*x + y*y);
    ry = 0;
    rotation = atan2(y, x);
  }
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
