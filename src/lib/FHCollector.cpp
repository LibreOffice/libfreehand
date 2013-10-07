/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FHCollector.h"

libfreehand::FHCollector::FHCollector(libwpg::WPGPaintInterface *painter) :
  m_painter(painter)
{
}

libfreehand::FHCollector::~FHCollector()
{
}

void libfreehand::FHCollector::collectUString(unsigned /* recordId */, const std::vector<unsigned short> & /* ustr */)
{
}

void libfreehand::FHCollector::collectMName(unsigned /* recordId */, const WPXString & /* name */)
{
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
