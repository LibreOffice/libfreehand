/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <libwpg/libwpg.h>
#include "FHStylesCollector.h"

libfreehand::FHStylesCollector::FHStylesCollector(libfreehand::FHPageInfo &pageInfo)
  : m_pageInfo(pageInfo)
  , m_minX(0)
  , m_minY(0)
  , m_maxX(0)
  , m_maxY(0)
{
}

libfreehand::FHStylesCollector::~FHStylesCollector()
{
}

void libfreehand::FHStylesCollector::collectOffsetX(double offsetX)
{
  m_minX = offsetX / 72.0;
  if (m_pageInfo.m_minX > 0.0)
  {
    if (m_pageInfo.m_minX > m_minX)
      m_pageInfo.m_minX = m_minX;
  }
  else
    m_pageInfo.m_minX = m_minX;
}

void libfreehand::FHStylesCollector::collectOffsetY(double offsetY)
{
  m_minY = offsetY / 72.0;
  if (m_pageInfo.m_minY > 0.0)
  {
    if (m_pageInfo.m_minY > m_minY)
      m_pageInfo.m_minY = m_minY;
  }
  else
    m_pageInfo.m_minY = m_minY;
}

void libfreehand::FHStylesCollector::collectPageWidth(double pageWidth)
{
  m_maxX = m_minX + pageWidth / 72.0;
  if (m_pageInfo.m_maxX < m_maxX)
    m_pageInfo.m_maxX = m_maxX;
}

void libfreehand::FHStylesCollector::collectPageHeight(double pageHeight)
{
  m_maxY = m_minY + pageHeight / 72.0;
  if (m_pageInfo.m_maxY < m_maxY)
    m_pageInfo.m_maxY = m_maxY;
}


/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
