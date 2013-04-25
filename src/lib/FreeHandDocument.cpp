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
#include "FreeHandDocument.h"
#include "FHCollector.h"
#include "FHParser.h"
#include "FHSVGGenerator.h"
#include "libfreehand_utils.h"

namespace
{

using namespace libfreehand;

static bool findAGD(WPXInputStream *input)
{
  unsigned agd = readU32(input);
  input->seek(-4, WPX_SEEK_CUR);
  if (((agd >> 24) & 0xff) == 'A' && ((agd >> 16) & 0xff) == 'G' && ((agd >> 8) & 0xff) == 'D')
  {
    FH_DEBUG_MSG(("Found AGD at offset 0x%lx (FreeHand version %i)\n", input->tell(), (agd & 0xff) - 0x30 + 5));
    return true;
  }
  else
  {
    // parse the document for AGD block
    while (!input->atEOS())
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
          input->seek(-4, WPX_SEEK_CUR);
          if (((agd >> 24) & 0xff) == 'A' && ((agd >> 16) & 0xff) == 'G' && ((agd >> 8) & 0xff) == 'D')
          {
            FH_DEBUG_MSG(("Found AGD at offset 0x%lx (FreeHand version %i)\n", input->tell(), (agd & 0xff) - 0x30 + 5));
            return true;
          }
        }
      }
      input->seek(length, WPX_SEEK_CUR);
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
bool libfreehand::FreeHandDocument::isSupported(WPXInputStream *input)
{
  try
  {
    input->seek(0, WPX_SEEK_SET);
    if (findAGD(input))
    {
      input->seek(0, WPX_SEEK_SET);
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
WPGPaintInterface class implementation when needed. This is often commonly called the
'main parsing routine'.
\param input The input stream
\param painter A WPGPainterInterface implementation
\return A value that indicates whether the parsing was successful
*/
bool libfreehand::FreeHandDocument::parse(::WPXInputStream *input, libwpg::WPGPaintInterface *painter)
{
  try
  {
    input->seek(0, WPX_SEEK_SET);
    if (findAGD(input))
    {
      FHCollector collector(painter);
      FHParser parser(input, &collector);
      return parser.parse();
    }
  }
  catch (...)
  {
  }
  return false;
}

/**
Parses the input stream content and generates a valid Scalable Vector Graphics
Provided as a convenience function for applications that support SVG internally.
\param input The input stream
\param output The output string whose content is the resulting SVG
\return A value that indicates whether the SVG generation was successful.
*/
bool libfreehand::FreeHandDocument::generateSVG(::WPXInputStream *input, libfreehand::FHStringVector &output)
{
  libfreehand::FHSVGGenerator generator(output);
  bool result = libfreehand::FreeHandDocument::parse(input, &generator);
  return result;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
