/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cstdarg>
#include <cstdio>

#include <unicode/utf8.h>
#include <unicode/utf16.h>
#include "libfreehand_utils.h"

namespace
{

static const unsigned _macRomanCharacterMap[] =
{
  0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
  0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
  0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
  0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
  0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
  0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
  0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
  0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
  0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
  0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x0020,
  0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1,
  0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8,
  0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3,
  0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc,
  0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df,
  0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8,
  0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211,
  0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x00e6, 0x00f8,
  0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab,
  0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153,
  0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca,
  0x00ff, 0x0178, 0x2044, 0x20ac, 0x2039, 0x203a, 0xfb01, 0xfb02,
  0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1,
  0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4,
  0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc,
  0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7
};

}

#ifdef DEBUG
void libfreehand::debugPrint(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  va_end(args);
}
#endif

uint8_t libfreehand::readU8(librevenge::RVNGInputStream *input)
{
  if (!input || input->isEnd())
  {
    FH_DEBUG_MSG(("Throwing EndOfStreamException\n"));
    throw EndOfStreamException();
  }
  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint8_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint8_t))
    return *(uint8_t const *)(p);
  FH_DEBUG_MSG(("Throwing EndOfStreamException\n"));
  throw EndOfStreamException();
}

int8_t libfreehand::readS8(librevenge::RVNGInputStream *input)
{
  return (int8_t)readU8(input);
}

uint16_t libfreehand::readU16(librevenge::RVNGInputStream *input)
{
  if (!input || input->isEnd())
  {
    FH_DEBUG_MSG(("Throwing EndOfStreamException\n"));
    throw EndOfStreamException();
  }
  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint16_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint16_t))
    return (uint16_t)p[1]|((uint16_t)p[0]<<8);

  FH_DEBUG_MSG(("Throwing EndOfStreamException\n"));
  throw EndOfStreamException();
}

int16_t libfreehand::readS16(librevenge::RVNGInputStream *input)
{
  return (int16_t)readU16(input);
}

uint32_t libfreehand::readU32(librevenge::RVNGInputStream *input)
{
  if (!input || input->isEnd())
  {
    FH_DEBUG_MSG(("Throwing EndOfStreamException\n"));
    throw EndOfStreamException();
  }
  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint32_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint32_t))
    return (uint32_t)p[3]|((uint32_t)p[2]<<8)
           |((uint32_t)p[1]<<16)|((uint32_t)p[0]<<24);

  FH_DEBUG_MSG(("Throwing EndOfStreamException\n"));
  throw EndOfStreamException();
}

int32_t libfreehand::readS32(librevenge::RVNGInputStream *input)
{
  return (int32_t)readU32(input);
}

unsigned long libfreehand::getRemainingLength(librevenge::RVNGInputStream *const input)
{
  if (!input || input->tell() < 0)
    throw EndOfStreamException();

  const long begin = input->tell();

  if (input->seek(0, librevenge::RVNG_SEEK_END) != 0)
  {
    // librevenge::RVNG_SEEK_END does not work. Use the harder way.
    while (!input->isEnd())
      readU8(input);
  }
  const long end = input->tell();

  if (input->seek(begin, librevenge::RVNG_SEEK_SET) != 0)
    throw EndOfStreamException();

  if (end < begin)
    throw EndOfStreamException();
  return static_cast<unsigned long>(end - begin);
}

void libfreehand::_appendUTF16(librevenge::RVNGString &text, std::vector<unsigned short> &characters)
{
  if (characters.empty())
    return;

  const unsigned short *s = &characters[0];
  int j = 0;
  int length = characters.size();

  while (j < length)
  {
    UChar32 c;
    U16_NEXT(s, j, length, c)
    unsigned char outbuf[U8_MAX_LENGTH+1];
    int i = 0;
    U8_APPEND_UNSAFE(&outbuf[0], i, c);
    outbuf[i] = 0;

    text.append((char *)outbuf);
  }
}

void libfreehand::writeU16(librevenge::RVNGBinaryData &buffer, const int value)
{
  buffer.append((unsigned char)(value & 0xFF));
  buffer.append((unsigned char)((value >> 8) & 0xFF));
}

void libfreehand::writeU32(librevenge::RVNGBinaryData &buffer, const int value)
{
  buffer.append((unsigned char)(value & 0xFF));
  buffer.append((unsigned char)((value >> 8) & 0xFF));
  buffer.append((unsigned char)((value >> 16) & 0xFF));
  buffer.append((unsigned char)((value >> 24) & 0xFF));
}

void libfreehand::_appendMacRoman(librevenge::RVNGString &text, unsigned char character)
{
  if (character < 0x20)
    text.append((char)character);
  else
  {
    /* Mapping of Apple's MacRoman character set in Unicode */

    UChar32 c = _macRomanCharacterMap[character - 0x20];
    unsigned char outbuf[U8_MAX_LENGTH+1];
    int i = 0;
    U8_APPEND_UNSAFE(&outbuf[0], i, c);
    outbuf[i] = 0;

    text.append((char *)outbuf);
  }
}


/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
