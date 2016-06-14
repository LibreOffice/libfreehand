/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sstream>
#include <string>
#include <string.h>
#include <libfreehand/libfreehand.h>
#include "FHParser.h"
#include "libfreehand_utils.h"

namespace libfreehand
{

namespace
{

bool findAGD(librevenge::RVNGInputStream *input)
{
  unsigned agd = readU32(input);
  input->seek(-4, librevenge::RVNG_SEEK_CUR);
  if (((agd >> 24) & 0xff) == 'A' && ((agd >> 16) & 0xff) == 'G' && ((agd >> 8) & 0xff) == 'D')
  {
    FH_DEBUG_MSG(("Found AGD at offset 0x%lx (FreeHand version %i)\n", input->tell(), (agd & 0xff) - 0x30 + 5));
    return true;
  }
  else if (((agd >> 24) & 0xff) == 'F' && ((agd >> 16) & 0xff) == 'H' && ((agd >> 8) & 0xff) == '3')
  {
    FH_DEBUG_MSG(("Found FH3 at offset 0x%lx (FreeHand version %c.%c)\n", input->tell(), (agd >> 8) & 0xff, agd & 0xff));
    return true;
  }
  else
  {
    // parse the document for AGD block
    while (!input->isEnd())
    {
      if (0x1c != readU8(input))
        return false;
      unsigned short opcode = readU16(input);
      unsigned char flag = readU8(input);
      unsigned length = readU8(input);
      if (0x80 == flag)
      {
        if (4 != length)
          return false;
        length = readU32(input);
        if (0x080a == opcode)
        {
          agd = readU32(input);
          input->seek(-4, librevenge::RVNG_SEEK_CUR);
          if (((agd >> 24) & 0xff) == 'A' && ((agd >> 16) & 0xff) == 'G' && ((agd >> 8) & 0xff) == 'D')
          {
            FH_DEBUG_MSG(("Found AGD at offset 0x%lx (FreeHand version %i)\n", input->tell(), (agd & 0xff) - 0x30 + 5));
            return true;
          }
        }
      }
      input->seek(length, librevenge::RVNG_SEEK_CUR);
    }
  }
  return false;
}

} // anonymous namespace

/**
Analyzes the content of an input stream to see if it can be parsed
\param input The input stream
\return A value that indicates whether the content from the input
stream is a FreeHand Document that libfreehand is able to parse
*/
FHAPI bool FreeHandDocument::isSupported(librevenge::RVNGInputStream *input)
{
  if (!input)
    return false;

  try
  {
    input->seek(0, librevenge::RVNG_SEEK_SET);
    if (findAGD(input))
    {
      input->seek(0, librevenge::RVNG_SEEK_SET);
      return true;
    }
  }
  catch (...)
  {
  }
  return false;

}

/**
Parses the input stream content. It will make callbacks to the functions provided by a
librevenge::RVNGDrawingInterface class implementation when needed. This is often commonly called the
'main parsing routine'.
\param input The input stream
\param painter A librevenge::RVNGDrawingerInterface implementation
\return A value that indicates whether the parsing was successful
*/
FHAPI bool FreeHandDocument::parse(librevenge::RVNGInputStream *input, librevenge::RVNGDrawingInterface *painter)
{
  if (!input)
    return false;

  try
  {
    input->seek(0, librevenge::RVNG_SEEK_SET);
    if (findAGD(input))
    {
      FHParser parser;
      if (!parser.parse(input, painter))
        return false;
    }
    else
      return false;
    return true;
  }
  catch (...)
  {
  }
  return false;
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
