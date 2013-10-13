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
{
}

libfreehand::FHStylesCollector::~FHStylesCollector()
{
}

void libfreehand::FHStylesCollector::collectOffsetX(double offsetX)
{
  m_pageInfo.m_offsetX = offsetX;
}

void libfreehand::FHStylesCollector::collectOffsetY(double offsetY)
{
  m_pageInfo.m_offsetY = offsetY;
}

void libfreehand::FHStylesCollector::collectPageWidth(double pageWidth)
{
  m_pageInfo.m_width = pageWidth;
}

void libfreehand::FHStylesCollector::collectPageHeight(double pageHeight)
{
  m_pageInfo.m_height = pageHeight;
}


/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
