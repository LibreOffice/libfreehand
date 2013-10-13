/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <libwpd-stream/libwpd-stream.h>
#include <libwpd/libwpd.h>
#include <libfreehand/libfreehand.h>

namespace
{

int printUsage()
{
  printf("Usage: vsd2svg [OPTION] <FreeHand Document>\n");
  printf("\n");
  printf("Options:\n");
  printf("--help                Shows this help message\n");
  return -1;
}

} // anonymous namespace

int main(int argc, char *argv[])
{
  if (argc < 2)
    return printUsage();

  char *file = 0;

  for (int i = 1; i < argc; i++)
  {
    if (!file && strncmp(argv[i], "--", 2))
      file = argv[i];
    else
      return printUsage();
  }

  if (!file)
    return printUsage();

  WPXFileStream input(file);

  if (!libfreehand::FreeHandDocument::isSupported(&input))
  {
    std::cerr << "ERROR: Unsupported file format!" << std::endl;
    return 1;
  }

  libfreehand::FHStringVector output;
  if (!libfreehand::FreeHandDocument::generateSVG(&input, output))
  {
    std::cerr << "ERROR: SVG Generation failed!" << std::endl;
    return 1;
  }

  if (output.empty())
  {
    std::cerr << "ERROR: No SVG document generated!" << std::endl;
    return 1;
  }

  std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
  std::cout << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"";
  std::cout << " \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";

  std::cout << output[0].cstr() << std::endl;

  return 0;
}
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
