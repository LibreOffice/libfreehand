/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __FREEHANDDOCUMENT_H__
#define __FREEHANDDOCUMENT_H__

#include <libwpd/libwpd.h>
#include <libwpg/libwpg.h>
#include "FHStringVector.h"

class WPXInputStream;

namespace libfreehand
{
class FreeHandDocument
{
public:

  static bool isSupported(WPXInputStream *input);

  static bool parse(WPXInputStream *input, libwpg::WPGPaintInterface *painter);

  static bool generateSVG(::WPXInputStream *input, FHStringVector &output);
};

} // namespace libfreehand

#endif //  __FHRAPHICS_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
