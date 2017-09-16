/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __LIBFREEHAND_UTILS_H__
#define __LIBFREEHAND_UTILS_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>
#include <string>
#include <math.h>

#include <boost/cstdint.hpp>

#include <librevenge/librevenge.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FH_EPSILON 1E-6
#define FH_ALMOST_ZERO(m) (fabs(m) <= FH_EPSILON)

// do nothing with debug messages in a release compile
#ifdef DEBUG

#if defined(HAVE_FUNC_ATTRIBUTE_FORMAT)
#define FH_ATTRIBUTE_PRINTF(fmt, arg) __attribute__((format(printf, fmt, arg)))
#else
#define FH_ATTRIBUTE_PRINTF(fmt, arg)
#endif

namespace libfreehand
{
void debugPrint(const char *format, ...) FH_ATTRIBUTE_PRINTF(1, 2);
}

#define FH_DEBUG_MSG(M) debugPrint M
#define FH_DEBUG(M) M

#else
#define FH_DEBUG_MSG(M)
#define FH_DEBUG(M)
#endif

namespace libfreehand
{

uint8_t readU8(librevenge::RVNGInputStream *input);
uint16_t readU16(librevenge::RVNGInputStream *input);
uint32_t readU32(librevenge::RVNGInputStream *input);
int8_t readS8(librevenge::RVNGInputStream *input);
int16_t readS16(librevenge::RVNGInputStream *input);
int32_t readS32(librevenge::RVNGInputStream *input);

unsigned long getRemainingLength(librevenge::RVNGInputStream *input);

void writeU16(librevenge::RVNGBinaryData &buffer, const int value);
void writeU32(librevenge::RVNGBinaryData &buffer, const int value);

void _appendUTF16(librevenge::RVNGString &text, std::vector<unsigned short> &characters);
void _appendMacRoman(librevenge::RVNGString &text, unsigned char character);

class EndOfStreamException
{
};

class GenericException
{
};

} // namespace libfreehand

#endif // __LIBFREEHAND_UTILS_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
