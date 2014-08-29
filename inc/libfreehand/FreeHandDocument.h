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

#include <librevenge/librevenge.h>

#ifdef DLL_EXPORT
#ifdef LIBFREEHAND_BUILD
#define FHAPI __declspec(dllexport)
#else
#define FHAPI __declspec(dllimport)
#endif
#else // !DLL_EXPORT
#ifdef LIBFREEHAND_VISIBILITY
#define FHAPI __attribute__((visibility("default")))
#else
#define FHAPI
#endif
#endif

namespace libfreehand
{
class FreeHandDocument
{
public:

  static FHAPI bool isSupported(librevenge::RVNGInputStream *input);

  static FHAPI bool parse(librevenge::RVNGInputStream *input, librevenge::RVNGDrawingInterface *painter);
};

} // namespace libfreehand

#endif //  __FHRAPHICS_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
