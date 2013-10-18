/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __FHTYPES_H__
#define __FHTYPES_H__

namespace libfreehand
{

struct FHPageInfo
{
  double m_minX;
  double m_minY;
  double m_maxX;
  double m_maxY;
  FHPageInfo() : m_minX(0.0), m_minY(0.0), m_maxX(0.0), m_maxY(0.0) {}
};

} // namespace libfreehand

#endif /* __FHTYPES_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
