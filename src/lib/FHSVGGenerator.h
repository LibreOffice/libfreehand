/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __FHSVGGENERATOR_H__
#define __FHSVGGENERATOR_H__

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <libwpd/libwpd.h>
#include <libwpg/libwpg.h>
#include "FHStringVector.h"

namespace libfreehand
{

class FHSVGGenerator : public libwpg::WPGPaintInterface
{
public:
  FHSVGGenerator(FHStringVector &vec);
  ~FHSVGGenerator();

  void startGraphics(const ::WPXPropertyList &propList);
  void endGraphics();
  void startLayer(const ::WPXPropertyList &propList);
  void endLayer();
  void startEmbeddedGraphics(const ::WPXPropertyList & /*propList*/) {}
  void endEmbeddedGraphics() {}

  void setStyle(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &gradient);

  void drawRectangle(const ::WPXPropertyList &propList);
  void drawEllipse(const ::WPXPropertyList &propList);
  void drawPolyline(const ::WPXPropertyListVector &vertices);
  void drawPolygon(const ::WPXPropertyListVector &vertices);
  void drawPath(const ::WPXPropertyListVector &path);
  void drawGraphicObject(const ::WPXPropertyList &propList, const ::WPXBinaryData &binaryData);
  void startTextObject(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &path);
  void endTextObject();
  void startTextLine(const ::WPXPropertyList & /* propList */) {}
  void endTextLine() {}
  void startTextSpan(const ::WPXPropertyList &propList);
  void endTextSpan();
  void insertText(const ::WPXString &str);

private:
  ::WPXPropertyListVector m_gradient;
  ::WPXPropertyList m_style;
  int m_gradientIndex;
  int m_patternIndex;
  int m_shadowIndex;
  void writeStyle(bool isClosed=true);
  void drawPolySomething(const ::WPXPropertyListVector &vertices, bool isClosed);

  std::ostringstream m_outputSink;
  FHStringVector &m_vec;
};

} // namespace libfreehand

#endif // __FHSVGGENERATOR_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
