/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "libfreehand_utils.h"

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
    return (uint32_t)p[3]|((uint32_t)p[2]<<8)|((uint32_t)p[1]<<16)|((uint32_t)p[0]<<24);

  FH_DEBUG_MSG(("Throwing EndOfStreamException\n"));
  throw EndOfStreamException();
}

int32_t libfreehand::readS32(librevenge::RVNGInputStream *input)
{
  return (int32_t)readU32(input);
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
