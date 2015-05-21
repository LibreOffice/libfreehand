/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <string.h>
#include <librevenge/librevenge.h>
#include "FHCollector.h"
#include "FHConstants.h"
#include "libfreehand_utils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef DUMP_CONTENTS
#define DUMP_CONTENTS 0
#endif
#ifndef DUMP_BINARY_OBJECTS
#define DUMP_BINARY_OBJECTS 0
#endif
#ifndef DEBUG_BOUNDING_BOX
#define DEBUG_BOUNDING_BOX 0
#endif
#ifndef DUMP_TILE_FILLS
#define DUMP_TILE_FILLS 0
#endif

#define FH_UNINITIALIZED(pI) \
  FH_ALMOST_ZERO(pI.m_minX) && FH_ALMOST_ZERO(pI.m_minY) && FH_ALMOST_ZERO(pI.m_maxY) && FH_ALMOST_ZERO(pI.m_maxX)

namespace
{
bool isTiff(const unsigned char *buffer, unsigned long size)
{
  if (size < 4)
    return false;
  if (buffer[0] == 0x49 && buffer[1] == 0x49 && buffer[2] == 0x2a && buffer[3] == 0x00)
    return true;
  if (buffer[0] == 0x4d && buffer[1] == 0x4d && buffer[2] == 0x00 && buffer[3] == 0x2a)
    return true;
  return false;
}

bool isBmp(const unsigned char *buffer, unsigned long size)
{
  if (size < 6)
    return false;
  if (buffer[0] != 0x42)
    return false;
  if (buffer[1] != 0x4d)
    return false;
  unsigned bufferSize = ((unsigned long)buffer[2] + ((unsigned long)buffer[3] << 8) + ((unsigned long)buffer[4] << 8) + ((unsigned long)buffer[5] << 8));
  if (bufferSize != size)
    return false;
  return true;
}

bool isJpeg(const unsigned char *buffer, unsigned long size)
{
  if (size < 4)
    return false;
  if (buffer[0] != 0xff)
    return false;
  if (buffer[1] != 0xd8)
    return false;
  if (buffer[size-2] != 0xff)
    return false;
  if (buffer[size-1] != 0xd9)
    return false;
  return true;
}

bool isPng(const unsigned char *buffer, unsigned long size)
{
  if (size < 8)
    return false;
  if (buffer[0] != 0x89)
    return false;
  if (buffer[1] != 0x50)
    return false;
  if (buffer[2] != 0x4e)
    return false;
  if (buffer[3] != 0x47)
    return false;
  if (buffer[4] != 0x0d)
    return false;
  if (buffer[5] != 0x0a)
    return false;
  if (buffer[6] != 0x1a)
    return false;
  if (buffer[7] != 0x0a)
    return false;
  return true;
}

librevenge::RVNGString _getColorString(const libfreehand::FHRGBColor &color)
{
  ::librevenge::RVNGString colorString;
  colorString.sprintf("#%.2x%.2x%.2x", color.m_red >> 8, color.m_green >> 8, color.m_blue >> 8);
  return colorString;
}

static void _composePath(::librevenge::RVNGPropertyListVector &path, bool isClosed)
{
  bool firstPoint = true;
  bool wasMove = false;
  double initialX = 0.0;
  double initialY = 0.0;
  double previousX = 0.0;
  double previousY = 0.0;
  double x = 0.0;
  double y = 0.0;
  std::vector<librevenge::RVNGPropertyList> tmpPath;

  librevenge::RVNGPropertyListVector::Iter i(path);
  for (i.rewind(); i.next();)
  {
    if (!i()["librevenge:path-action"])
      continue;
    if (i()["svg:x"] && i()["svg:y"])
    {
      bool ignoreM = false;
      x = i()["svg:x"]->getDouble();
      y = i()["svg:y"]->getDouble();
      if (firstPoint)
      {
        initialX = x;
        initialY = y;
        firstPoint = false;
        wasMove = true;
      }
      else if (i()["librevenge:path-action"]->getStr() == "M")
      {
        if (FH_ALMOST_ZERO(previousX - x) && FH_ALMOST_ZERO(previousY - y))
          ignoreM = true;
        else
        {
          if (!tmpPath.empty())
          {
            if (!wasMove)
            {
              if ((FH_ALMOST_ZERO(initialX - previousX) && FH_ALMOST_ZERO(initialY - previousY)) || isClosed)
              {
                librevenge::RVNGPropertyList node;
                node.insert("librevenge:path-action", "Z");
                tmpPath.push_back(node);
              }
            }
            else
              tmpPath.pop_back();
          }
        }

        if (!ignoreM)
        {
          initialX = x;
          initialY = y;
          wasMove = true;
        }

      }
      else
        wasMove = false;

      if (!ignoreM)
      {
        tmpPath.push_back(i());
        previousX = x;
        previousY = y;
      }

    }
    else if (i()["librevenge:path-action"]->getStr() == "Z")
    {
      if (tmpPath.back()["librevenge:path-action"] && tmpPath.back()["librevenge:path-action"]->getStr() != "Z")
        tmpPath.push_back(i());
    }
  }
  if (!tmpPath.empty())
  {
    if (!wasMove)
    {
      if ((FH_ALMOST_ZERO(initialX - previousX) && FH_ALMOST_ZERO(initialY - previousY)) || isClosed)
      {
        if (tmpPath.back()["librevenge:path-action"] && tmpPath.back()["librevenge:path-action"]->getStr() != "Z")
        {
          librevenge::RVNGPropertyList closedPath;
          closedPath.insert("librevenge:path-action", "Z");
          tmpPath.push_back(closedPath);
        }
      }
    }
    else
      tmpPath.pop_back();
  }
  if (!tmpPath.empty())
  {
    path.clear();
    for (std::vector<librevenge::RVNGPropertyList>::const_iterator iter = tmpPath.begin(); iter != tmpPath.end(); ++iter)
      path.append(*iter);
  }
}

}

libfreehand::FHCollector::FHCollector() :
  m_pageInfo(), m_fhTail(), m_block(), m_transforms(), m_paths(), m_strings(), m_names(), m_lists(),
  m_layers(), m_groups(), m_currentTransforms(), m_fakeTransforms(), m_compositePaths(), m_fonts(), m_paragraphs(),
  m_textBloks(), m_textObjects(), m_charProperties(), m_rgbColors(), m_basicFills(), m_propertyLists(),
  m_displayTexts(), m_graphicStyles(), m_attributeHolders(), m_data(), m_dataLists(), m_images(), m_multiColorLists(),
  m_linearFills(), m_tints(), m_lensFills(), m_radialFills(), m_newBlends(), m_filterAttributeHolders(),
  m_shadowFilters(), m_glowFilters(), m_tileFills(), m_symbolClasses(), m_symbolInstances(), m_patternFills(),
  m_strokeId(0), m_fillId(0), m_contentId(0)
{
}

libfreehand::FHCollector::~FHCollector()
{
}

void libfreehand::FHCollector::collectPageInfo(const FHPageInfo &pageInfo)
{
  m_pageInfo = pageInfo;
}

void libfreehand::FHCollector::collectString(unsigned recordId, const librevenge::RVNGString &str)
{
  m_strings[recordId] = str;
}

void libfreehand::FHCollector::collectName(unsigned recordId, const librevenge::RVNGString &name)
{
  m_names[name] = recordId;
  if (name == "stroke")
    m_strokeId = recordId;
  if (name == "fill")
    m_fillId = recordId;
  if (name == "contents")
    m_contentId = recordId;
}

void libfreehand::FHCollector::collectPath(unsigned recordId, const libfreehand::FHPath &path)
{
  m_paths[recordId] = path;
}

void libfreehand::FHCollector::collectXform(unsigned recordId,
                                            double m11, double m21, double m12, double m22, double m13, double m23)
{
  m_transforms[recordId] = FHTransform(m11, m21, m12, m22, m13, m23);
}

void libfreehand::FHCollector::collectFHTail(unsigned /* recordId */, const FHTail &fhTail)
{
  m_fhTail = fhTail;
}

void libfreehand::FHCollector::collectBlock(unsigned recordId, const libfreehand::FHBlock &block)
{
  if (m_block.first && m_block.first != recordId)
  {
    FH_DEBUG_MSG(("FHCollector::collectBlock -- WARNING: Several \"Block\" records in the file\n"));
  }
  m_block = std::make_pair(recordId, block);
}

void libfreehand::FHCollector::collectList(unsigned recordId, const libfreehand::FHList &lst)
{
  m_lists[recordId] = lst;
}

void libfreehand::FHCollector::collectLayer(unsigned recordId, const libfreehand::FHLayer &layer)
{
  m_layers[recordId] = layer;
}

void libfreehand::FHCollector::collectGroup(unsigned recordId, const libfreehand::FHGroup &group)
{
  m_groups[recordId] = group;
}

void libfreehand::FHCollector::collectClipGroup(unsigned recordId, const libfreehand::FHGroup &group)
{
  m_clipGroups[recordId] = group;
}

void libfreehand::FHCollector::collectCompositePath(unsigned recordId, const libfreehand::FHCompositePath &compositePath)
{
  m_compositePaths[recordId] = compositePath;
}

void libfreehand::FHCollector::collectTString(unsigned recordId, const std::vector<unsigned> &elements)
{
  m_tStrings[recordId] = elements;
}

void libfreehand::FHCollector::collectAGDFont(unsigned recordId, const FHAGDFont &font)
{
  m_fonts[recordId] = font;
}

void libfreehand::FHCollector::collectParagraph(unsigned recordId, const FHParagraph &paragraph)
{
  m_paragraphs[recordId] = paragraph;
}

void libfreehand::FHCollector::collectTextBlok(unsigned recordId, const std::vector<unsigned short> &characters)
{
  m_textBloks[recordId]  = characters;
}

void libfreehand::FHCollector::collectTextObject(unsigned recordId, const FHTextObject &textObject)
{
  m_textObjects[recordId] = textObject;
}

void libfreehand::FHCollector::collectCharProps(unsigned recordId, const FHCharProperties &charProps)
{
  m_charProperties[recordId] = charProps;
}

void libfreehand::FHCollector::collectColor(unsigned recordId, const FHRGBColor &color)
{
  m_rgbColors[recordId] = color;
}

void libfreehand::FHCollector::collectTintColor(unsigned recordId, const FHTintColor &color)
{
  m_tints[recordId] = color;
}

void libfreehand::FHCollector::collectBasicFill(unsigned recordId, const FHBasicFill &fill)
{
  m_basicFills[recordId] = fill;
}

void libfreehand::FHCollector::collectBasicLine(unsigned recordId, const FHBasicLine &line)
{
  m_basicLines[recordId] = line;
}

void libfreehand::FHCollector::collectTileFill(unsigned recordId, const FHTileFill &fill)
{
  m_tileFills[recordId] = fill;
}

void libfreehand::FHCollector::collectPatternFill(unsigned recordId, const FHPatternFill &fill)
{
  m_patternFills[recordId] = fill;
}

void libfreehand::FHCollector::collectPropList(unsigned recordId, const FHPropList &propertyList)
{
  m_propertyLists[recordId] = propertyList;
}

void libfreehand::FHCollector::collectDisplayText(unsigned recordId, const FHDisplayText &displayText)
{
  m_displayTexts[recordId] = displayText;
}

void libfreehand::FHCollector::collectGraphicStyle(unsigned recordId, const FHGraphicStyle &graphicStyle)
{
  m_graphicStyles[recordId] = graphicStyle;
}

void libfreehand::FHCollector::collectAttributeHolder(unsigned recordId, const FHAttributeHolder &attributeHolder)
{
  m_attributeHolders[recordId] = attributeHolder;
}

void libfreehand::FHCollector::collectFilterAttributeHolder(unsigned recordId, const FHFilterAttributeHolder &filterAttributeHolder)
{
  m_filterAttributeHolders[recordId] = filterAttributeHolder;
}

void libfreehand::FHCollector::collectData(unsigned recordId, const ::librevenge::RVNGBinaryData &data)
{
  m_data[recordId] = data;
}

void libfreehand::FHCollector::collectDataList(unsigned recordId, const FHDataList &list)
{
  m_dataLists[recordId] = list;
}

void libfreehand::FHCollector::collectImage(unsigned recordId, const FHImageImport &image)
{
  m_images[recordId] = image;
}

void libfreehand::FHCollector::collectMultiColorList(unsigned recordId, const std::vector<FHColorStop> &colorStops)
{
  m_multiColorLists[recordId] = colorStops;
}

void libfreehand::FHCollector::collectLinearFill(unsigned recordId, const FHLinearFill &fill)
{
  m_linearFills[recordId] = fill;
}

void libfreehand::FHCollector::collectLensFill(unsigned recordId, const FHLensFill &fill)
{
  m_lensFills[recordId] = fill;
}

void libfreehand::FHCollector::collectRadialFill(unsigned recordId, const FHRadialFill &fill)
{
  m_radialFills[recordId] = fill;
}

void libfreehand::FHCollector::collectNewBlend(unsigned recordId, const FHNewBlend &newBlend)
{
  m_newBlends[recordId] = newBlend;
}

void libfreehand::FHCollector::collectOpacityFilter(unsigned recordId, double opacity)
{
  m_opacityFilters[recordId] = opacity;
}

void libfreehand::FHCollector::collectFWShadowFilter(unsigned recordId, const FWShadowFilter &filter)
{
  m_shadowFilters[recordId] = filter;
}

void libfreehand::FHCollector::collectFWGlowFilter(unsigned recordId, const FWGlowFilter &filter)
{
  m_glowFilters[recordId] = filter;
}

void libfreehand::FHCollector::collectSymbolClass(unsigned recordId, const FHSymbolClass &symbolClass)
{
  m_symbolClasses[recordId] = symbolClass;
}

void libfreehand::FHCollector::collectSymbolInstance(unsigned recordId, const FHSymbolInstance &symbolInstance)
{
  m_symbolInstances[recordId] = symbolInstance;
}

void libfreehand::FHCollector::_normalizePath(libfreehand::FHPath &path)
{
  FHTransform trafo(1.0, 0.0, 0.0, -1.0, - m_pageInfo.m_minX, m_pageInfo.m_maxY);
  path.transform(trafo);
}

void libfreehand::FHCollector::_normalizePoint(double &x, double &y)
{
  FHTransform trafo(1.0, 0.0, 0.0, -1.0, - m_pageInfo.m_minX, m_pageInfo.m_maxY);
  trafo.applyToPoint(x, y);
}

void libfreehand::FHCollector::_getBBofPath(const FHPath *path, libfreehand::FHBoundingBox &bBox)
{
  if (!path || path->empty())
    return;

  FHPath fhPath(*path);
  unsigned short xform = fhPath.getXFormId();

  if (xform)
  {
    const FHTransform *trafo = _findTransform(xform);
    if (trafo)
      fhPath.transform(*trafo);
  }
  std::stack<FHTransform> groupTransforms(m_currentTransforms);
  while (!groupTransforms.empty())
  {
    fhPath.transform(groupTransforms.top());
    groupTransforms.pop();
  }
  _normalizePath(fhPath);
  std::stack<FHTransform> fakeTransforms(m_fakeTransforms);
  while (!fakeTransforms.empty())
  {
    fhPath.transform(fakeTransforms.top());
    fakeTransforms.pop();
  }

  FHBoundingBox tmpBBox;
  fhPath.getBoundingBox(tmpBBox.m_xmin, tmpBBox.m_ymin, tmpBBox.m_xmax, tmpBBox.m_ymax);
  bBox.merge(tmpBBox);
}

void libfreehand::FHCollector::_getBBofGroup(const FHGroup *group, libfreehand::FHBoundingBox &bBox)
{
  if (!group)
    return;

  if (group->m_xFormId)
  {
    const FHTransform *trafo = _findTransform(group->m_xFormId);
    if (trafo)
      m_currentTransforms.push(*trafo);
    else
      m_currentTransforms.push(libfreehand::FHTransform());
  }
  else
    m_currentTransforms.push(libfreehand::FHTransform());

  const std::vector<unsigned> *elements = _findListElements(group->m_elementsId);
  if (!elements)
  {
    FH_DEBUG_MSG(("ERROR: The pointed element list does not exist\n"));
    return;
  }

  for (std::vector<unsigned>::const_iterator iterVec = elements->begin(); iterVec != elements->end(); ++iterVec)
  {
    FHBoundingBox tmpBBox;
    _getBBofSomething(*iterVec, tmpBBox);
    bBox.merge(tmpBBox);
  }

  if (!m_currentTransforms.empty())
    m_currentTransforms.pop();
}

void libfreehand::FHCollector::_getBBofClipGroup(const FHGroup *group, libfreehand::FHBoundingBox &bBox)
{
  if (!group)
    return;

  if (group->m_xFormId)
  {
    const FHTransform *trafo = _findTransform(group->m_xFormId);
    if (trafo)
      m_currentTransforms.push(*trafo);
    else
      m_currentTransforms.push(libfreehand::FHTransform());
  }
  else
    m_currentTransforms.push(libfreehand::FHTransform());

  const std::vector<unsigned> *elements = _findListElements(group->m_elementsId);
  if (!elements)
  {
    FH_DEBUG_MSG(("ERROR: The pointed element list does not exist\n"));
    return;
  }

  std::vector<unsigned>::const_iterator iterVec = elements->begin();
  FHBoundingBox tmpBBox;
  _getBBofSomething(*iterVec, tmpBBox);
  bBox.merge(tmpBBox);

  if (!m_currentTransforms.empty())
    m_currentTransforms.pop();
}

void libfreehand::FHCollector::_getBBofCompositePath(const FHCompositePath *compositePath, libfreehand::FHBoundingBox &bBox)
{
  if (!compositePath)
    return;

  const std::vector<unsigned> *elements = _findListElements(compositePath->m_elementsId);
  if (elements && !elements->empty())
  {
    libfreehand::FHPath fhPath;
    std::vector<unsigned>::const_iterator iter = elements->begin();
    const libfreehand::FHPath *path = _findPath(*(iter++));
    if (path)
    {
      fhPath = *path;
      if (!fhPath.getGraphicStyleId())
        fhPath.setGraphicStyleId(compositePath->m_graphicStyleId);
    }

    for (; iter != elements->end(); ++iter)
    {
      path = _findPath(*iter);
      if (path)
      {
        fhPath.appendPath(*path);
        if (!fhPath.getGraphicStyleId())
          fhPath.setGraphicStyleId(compositePath->m_graphicStyleId);
      }
    }
    FHBoundingBox tmpBBox;
    _getBBofPath(&fhPath, tmpBBox);
    bBox.merge(tmpBBox);
  }
}

void libfreehand::FHCollector::_getBBofTextObject(const FHTextObject *textObject, libfreehand::FHBoundingBox &bBox)
{
  if (!textObject)
    return;

  double xa = textObject->m_startX;
  double ya = textObject->m_startY;
  double xb = textObject->m_startX + textObject->m_width;
  double yb = textObject->m_startY + textObject->m_height;
  double xc = xa;
  double yc = yb;
  double xd = xb;
  double yd = ya;
  unsigned xFormId = textObject->m_xFormId;
  if (xFormId)
  {
    const FHTransform *trafo = _findTransform(xFormId);
    if (trafo)
    {
      trafo->applyToPoint(xa, ya);
      trafo->applyToPoint(xb, yb);
      trafo->applyToPoint(xc, yc);
      trafo->applyToPoint(xd, yd);
    }
  }
  std::stack<FHTransform> groupTransforms(m_currentTransforms);
  while (!groupTransforms.empty())
  {
    groupTransforms.top().applyToPoint(xa, ya);
    groupTransforms.top().applyToPoint(xb, yb);
    groupTransforms.top().applyToPoint(xc, yc);
    groupTransforms.top().applyToPoint(xd, yd);
    groupTransforms.pop();
  }
  _normalizePoint(xa, ya);
  _normalizePoint(xb, yb);
  _normalizePoint(xc, yc);
  _normalizePoint(xd, yd);

  std::stack<FHTransform> fakeTransforms(m_fakeTransforms);
  while (!fakeTransforms.empty())
  {
    fakeTransforms.top().applyToPoint(xa, ya);
    fakeTransforms.top().applyToPoint(xb, yb);
    fakeTransforms.top().applyToPoint(xc, yc);
    fakeTransforms.top().applyToPoint(xd, yd);
    fakeTransforms.pop();
  }

  FHBoundingBox tmpBBox;
  if (xa < tmpBBox.m_xmin) tmpBBox.m_xmin = xa;
  if (xb < tmpBBox.m_xmin) tmpBBox.m_xmin = xb;
  if (xc < tmpBBox.m_xmin) tmpBBox.m_xmin = xc;
  if (xd < tmpBBox.m_xmin) tmpBBox.m_xmin = xd;

  if (xa > tmpBBox.m_xmax) tmpBBox.m_xmax = xa;
  if (xb > tmpBBox.m_xmax) tmpBBox.m_xmax = xb;
  if (xc > tmpBBox.m_xmax) tmpBBox.m_xmax = xc;
  if (xd > tmpBBox.m_xmax) tmpBBox.m_xmax = xd;

  if (ya < tmpBBox.m_ymin) tmpBBox.m_ymin = ya;
  if (yb < tmpBBox.m_ymin) tmpBBox.m_ymin = yb;
  if (yc < tmpBBox.m_ymin) tmpBBox.m_ymin = yc;
  if (yd < tmpBBox.m_ymin) tmpBBox.m_ymin = yd;

  if (ya > tmpBBox.m_ymax) tmpBBox.m_ymax = ya;
  if (yb > tmpBBox.m_ymax) tmpBBox.m_ymax = yb;
  if (yc > tmpBBox.m_ymax) tmpBBox.m_ymax = yc;
  if (yd > tmpBBox.m_ymax) tmpBBox.m_ymax = yd;
  bBox.merge(tmpBBox);
}

void libfreehand::FHCollector::_getBBofDisplayText(const FHDisplayText *displayText, libfreehand::FHBoundingBox &bBox)
{
  if (!displayText)
    return;

  double xa = displayText->m_startX;
  double ya = displayText->m_startY;
  double xb = displayText->m_startX + displayText->m_width;
  double yb = displayText->m_startY + displayText->m_height;
  double xc = xa;
  double yc = yb;
  double xd = xb;
  double yd = ya;
  unsigned xFormId = displayText->m_xFormId;
  if (xFormId)
  {
    const FHTransform *trafo = _findTransform(xFormId);
    if (trafo)
    {
      trafo->applyToPoint(xa, ya);
      trafo->applyToPoint(xb, yb);
      trafo->applyToPoint(xc, yc);
      trafo->applyToPoint(xd, yd);
    }
  }
  std::stack<FHTransform> groupTransforms(m_currentTransforms);
  while (!groupTransforms.empty())
  {
    groupTransforms.top().applyToPoint(xa, ya);
    groupTransforms.top().applyToPoint(xb, yb);
    groupTransforms.top().applyToPoint(xc, yc);
    groupTransforms.top().applyToPoint(xd, yd);
    groupTransforms.pop();
  }
  _normalizePoint(xa, ya);
  _normalizePoint(xb, yb);
  _normalizePoint(xc, yc);
  _normalizePoint(xd, yd);

  std::stack<FHTransform> fakeTransforms(m_fakeTransforms);
  while (!fakeTransforms.empty())
  {
    fakeTransforms.top().applyToPoint(xa, ya);
    fakeTransforms.top().applyToPoint(xb, yb);
    fakeTransforms.top().applyToPoint(xc, yc);
    fakeTransforms.top().applyToPoint(xd, yd);
    fakeTransforms.pop();
  }

  FHBoundingBox tmpBBox;
  if (xa < tmpBBox.m_xmin) tmpBBox.m_xmin = xa;
  if (xb < tmpBBox.m_xmin) tmpBBox.m_xmin = xb;
  if (xc < tmpBBox.m_xmin) tmpBBox.m_xmin = xc;
  if (xd < tmpBBox.m_xmin) tmpBBox.m_xmin = xd;

  if (xa > tmpBBox.m_xmax) tmpBBox.m_xmax = xa;
  if (xb > tmpBBox.m_xmax) tmpBBox.m_xmax = xb;
  if (xc > tmpBBox.m_xmax) tmpBBox.m_xmax = xc;
  if (xd > tmpBBox.m_xmax) tmpBBox.m_xmax = xd;

  if (ya < tmpBBox.m_ymin) tmpBBox.m_ymin = ya;
  if (yb < tmpBBox.m_ymin) tmpBBox.m_ymin = yb;
  if (yc < tmpBBox.m_ymin) tmpBBox.m_ymin = yc;
  if (yd < tmpBBox.m_ymin) tmpBBox.m_ymin = yd;

  if (ya > tmpBBox.m_ymax) tmpBBox.m_ymax = ya;
  if (yb > tmpBBox.m_ymax) tmpBBox.m_ymax = yb;
  if (yc > tmpBBox.m_ymax) tmpBBox.m_ymax = yc;
  if (yd > tmpBBox.m_ymax) tmpBBox.m_ymax = yd;
  bBox.merge(tmpBBox);
}

void libfreehand::FHCollector::_getBBofImageImport(const FHImageImport *image, libfreehand::FHBoundingBox &bBox)
{
  if (!image)
    return;

  double xa = image->m_startX;
  double ya = image->m_startY;
  double xb = image->m_startX + image->m_width;
  double yb = image->m_startY + image->m_height;
  double xc = xa;
  double yc = yb;
  double xd = xb;
  double yd = ya;
  unsigned xFormId = image->m_xFormId;
  if (xFormId)
  {
    const FHTransform *trafo = _findTransform(xFormId);
    if (trafo)
    {
      trafo->applyToPoint(xa, ya);
      trafo->applyToPoint(xb, yb);
      trafo->applyToPoint(xc, yc);
      trafo->applyToPoint(xd, yd);
    }
  }
  std::stack<FHTransform> groupTransforms(m_currentTransforms);
  while (!groupTransforms.empty())
  {
    groupTransforms.top().applyToPoint(xa, ya);
    groupTransforms.top().applyToPoint(xb, yb);
    groupTransforms.top().applyToPoint(xc, yc);
    groupTransforms.top().applyToPoint(xd, yd);
    groupTransforms.pop();
  }
  _normalizePoint(xa, ya);
  _normalizePoint(xb, yb);
  _normalizePoint(xc, yc);
  _normalizePoint(xd, yd);

  std::stack<FHTransform> fakeTransforms(m_fakeTransforms);
  while (!fakeTransforms.empty())
  {
    fakeTransforms.top().applyToPoint(xa, ya);
    fakeTransforms.top().applyToPoint(xb, yb);
    fakeTransforms.top().applyToPoint(xc, yc);
    fakeTransforms.top().applyToPoint(xd, yd);
    fakeTransforms.pop();
  }

  FHBoundingBox tmpBBox;
  if (xa < tmpBBox.m_xmin) tmpBBox.m_xmin = xa;
  if (xb < tmpBBox.m_xmin) tmpBBox.m_xmin = xb;
  if (xc < tmpBBox.m_xmin) tmpBBox.m_xmin = xc;
  if (xd < tmpBBox.m_xmin) tmpBBox.m_xmin = xd;

  if (xa > tmpBBox.m_xmax) tmpBBox.m_xmax = xa;
  if (xb > tmpBBox.m_xmax) tmpBBox.m_xmax = xb;
  if (xc > tmpBBox.m_xmax) tmpBBox.m_xmax = xc;
  if (xd > tmpBBox.m_xmax) tmpBBox.m_xmax = xd;

  if (ya < tmpBBox.m_ymin) tmpBBox.m_ymin = ya;
  if (yb < tmpBBox.m_ymin) tmpBBox.m_ymin = yb;
  if (yc < tmpBBox.m_ymin) tmpBBox.m_ymin = yc;
  if (yd < tmpBBox.m_ymin) tmpBBox.m_ymin = yd;

  if (ya > tmpBBox.m_ymax) tmpBBox.m_ymax = ya;
  if (yb > tmpBBox.m_ymax) tmpBBox.m_ymax = yb;
  if (yc > tmpBBox.m_ymax) tmpBBox.m_ymax = yc;
  if (yd > tmpBBox.m_ymax) tmpBBox.m_ymax = yd;
  bBox.merge(tmpBBox);
}

void libfreehand::FHCollector::_getBBofNewBlend(const FHNewBlend * /* newBlend */, libfreehand::FHBoundingBox & /* bBox */)
{
}

void libfreehand::FHCollector::_getBBofSymbolInstance(const FHSymbolInstance *symbolInstance, libfreehand::FHBoundingBox &bBox)
{
  if (!symbolInstance)
    return;

  m_currentTransforms.push(symbolInstance->m_xForm);

  const FHSymbolClass *symbolClass = _findSymbolClass(symbolInstance->m_symbolClassId);
  if (symbolClass)
  {
    FHBoundingBox tmpBBox;
    _getBBofSomething(symbolClass->m_groupId, tmpBBox);
    bBox.merge(tmpBBox);
  }

  if (!m_currentTransforms.empty())
    m_currentTransforms.pop();
}

void libfreehand::FHCollector::_getBBofSomething(unsigned somethingId, libfreehand::FHBoundingBox &bBox)
{
  if (!somethingId)
    return;

  FHBoundingBox tmpBBox;
  _getBBofGroup(_findGroup(somethingId), tmpBBox);
  _getBBofClipGroup(_findClipGroup(somethingId), tmpBBox);
  _getBBofPath(_findPath(somethingId), tmpBBox);
  _getBBofCompositePath(_findCompositePath(somethingId), tmpBBox);
  _getBBofTextObject(_findTextObject(somethingId), tmpBBox);
  _getBBofDisplayText(_findDisplayText(somethingId), tmpBBox);
  _getBBofImageImport(_findImageImport(somethingId), tmpBBox);
  _getBBofNewBlend(_findNewBlend(somethingId), tmpBBox);
  _getBBofSymbolInstance(_findSymbolInstance(somethingId), tmpBBox);
  bBox.merge(tmpBBox);
}


void libfreehand::FHCollector::_outputPath(const libfreehand::FHPath *path, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !path || path->empty())
    return;

  FHPath fhPath(*path);
  librevenge::RVNGPropertyList propList;
  _appendStrokeProperties(propList, fhPath.getGraphicStyleId());
  _appendFillProperties(propList, fhPath.getGraphicStyleId());
  unsigned contentId = _findContentId(fhPath.getGraphicStyleId());
  if (fhPath.getEvenOdd())
    propList.insert("svg:fill-rule", "evenodd");

  unsigned short xform = fhPath.getXFormId();

  if (xform)
  {
    const FHTransform *trafo = _findTransform(xform);
    if (trafo)
      fhPath.transform(*trafo);
  }
  std::stack<FHTransform> groupTransforms(m_currentTransforms);
  while (!groupTransforms.empty())
  {
    fhPath.transform(groupTransforms.top());
    groupTransforms.pop();
  }
  _normalizePath(fhPath);

  std::stack<FHTransform> fakeTransforms(m_fakeTransforms);
  while (!fakeTransforms.empty())
  {
    fhPath.transform(fakeTransforms.top());
    fakeTransforms.pop();
  }

  librevenge::RVNGPropertyListVector propVec;
  fhPath.writeOut(propVec);
  if (propList["draw:fill"] && propList["draw:fill"]->getStr() != "none")
    _composePath(propVec, true);
  else
    _composePath(propVec, fhPath.isClosed());
  librevenge::RVNGPropertyList pList;
  pList.insert("svg:d", propVec);
  if (contentId)
    painter->openGroup(::librevenge::RVNGPropertyList());
  painter->setStyle(propList);
  painter->drawPath(pList);
  if (contentId)
  {
    FHBoundingBox bBox;
    fhPath.getBoundingBox(bBox.m_xmin, bBox.m_ymin, bBox.m_xmax, bBox.m_ymax);
    FHTransform trafo(1.0, 0.0, 0.0, 1.0, - bBox.m_xmin, - bBox.m_ymin);
    m_fakeTransforms.push(trafo);
    librevenge::RVNGStringVector svgOutput;
    librevenge::RVNGSVGDrawingGenerator generator(svgOutput, "");
    propList.clear();
    propList.insert("svg:width", bBox.m_xmax - bBox.m_xmin);
    propList.insert("svg:height", bBox.m_ymax - bBox.m_ymin);
    generator.startPage(propList);
    _outputSomething(contentId, &generator);
    generator.endPage();
    if (!svgOutput.empty() && svgOutput[0].size() > 140) // basically empty svg if it is not fullfilled
    {
      const char *header =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
      librevenge::RVNGBinaryData output((const unsigned char *)header, strlen(header));
      output.append((unsigned char *)svgOutput[0].cstr(), strlen(svgOutput[0].cstr()));
#if DUMP_CONTENTS
      {
        librevenge::RVNGString filename;
        filename.sprintf("fhcontents%.4x.svg", contentId);
        FILE *f = fopen(filename.cstr(), "wb");
        if (f)
        {
          const unsigned char *tmpBuffer = output.getDataBuffer();
          for (unsigned long k = 0; k < output.size(); k++)
            fprintf(f, "%c",tmpBuffer[k]);
          fclose(f);
        }
      }
#endif
      propList.clear();
      propList.insert("draw:stroke", "none");
      propList.insert("draw:fill", "bitmap");
      propList.insert("librevenge:mime-type", "image/svg+xml");
      propList.insert("style:repeat", "stretch");
      propList.insert("draw:fill-image", output.getBase64Data());
      painter->setStyle(propList);
      painter->drawPath(pList);
    }
    if (!m_fakeTransforms.empty())
      m_fakeTransforms.pop();
    painter->closeGroup();
  }
#if DEBUG_BOUNDING_BOX
  {
    librevenge::RVNGPropertyList rectangleProps;
    rectangleProps.insert("draw:fill", "none");
    rectangleProps.insert("draw:stroke", "solid");
    painter->setStyle(rectangleProps);
    double xmin, ymin, xmax, ymax;
    fhPath.getBoundingBox(xmin, ymin, xmax, ymax);
    librevenge::RVNGPropertyList rectangleList;
    rectangleList.insert("svg:x", xmin);
    rectangleList.insert("svg:y", ymin);
    rectangleList.insert("svg:width", xmax - xmin);
    rectangleList.insert("svg:height", ymax - ymin);
    painter->drawRectangle(rectangleList);
  }
#endif
}

void libfreehand::FHCollector::_outputSomething(unsigned somethingId, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !somethingId)
    return;

  _outputGroup(_findGroup(somethingId), painter);
  _outputClipGroup(_findClipGroup(somethingId), painter);
  _outputPath(_findPath(somethingId), painter);
  _outputCompositePath(_findCompositePath(somethingId), painter);
  _outputTextObject(_findTextObject(somethingId), painter);
  _outputDisplayText(_findDisplayText(somethingId), painter);
  _outputImageImport(_findImageImport(somethingId), painter);
  _outputNewBlend(_findNewBlend(somethingId), painter);
  _outputSymbolInstance(_findSymbolInstance(somethingId), painter);
}

void libfreehand::FHCollector::_outputGroup(const libfreehand::FHGroup *group, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !group)
    return;

  if (group->m_xFormId)
  {
    const FHTransform *trafo = _findTransform(group->m_xFormId);
    if (trafo)
      m_currentTransforms.push(*trafo);
    else
      m_currentTransforms.push(libfreehand::FHTransform());
  }
  else
    m_currentTransforms.push(libfreehand::FHTransform());

  const std::vector<unsigned> *elements = _findListElements(group->m_elementsId);
  if (!elements)
  {
    FH_DEBUG_MSG(("ERROR: The pointed element list does not exist\n"));
    return;
  }

  if (!elements->empty())
  {
    painter->openGroup(::librevenge::RVNGPropertyList());
    for (std::vector<unsigned>::const_iterator iterVec = elements->begin(); iterVec != elements->end(); ++iterVec)
      _outputSomething(*iterVec, painter);
    painter->closeGroup();
  }

  if (!m_currentTransforms.empty())
    m_currentTransforms.pop();
}

void libfreehand::FHCollector::_outputClipGroup(const libfreehand::FHGroup *group, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !group)
    return;

  const std::vector<unsigned> *elements = _findListElements(group->m_elementsId);
  if (!elements)
  {
    FH_DEBUG_MSG(("ERROR: The pointed element list does not exist\n"));
    return;
  }

  if (!elements->empty())
  {
    std::vector<unsigned>::const_iterator iter = elements->begin();
    const FHPath *path = _findPath(*iter);
    if (!path)
      _outputGroup(group, painter);
    else
    {
      if (group->m_xFormId)
      {
        const FHTransform *trafo = _findTransform(group->m_xFormId);
        if (trafo)
          m_currentTransforms.push(*trafo);
        else
          m_currentTransforms.push(libfreehand::FHTransform());
      }
      else
        m_currentTransforms.push(libfreehand::FHTransform());

      librevenge::RVNGPropertyList propList;
      FHPath fhPath(*path);
      _appendStrokeProperties(propList, fhPath.getGraphicStyleId());
      _appendFillProperties(propList, fhPath.getGraphicStyleId());
      if (fhPath.getEvenOdd())
        propList.insert("svg:fill-rule", "evenodd");
      unsigned short xform = fhPath.getXFormId();

      if (xform)
      {
        const FHTransform *trafo = _findTransform(xform);
        if (trafo)
          fhPath.transform(*trafo);
      }
      std::stack<FHTransform> groupTransforms(m_currentTransforms);
      while (!groupTransforms.empty())
      {
        fhPath.transform(groupTransforms.top());
        groupTransforms.pop();
      }
      _normalizePath(fhPath);

      std::stack<FHTransform> fakeTransforms(m_fakeTransforms);
      while (!fakeTransforms.empty())
      {
        fhPath.transform(fakeTransforms.top());
        fakeTransforms.pop();
      }

      if (!m_currentTransforms.empty())
        m_currentTransforms.pop();

      librevenge::RVNGPropertyListVector propVec;
      fhPath.writeOut(propVec);
      _composePath(propVec, true);
      librevenge::RVNGPropertyList pList;
      pList.insert("svg:d", propVec);


      FHBoundingBox bBox;
      fhPath.getBoundingBox(bBox.m_xmin, bBox.m_ymin, bBox.m_xmax, bBox.m_ymax);
      FHTransform trafo(1.0, 0.0, 0.0, 1.0, - bBox.m_xmin, - bBox.m_ymin);
      m_fakeTransforms.push(trafo);
      librevenge::RVNGStringVector svgOutput;
      librevenge::RVNGSVGDrawingGenerator generator(svgOutput, "");
      propList.clear();
      propList.insert("svg:width", bBox.m_xmax - bBox.m_xmin);
      propList.insert("svg:height", bBox.m_ymax - bBox.m_ymin);
      generator.startPage(propList);
      _outputGroup(group, &generator);
      generator.endPage();
      if (!svgOutput.empty() && svgOutput[0].size() > 140) // basically empty svg if it is not fullfilled
      {
        const char *header =
          "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
        librevenge::RVNGBinaryData output((const unsigned char *)header, strlen(header));
        output.append((unsigned char *)svgOutput[0].cstr(), strlen(svgOutput[0].cstr()));
        propList.insert("draw:stroke", "none");
        propList.insert("draw:fill", "bitmap");
        propList.insert("librevenge:mime-type", "image/svg+xml");
        propList.insert("style:repeat", "stretch");
        propList.insert("draw:fill-image", output.getBase64Data());
        painter->setStyle(propList);
        painter->drawPath(pList);
      }
      if (!m_fakeTransforms.empty())
        m_fakeTransforms.pop();
    }
  }
}

void libfreehand::FHCollector::_outputNewBlend(const libfreehand::FHNewBlend *newBlend, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !newBlend)
    return;

  m_currentTransforms.push(libfreehand::FHTransform());

  painter->openGroup(::librevenge::RVNGPropertyList());
  const std::vector<unsigned> *elements1 = _findListElements(newBlend->m_list1Id);
  if (elements1 && !elements1->empty())
  {
    for (std::vector<unsigned>::const_iterator iterVec = elements1->begin(); iterVec != elements1->end(); ++iterVec)
      _outputSomething(*iterVec, painter);
  }
  const std::vector<unsigned> *elements2 = _findListElements(newBlend->m_list2Id);
  if (elements2 && !elements2->empty())
  {
    for (std::vector<unsigned>::const_iterator iterVec = elements2->begin(); iterVec != elements2->end(); ++iterVec)
      _outputSomething(*iterVec, painter);
  }
  const std::vector<unsigned> *elements3 = _findListElements(newBlend->m_list3Id);
  if (elements3 && !elements3->empty())
  {
    for (std::vector<unsigned>::const_iterator iterVec = elements3->begin(); iterVec != elements3->end(); ++iterVec)
      _outputSomething(*iterVec, painter);
  }
  painter->closeGroup();

  if (!m_currentTransforms.empty())
    m_currentTransforms.pop();
}

void libfreehand::FHCollector::_outputSymbolInstance(const libfreehand::FHSymbolInstance *symbolInstance, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !symbolInstance)
    return;

  m_currentTransforms.push(symbolInstance->m_xForm);

  const FHSymbolClass *symbolClass = _findSymbolClass(symbolInstance->m_symbolClassId);
  if (symbolClass)
  {
    _outputSomething(symbolClass->m_groupId, painter);
  }

  if (!m_currentTransforms.empty())
    m_currentTransforms.pop();
}

void libfreehand::FHCollector::outputDrawing(::librevenge::RVNGDrawingInterface *painter)
{

#if DUMP_BINARY_OBJECTS
  for (std::map<unsigned, FHImageImport>::const_iterator iterImage = m_images.begin(); iterImage != m_images.end(); ++iterImage)
  {
    librevenge::RVNGBinaryData data = getImageData(iterImage->second.m_dataListId);
    librevenge::RVNGString filename;
    filename.sprintf("freehanddump%.4x.%s", iterImage->first, iterImage->second.m_format.empty() ? "bin" : iterImage->second.m_format.cstr());
    FILE *f = fopen(filename.cstr(), "wb");
    if (f)
    {
      const unsigned char *tmpBuffer = data.getDataBuffer();
      for (unsigned long k = 0; k < data.size(); k++)
        fprintf(f, "%c",tmpBuffer[k]);
      fclose(f);
    }
  }
#endif

  if (!painter)
    return;

  if (!m_fhTail.m_blockId || m_fhTail.m_blockId != m_block.first)
  {
    FH_DEBUG_MSG(("WARNING: FHTail points to an invalid Block ID\n"));
    m_fhTail.m_blockId = m_block.first;
  }
  if (!m_fhTail.m_blockId)
  {
    FH_DEBUG_MSG(("ERROR: Block record is absent from this file\n"));
    return;
  }

  if (FH_UNINITIALIZED(m_pageInfo))
    m_pageInfo = m_fhTail.m_pageInfo;

  painter->startDocument(librevenge::RVNGPropertyList());
  librevenge::RVNGPropertyList propList;
  propList.insert("svg:height", m_pageInfo.m_maxY - m_pageInfo.m_minY);
  propList.insert("svg:width", m_pageInfo.m_maxX - m_pageInfo.m_minX);
  painter->startPage(propList);

  unsigned layerListId = m_block.second.m_layerListId;

  const std::vector<unsigned> *elements = _findListElements(layerListId);
  if (elements)
  {
    for (std::vector<unsigned>::const_iterator iter = elements->begin(); iter != elements->end(); ++iter)
    {
      _outputLayer(*iter, painter);
    }
  }
  painter->endPage();
  painter->endDocument();
}

void libfreehand::FHCollector::_outputLayer(unsigned layerId, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter)
    return;

  std::map<unsigned, FHLayer>::const_iterator layerIter = m_layers.find(layerId);
  if (layerIter == m_layers.end())
  {
    FH_DEBUG_MSG(("ERROR: Could not find the referenced layer\n"));
    return;
  }

  if (layerIter->second.m_visibility != 3)
    return;

  unsigned layerElementsListId = layerIter->second.m_elementsId;
  if (!layerElementsListId)
  {
    FH_DEBUG_MSG(("ERROR: Layer points to invalid element list\n"));
    return;
  }

  const std::vector<unsigned> *elements = _findListElements(layerElementsListId);
  if (!elements)
  {
    FH_DEBUG_MSG(("ERROR: The pointed element list does not exist\n"));
    return;
  }

  for (std::vector<unsigned>::const_iterator iterVec = elements->begin(); iterVec != elements->end(); ++iterVec)
    _outputSomething(*iterVec, painter);
}

void libfreehand::FHCollector::_outputCompositePath(const libfreehand::FHCompositePath *compositePath, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !compositePath)
    return;

  const std::vector<unsigned> *elements = _findListElements(compositePath->m_elementsId);
  if (elements && !elements->empty())
  {
    libfreehand::FHPath fhPath;
    std::vector<unsigned>::const_iterator iter = elements->begin();
    const libfreehand::FHPath *path = _findPath(*(iter++));
    if (path)
    {
      fhPath = *path;
      if (!fhPath.getGraphicStyleId())
        fhPath.setGraphicStyleId(compositePath->m_graphicStyleId);
    }

    for (; iter != elements->end(); ++iter)
    {
      path = _findPath(*iter);
      if (path)
      {
        fhPath.appendPath(*path);
        if (!fhPath.getGraphicStyleId())
          fhPath.setGraphicStyleId(compositePath->m_graphicStyleId);
      }
    }
    _outputPath(&fhPath, painter);
  }
}

void libfreehand::FHCollector::_outputTextObject(const libfreehand::FHTextObject *textObject, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !textObject)
    return;

  double xa = textObject->m_startX;
  double ya = textObject->m_startY;
  double xb = textObject->m_startX + textObject->m_width;
  double yb = textObject->m_startY + textObject->m_height;
  double xc = xa;
  double yc = yb;
  unsigned xFormId = textObject->m_xFormId;
  if (xFormId)
  {
    const FHTransform *trafo = _findTransform(xFormId);
    if (trafo)
    {
      trafo->applyToPoint(xa, ya);
      trafo->applyToPoint(xb, yb);
      trafo->applyToPoint(xc, yc);
    }
  }
  std::stack<FHTransform> groupTransforms(m_currentTransforms);
  while (!groupTransforms.empty())
  {
    groupTransforms.top().applyToPoint(xa, ya);
    groupTransforms.top().applyToPoint(xb, yb);
    groupTransforms.top().applyToPoint(xc, yc);
    groupTransforms.pop();
  }
  _normalizePoint(xa, ya);
  _normalizePoint(xb, yb);
  _normalizePoint(xc, yc);

  std::stack<FHTransform> fakeTransforms(m_fakeTransforms);
  while (!fakeTransforms.empty())
  {
    fakeTransforms.top().applyToPoint(xa, ya);
    fakeTransforms.top().applyToPoint(xb, yb);
    fakeTransforms.top().applyToPoint(xc, yc);
    fakeTransforms.pop();
  }

  double rotation = atan2(yb-yc, xb-xc);
  double height = sqrt((xc-xa)*(xc-xa) + (yc-ya)*(yc-ya));
  double width = sqrt((xc-xb)*(xc-xb) + (yc-yb)*(yc-yb));
  double xmid = (xa + xb) / 2.0;
  double ymid = (ya + yb) / 2.0;

  ::librevenge::RVNGPropertyList textObjectProps;
  textObjectProps.insert("svg:x", xmid - textObject->m_width / 2.0);
  textObjectProps.insert("svg:y", ymid + textObject->m_height / 2.0);
  textObjectProps.insert("svg:height", height);
  textObjectProps.insert("svg:width", width);
  if (!FH_ALMOST_ZERO(rotation))
    textObjectProps.insert("librevenge:rotate", rotation * 180.0 / M_PI);
  painter->startTextObject(textObjectProps);

  const std::vector<unsigned> *elements = _findTStringElements(textObject->m_tStringId);
  if (elements && !elements->empty())
  {
    for (std::vector<unsigned>::const_iterator iter = elements->begin(); iter != elements->end(); ++iter)
      _outputParagraph(_findParagraph(*iter), painter);
  }

  painter->endTextObject();
}

void libfreehand::FHCollector::_outputParagraph(const libfreehand::FHParagraph *paragraph, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !paragraph)
    return;
  librevenge::RVNGPropertyList propList;
  _appendParagraphProperties(propList, paragraph->m_paraStyleId);
  painter->openParagraph(propList);

  std::map<unsigned, std::vector<unsigned short> >::const_iterator iter = m_textBloks.find(paragraph->m_textBlokId);
  if (iter != m_textBloks.end())
  {

    for (std::vector<std::pair<unsigned, unsigned> >::size_type i = 0; i < paragraph->m_charStyleIds.size(); ++i)
    {
      _outputTextRun(&(iter->second), paragraph->m_charStyleIds[i].first,
                     (i+1 < paragraph->m_charStyleIds.size() ? paragraph->m_charStyleIds[i+1].first : iter->second.size()) - paragraph->m_charStyleIds[i].first,
                     paragraph->m_charStyleIds[i].second, painter);
    }

  }

  painter->closeParagraph();
}

void libfreehand::FHCollector::_appendCharacterProperties(::librevenge::RVNGPropertyList &propList, unsigned charPropsId)
{
  std::map<unsigned, FHCharProperties>::const_iterator iter = m_charProperties.find(charPropsId);
  if (iter == m_charProperties.end())
    return;
  const FHCharProperties &charProps = iter->second;
  if (charProps.m_fontNameId)
  {
    std::map<unsigned, ::librevenge::RVNGString>::const_iterator iterString = m_strings.find(charProps.m_fontNameId);
    if (iterString != m_strings.end())
      propList.insert("fo:font-name", iterString->second);
  }
  propList.insert("fo:font-size", charProps.m_fontSize, librevenge::RVNG_POINT);
  if (charProps.m_fontId)
    _appendFontProperties(propList, charProps.m_fontId);
  if (charProps.m_textColorId)
  {
    std::map<unsigned, FHBasicFill>::const_iterator iterBasicFill = m_basicFills.find(charProps.m_textColorId);
    if (iterBasicFill != m_basicFills.end() && iterBasicFill->second.m_colorId)
    {
      librevenge::RVNGString color = getColorString(iterBasicFill->second.m_colorId);
      if (!color.empty())
        propList.insert("fo:color", color);
    }
  }
  propList.insert("style:text-scale", charProps.m_horizontalScale, librevenge::RVNG_PERCENT);
}

void libfreehand::FHCollector::_appendCharacterProperties(::librevenge::RVNGPropertyList &propList, const FH3CharProperties &charProps)
{
  if (charProps.m_fontNameId)
  {
    std::map<unsigned, ::librevenge::RVNGString>::const_iterator iterString = m_strings.find(charProps.m_fontNameId);
    if (iterString != m_strings.end())
      propList.insert("fo:font-name", iterString->second);
  }
  propList.insert("fo:font-size", charProps.m_fontSize, librevenge::RVNG_POINT);
  if (charProps.m_fontColorId)
  {
    librevenge::RVNGString color = getColorString(charProps.m_fontColorId);
    if (!color.empty())
      propList.insert("fo:color", color);
  }
  if (charProps.m_fontStyle & 1)
    propList.insert("fo:font-weight", "bold");
  if (charProps.m_fontStyle & 2)
    propList.insert("fo:font-style", "italic");
}

void libfreehand::FHCollector::_appendParagraphProperties(::librevenge::RVNGPropertyList & /* propList */, const FH3ParaProperties & /* paraProps */)
{
}

void libfreehand::FHCollector::_appendParagraphProperties(::librevenge::RVNGPropertyList & /* propList */, unsigned /* paraPropsId */)
{
}

void libfreehand::FHCollector::_outputDisplayText(const libfreehand::FHDisplayText *displayText, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !displayText)
    return;

  double xa = displayText->m_startX;
  double ya = displayText->m_startY;
  double xb = displayText->m_startX + displayText->m_width;
  double yb = displayText->m_startY + displayText->m_height;
  double xc = xa;
  double yc = yb;
  unsigned xFormId = displayText->m_xFormId;
  if (xFormId)
  {
    const FHTransform *trafo = _findTransform(xFormId);
    if (trafo)
    {
      trafo->applyToPoint(xa, ya);
      trafo->applyToPoint(xb, yb);
      trafo->applyToPoint(xc, yc);
    }
  }
  std::stack<FHTransform> groupTransforms(m_currentTransforms);
  while (!groupTransforms.empty())
  {
    groupTransforms.top().applyToPoint(xa, ya);
    groupTransforms.top().applyToPoint(xb, yb);
    groupTransforms.top().applyToPoint(xc, yc);
    groupTransforms.pop();
  }
  _normalizePoint(xa, ya);
  _normalizePoint(xb, yb);
  _normalizePoint(xc, yc);

  std::stack<FHTransform> fakeTransforms(m_fakeTransforms);
  while (!fakeTransforms.empty())
  {
    fakeTransforms.top().applyToPoint(xa, ya);
    fakeTransforms.top().applyToPoint(xb, yb);
    fakeTransforms.top().applyToPoint(xc, yc);
    fakeTransforms.pop();
  }

  double rotation = atan2(yb-yc, xb-xc);
  double height = sqrt((xc-xa)*(xc-xa) + (yc-ya)*(yc-ya));
  double width = sqrt((xc-xb)*(xc-xb) + (yc-yb)*(yc-yb));
  double xmid = (xa + xb) / 2.0;
  double ymid = (ya + yb) / 2.0;

  ::librevenge::RVNGPropertyList textObjectProps;
  textObjectProps.insert("svg:x", xmid - displayText->m_width / 2.0);
  textObjectProps.insert("svg:y", ymid + displayText->m_height / 2.0);
  textObjectProps.insert("svg:height", height);
  textObjectProps.insert("svg:width", width);
  if (!FH_ALMOST_ZERO(rotation))
    textObjectProps.insert("librevenge:rotate", rotation * 180.0 / M_PI);

  painter->startTextObject(textObjectProps);

  std::vector<FH3ParaProperties>::const_iterator iterPara = displayText->m_paraProps.begin();
  std::vector<FH3CharProperties>::const_iterator iterChar = displayText->m_charProps.begin();

  FH3ParaProperties paraProps = *iterPara++;
  FH3CharProperties charProps = *iterChar++;
  librevenge::RVNGString text;
  std::vector<unsigned char>::size_type i = 0;

  librevenge::RVNGPropertyList paraPropList;
  _appendParagraphProperties(paraPropList, paraProps);
  painter->openParagraph(paraPropList);
  bool isParagraphOpened = true;

  librevenge::RVNGPropertyList charPropList;
  _appendCharacterProperties(charPropList, charProps);
  painter->openSpan(charPropList);
  bool isSpanOpened = true;

  while (i < displayText->m_characters.size())
  {
    _appendMacRoman(text, displayText->m_characters[i++]);
    if (i > paraProps.m_offset)
    {
      if (!text.empty())
        painter->insertText(text);
      text.clear();
      if (isParagraphOpened)
      {
        if (isSpanOpened)
        {
          painter->closeSpan();
          isSpanOpened = false;
        }
        painter->closeParagraph();
        isParagraphOpened = false;
      }
      if (iterPara != displayText->m_paraProps.end())
      {
        paraProps = *iterPara++;
      }
    }
    if (i > charProps.m_offset)
    {
      if (!text.empty())
        painter->insertText(text);
      text.clear();
      if (isSpanOpened)
      {
        painter->closeSpan();
        isSpanOpened = false;
      }
      if (iterChar != displayText->m_charProps.end())
      {
        charProps = *iterChar++;
      }
    }
    if (i >= displayText->m_characters.size())
      break;
    if (!isParagraphOpened)
    {
      paraPropList.clear();
      _appendParagraphProperties(paraPropList, paraProps);
      painter->openParagraph(paraPropList);
      isParagraphOpened = true;
      if (!isSpanOpened)
      {
        charPropList.clear();
        _appendCharacterProperties(charPropList, charProps);
        painter->openSpan(charPropList);
        isSpanOpened = true;
      }
    }
    if (!isSpanOpened)
    {
      charPropList.clear();
      _appendCharacterProperties(charPropList, charProps);
      painter->openSpan(charPropList);
      isSpanOpened = true;
    }
  }
  if (!text.empty())
    painter->insertText(text);
  if (isSpanOpened)
    painter->closeSpan();
  if (isParagraphOpened)
    painter->closeParagraph();

  painter->endTextObject();
}

void libfreehand::FHCollector::_outputImageImport(const FHImageImport *image, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !image)
    return;

  librevenge::RVNGPropertyList propList;
  _appendStrokeProperties(propList, image->m_graphicStyleId);
  _appendFillProperties(propList, image->m_graphicStyleId);
  double xa = image->m_startX;
  double ya = image->m_startY;
  double xb = image->m_startX + image->m_width;
  double yb = image->m_startY + image->m_height;
  double xc = xa;
  double yc = yb;
  unsigned xFormId = image->m_xFormId;
  if (xFormId)
  {
    const FHTransform *trafo = _findTransform(xFormId);
    if (trafo)
    {
      trafo->applyToPoint(xa, ya);
      trafo->applyToPoint(xb, yb);
      trafo->applyToPoint(xc, yc);
    }
  }
  std::stack<FHTransform> groupTransforms(m_currentTransforms);
  while (!groupTransforms.empty())
  {
    groupTransforms.top().applyToPoint(xa, ya);
    groupTransforms.top().applyToPoint(xb, yb);
    groupTransforms.top().applyToPoint(xc, yc);
    groupTransforms.pop();
  }
  _normalizePoint(xa, ya);
  _normalizePoint(xb, yb);
  _normalizePoint(xc, yc);
  std::stack<FHTransform> fakeTransforms(m_fakeTransforms);
  while (!fakeTransforms.empty())
  {
    fakeTransforms.top().applyToPoint(xa, ya);
    fakeTransforms.top().applyToPoint(xb, yb);
    fakeTransforms.top().applyToPoint(xc, yc);
    fakeTransforms.pop();
  }

  double rotation = atan2(yb-yc, xb-xc);
  double height = sqrt((xc-xa)*(xc-xa) + (yc-ya)*(yc-ya));
  double width = sqrt((xc-xb)*(xc-xb) + (yc-yb)*(yc-yb));
  double xmid = (xa + xb) / 2.0;
  double ymid = (ya + yb) / 2.0;

  ::librevenge::RVNGPropertyList imageProps;
  imageProps.insert("svg:x", xmid - width / 2.0);
  imageProps.insert("svg:y", ymid - height / 2.0);
  imageProps.insert("svg:height", height);
  imageProps.insert("svg:width", width);
  if (!FH_ALMOST_ZERO(rotation))
    imageProps.insert("librevenge:rotate", rotation * 180.0 / M_PI);

  imageProps.insert("librevenge:mime-type", "whatever");
  librevenge::RVNGBinaryData data = getImageData(image->m_dataListId);

  if (data.empty())
    return;

  const unsigned char *buffer = data.getDataBuffer();
  unsigned long size = data.size();
  if (isTiff(buffer, size))
    imageProps.insert("librevenge:mime-type", "image/tiff");
  else if (isBmp(buffer, size))
    imageProps.insert("librevenge:mime-type", "image/bmp");
  else if (isJpeg(buffer, size))
    imageProps.insert("librevenge:mime-type", "image/jpeg");
  else if (isPng(buffer, size))
    imageProps.insert("librevenge:mime-type", "image/png");

  imageProps.insert("office:binary-data", data);
  painter->setStyle(propList);
  painter->drawGraphicObject(imageProps);
}

void libfreehand::FHCollector::_outputTextRun(const std::vector<unsigned short> *characters, unsigned offset, unsigned length,
                                              unsigned charStyleId, ::librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !characters || characters->empty())
    return;
  librevenge::RVNGPropertyList propList;
  _appendCharacterProperties(propList, charStyleId);
  painter->openSpan(propList);
  std::vector<unsigned short> tmpChars;
  for (unsigned i = offset; i < length+offset && i < characters->size(); ++i)
    tmpChars.push_back((*characters)[i]);
  if (!tmpChars.empty())
  {
    librevenge::RVNGString text;
    _appendUTF16(text, tmpChars);
    painter->insertText(text);
  }
  painter->closeSpan();
}

const std::vector<unsigned> *libfreehand::FHCollector::_findListElements(unsigned id)
{
  std::map<unsigned, FHList>::const_iterator iter = m_lists.find(id);
  if (iter != m_lists.end())
    return &(iter->second.m_elements);
  return 0;
}


void libfreehand::FHCollector::_appendFontProperties(::librevenge::RVNGPropertyList &propList, unsigned agdFontId)
{
  std::map<unsigned, FHAGDFont>::const_iterator iter = m_fonts.find(agdFontId);
  if (iter == m_fonts.end())
    return;
  const FHAGDFont &font = iter->second;
  if (font.m_fontNameId)
  {
    std::map<unsigned, ::librevenge::RVNGString>::const_iterator iterString = m_strings.find(font.m_fontNameId);
    if (iterString != m_strings.end())
      propList.insert("fo:font-name", iterString->second);
  }
  propList.insert("fo:font-size", font.m_fontSize, librevenge::RVNG_POINT);
  if (font.m_fontStyle & 1)
    propList.insert("fo:font-weight", "bold");
  if (font.m_fontStyle & 2)
    propList.insert("fo:font-style", "italic");
}

void libfreehand::FHCollector::_appendFillProperties(::librevenge::RVNGPropertyList &propList, unsigned graphicStyleId)
{
  if (!propList["draw:fill"])
    propList.insert("draw:fill", "none");
  if (graphicStyleId)
  {
    const FHPropList *propertyList = _findPropList(graphicStyleId);
    if (propertyList)
    {
      if (propertyList->m_parentId)
        _appendFillProperties(propList, propertyList->m_parentId);
      std::map<unsigned, unsigned>::const_iterator iter = propertyList->m_elements.find(m_fillId);
      if (iter != propertyList->m_elements.end())
      {
        _appendBasicFill(propList, _findBasicFill(iter->second));
        _appendLinearFill(propList, _findLinearFill(iter->second));
        _appendLensFill(propList, _findLensFill(iter->second));
        _appendRadialFill(propList, _findRadialFill(iter->second));
        _appendTileFill(propList, _findTileFill(iter->second));
        _appendPatternFill(propList, _findPatternFill(iter->second));
      }
    }
    else
    {
      const FHGraphicStyle *graphicStyle = _findGraphicStyle(graphicStyleId);
      if (graphicStyle)
      {
        if (graphicStyle->m_parentId)
          _appendFillProperties(propList, graphicStyle->m_parentId);
        unsigned fillId = _findFillId(*graphicStyle);;
        if (fillId)
        {
          _appendBasicFill(propList, _findBasicFill(fillId));
          _appendLinearFill(propList, _findLinearFill(fillId));
          _appendLensFill(propList, _findLensFill(fillId));
          _appendRadialFill(propList, _findRadialFill(fillId));
          _appendTileFill(propList, _findTileFill(fillId));
          _appendPatternFill(propList, _findPatternFill(fillId));
        }
        else
        {
          const FHFilterAttributeHolder *filterAttributeHolder = _findFilterAttributeHolder(*graphicStyle);
          if (filterAttributeHolder)
          {
            if (filterAttributeHolder->m_graphicStyleId)
              _appendFillProperties(propList, filterAttributeHolder->m_graphicStyleId);
            if (filterAttributeHolder->m_filterId)
              _applyFilter(propList, filterAttributeHolder->m_filterId);
          }
        }
      }
    }
  }
}

void libfreehand::FHCollector::_appendStrokeProperties(::librevenge::RVNGPropertyList &propList, unsigned graphicStyleId)
{
  if (!propList["draw:stroke"])
    propList.insert("draw:stroke", "none");
  if (graphicStyleId)
  {
    const FHPropList *propertyList = _findPropList(graphicStyleId);
    if (propertyList)
    {
      if (propertyList->m_parentId)
        _appendStrokeProperties(propList, propertyList->m_parentId);
      std::map<unsigned, unsigned>::const_iterator iter = propertyList->m_elements.find(m_strokeId);
      if (iter != propertyList->m_elements.end())
      {
        _appendBasicLine(propList, _findBasicLine(iter->second));
      }
    }
    else
    {
      const FHGraphicStyle *graphicStyle = _findGraphicStyle(graphicStyleId);
      if (graphicStyle)
      {
        if (graphicStyle->m_parentId)
          _appendStrokeProperties(propList, graphicStyle->m_parentId);
        unsigned strokeId = _findStrokeId(*graphicStyle);
        if (strokeId)
          _appendBasicLine(propList, _findBasicLine(strokeId));
        else
        {
          const FHFilterAttributeHolder *filterAttributeHolder = _findFilterAttributeHolder(*graphicStyle);
          if (filterAttributeHolder)
          {
            if (filterAttributeHolder->m_graphicStyleId)
              _appendFillProperties(propList, filterAttributeHolder->m_graphicStyleId);
            if (filterAttributeHolder->m_filterId)
              _applyFilter(propList, filterAttributeHolder->m_filterId);
          }
        }
      }
    }
  }
}

void libfreehand::FHCollector::_appendBasicFill(::librevenge::RVNGPropertyList &propList, const libfreehand::FHBasicFill *basicFill)
{
  if (!basicFill)
    return;
  propList.insert("draw:fill", "solid");
  librevenge::RVNGString color = getColorString(basicFill->m_colorId);
  if (!color.empty())
    propList.insert("draw:fill-color", color);
  else
    propList.insert("draw:fill-color", "#000000");
}

unsigned libfreehand::FHCollector::_findContentId(unsigned graphicStyleId)
{
  if (graphicStyleId)
  {
    const FHPropList *propertyList = _findPropList(graphicStyleId);
    if (propertyList)
    {
      std::map<unsigned, unsigned>::const_iterator iter = propertyList->m_elements.find(m_contentId);
      if (iter != propertyList->m_elements.end())
        return iter->second;
    }
    else
    {
      const FHGraphicStyle *graphicStyle = _findGraphicStyle(graphicStyleId);
      if (graphicStyle)
      {
        std::map<unsigned, unsigned>::const_iterator iter = graphicStyle->m_elements.find(m_contentId);
        if (iter != graphicStyle->m_elements.end())
          return iter->second;
      }
    }
  }
  return 0;
}

void libfreehand::FHCollector::_appendLinearFill(::librevenge::RVNGPropertyList &propList, const libfreehand::FHLinearFill *linearFill)
{
  if (!linearFill)
    return;
  propList.insert("draw:fill", "gradient");
  propList.insert("draw:style", "linear");
  double angle = 90.0 - linearFill->m_angle;
  while (angle < 0.0)
    angle += 360.0;
  while (angle > 360.0)
    angle -= 360.0;
  propList.insert("draw:angle", angle, librevenge::RVNG_GENERIC);

  const std::vector<FHColorStop> *multiColorList = _findMultiColorList(linearFill->m_multiColorListId);
  if (multiColorList && multiColorList->size() > 1)
  {
    librevenge::RVNGString color = getColorString((*multiColorList)[0].m_colorId);
    if (!color.empty())
      propList.insert("draw:start-color", color);
    color = getColorString((*multiColorList)[1].m_colorId);
    if (!color.empty())
      propList.insert("draw:end-color", color);
  }
  else
  {
    librevenge::RVNGString color = getColorString(linearFill->m_color1Id);
    if (!color.empty())
      propList.insert("draw:start-color", color);
    color = getColorString(linearFill->m_color2Id);
    if (!color.empty())
      propList.insert("draw:end-color", color);
  }
}

void libfreehand::FHCollector::_applyFilter(::librevenge::RVNGPropertyList &propList, unsigned filterId)
{
  if (!filterId)
    return;
  _appendOpacity(propList, _findOpacityFilter(filterId));
  _appendShadow(propList, _findFWShadowFilter(filterId));
  _appendGlow(propList, _findFWGlowFilter(filterId));
}

void libfreehand::FHCollector::_appendOpacity(::librevenge::RVNGPropertyList &propList, const double *opacity)
{
  if (!opacity)
    return;
  if (propList["draw:fill"] && propList["draw:fill"]->getStr() != "none")
    propList.insert("draw:opacity", *opacity, librevenge::RVNG_PERCENT);
  if (propList["draw:stroke"] && propList["draw:stroke"]->getStr() != "none")
    propList.insert("svg:stroke-opacity", *opacity, librevenge::RVNG_PERCENT);
}

void libfreehand::FHCollector::_appendShadow(::librevenge::RVNGPropertyList &propList, const libfreehand::FWShadowFilter *filter)
{
  if (!filter)
    return;
  if (!filter->m_inner)
  {
    propList.insert("draw:shadow","visible"); // for ODG
    propList.insert("draw:shadow-offset-x",filter->m_distribution * cos(M_PI * filter->m_angle / 180.0));
    propList.insert("draw:shadow-offset-y",filter->m_distribution * sin(M_PI * filter->m_angle / 180.0));
    propList.insert("draw:shadow-color",getColorString(filter->m_colorId));
    propList.insert("draw:shadow-opacity",filter->m_opacity, librevenge::RVNG_PERCENT);
  }
}

void libfreehand::FHCollector::_appendGlow(::librevenge::RVNGPropertyList & /* propList */, const libfreehand::FWGlowFilter *filter)
{
  if (!filter)
    return;
}

void libfreehand::FHCollector::_appendLensFill(::librevenge::RVNGPropertyList &propList, const libfreehand::FHLensFill *lensFill)
{
  if (!lensFill)
    return;

  if (lensFill->m_colorId)
  {
    propList.insert("draw:fill", "solid");
    librevenge::RVNGString color = getColorString(lensFill->m_colorId);
    if (!color.empty())
      propList.insert("draw:fill-color", color);
    else
      propList.insert("draw:fill", "none");
  }
  else
    propList.insert("draw:fill", "none");

  switch (lensFill->m_mode)
  {
  case FH_LENSFILL_MODE_TRANSPARENCY:
    propList.insert("draw:opacity", lensFill->m_value / 100.0, librevenge::RVNG_PERCENT);
    break;
  case FH_LENSFILL_MODE_MONOCHROME:
    propList.insert("draw:fill", "none");
    propList.insert("draw:color-mode", "greyscale");
    break;
  case FH_LENSFILL_MODE_MAGNIFY:
    propList.insert("draw:fill", "none");
    break;
  case FH_LENSFILL_MODE_LIGHTEN: // emulate by semi-transparent white fill
    propList.insert("draw:fill", "solid");
    propList.insert("draw:fill-color", "#FFFFFF");
    propList.insert("draw:opacity", lensFill->m_value / 100.0, librevenge::RVNG_PERCENT);
    break;
  case FH_LENSFILL_MODE_DARKEN: // emulate by semi-transparent black fill
    propList.insert("draw:fill", "solid");
    propList.insert("draw:fill-color", "#000000");
    propList.insert("draw:opacity", lensFill->m_value / 100.0, librevenge::RVNG_PERCENT);
    break;
  case FH_LENSFILL_MODE_INVERT:
    propList.insert("draw:fill", "none");
    break;
  default:
    break;
  }
}

void libfreehand::FHCollector::_appendRadialFill(::librevenge::RVNGPropertyList &propList, const libfreehand::FHRadialFill *radialFill)
{
  if (!radialFill)
    return;

  propList.insert("draw:fill", "gradient");
  propList.insert("draw:style", "radial");
  propList.insert("svg:cx", radialFill->m_cx, librevenge::RVNG_PERCENT);
  propList.insert("svg:cy", radialFill->m_cy, librevenge::RVNG_PERCENT);

  const std::vector<FHColorStop> *multiColorList = _findMultiColorList(radialFill->m_multiColorListId);
  if (multiColorList && multiColorList->size() > 1)
  {
    librevenge::RVNGString color = getColorString((*multiColorList)[0].m_colorId);
    if (!color.empty())
      propList.insert("draw:start-color", color);
    color = getColorString((*multiColorList)[1].m_colorId);
    if (!color.empty())
      propList.insert("draw:end-color", color);
  }
  else
  {
    librevenge::RVNGString color = getColorString(radialFill->m_color1Id);
    if (!color.empty())
      propList.insert("draw:start-color", color);
    color = getColorString(radialFill->m_color2Id);
    if (!color.empty())
      propList.insert("draw:end-color", color);
  }
}

void libfreehand::FHCollector::_appendTileFill(::librevenge::RVNGPropertyList &propList, const libfreehand::FHTileFill *tileFill)
{
  if (!tileFill || !(tileFill->m_groupId))
    return;

  const FHTransform *trafo = _findTransform(tileFill->m_xFormId);
  if (trafo)
    m_currentTransforms.push(*trafo);
  else
    m_currentTransforms.push(FHTransform());

  FHBoundingBox bBox;
  _getBBofSomething(tileFill->m_groupId, bBox);
  if (bBox.isValid() && !FH_ALMOST_ZERO(bBox.m_xmax - bBox.m_xmin) && !FH_ALMOST_ZERO(bBox.m_ymax - bBox.m_ymin))
  {
    FHTransform fakeTrafo(tileFill->m_scaleX, 0.0, 0.0, tileFill->m_scaleY, - bBox.m_xmin, -bBox.m_ymin);
    m_fakeTransforms.push(fakeTrafo);

    librevenge::RVNGStringVector svgOutput;
    librevenge::RVNGSVGDrawingGenerator generator(svgOutput, "");
    librevenge::RVNGPropertyList pList;
    pList.insert("svg:width", tileFill->m_scaleX * (bBox.m_xmax - bBox.m_xmin));
    pList.insert("svg:height", tileFill->m_scaleY * (bBox.m_ymax - bBox.m_ymin));
    generator.startPage(pList);

    _outputSomething(tileFill->m_groupId, &generator);
    generator.endPage();
    if (!svgOutput.empty() && svgOutput[0].size() > 140) // basically empty svg if it is not fullfilled
    {
      const char *header =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
      librevenge::RVNGBinaryData output((const unsigned char *)header, strlen(header));
      output.append((unsigned char *)svgOutput[0].cstr(), strlen(svgOutput[0].cstr()));
#if DUMP_TILE_FILLS
      {
        librevenge::RVNGString filename;
        filename.sprintf("freehandtilefill%.4x.svg", tileFill->m_groupId);
        FILE *f = fopen(filename.cstr(), "wb");
        if (f)
        {
          const unsigned char *tmpBuffer = output.getDataBuffer();
          for (unsigned long k = 0; k < output.size(); k++)
            fprintf(f, "%c",tmpBuffer[k]);
          fclose(f);
        }
      }
#endif
      propList.insert("draw:fill", "bitmap");
      propList.insert("draw:fill-image", output.getBase64Data());
      propList.insert("librevenge:mime-type", "image/svg+xml");
      propList.insert("style:repeat", "repeat");
    }

    if (!m_fakeTransforms.empty())
      m_fakeTransforms.pop();
  }
  if (!m_currentTransforms.empty())
    m_currentTransforms.pop();
}

void libfreehand::FHCollector::_appendPatternFill(::librevenge::RVNGPropertyList &propList, const libfreehand::FHPatternFill *patternFill)
{
  if (!patternFill)
    return;
  librevenge::RVNGBinaryData output;
  _generateBitmapFromPattern(output, patternFill->m_colorId, patternFill->m_pattern);
  propList.insert("draw:fill", "bitmap");
  propList.insert("draw:fill-image", output.getBase64Data());
  propList.insert("librevenge:mime-type", "image/bmp");
  propList.insert("style:repeat", "repeat");
}

void libfreehand::FHCollector::_appendBasicLine(::librevenge::RVNGPropertyList &propList, const libfreehand::FHBasicLine *basicLine)
{
  if (!basicLine)
    return;
  propList.insert("draw:stroke", "solid");
  librevenge::RVNGString color = getColorString(basicLine->m_colorId);
  if (!color.empty())
    propList.insert("svg:stroke-color", color);
  else
    propList.insert("svg:stroke-color", "#000000");
  propList.insert("svg:stroke-width", basicLine->m_width);
}

const libfreehand::FHPath *libfreehand::FHCollector::_findPath(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHPath>::const_iterator iter = m_paths.find(id);
  if (iter != m_paths.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHNewBlend *libfreehand::FHCollector::_findNewBlend(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHNewBlend>::const_iterator iter = m_newBlends.find(id);
  if (iter != m_newBlends.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHGroup *libfreehand::FHCollector::_findGroup(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHGroup>::const_iterator iter = m_groups.find(id);
  if (iter != m_groups.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHGroup *libfreehand::FHCollector::_findClipGroup(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHGroup>::const_iterator iter = m_clipGroups.find(id);
  if (iter != m_clipGroups.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHCompositePath *libfreehand::FHCollector::_findCompositePath(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHCompositePath>::const_iterator iter = m_compositePaths.find(id);
  if (iter != m_compositePaths.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHTextObject *libfreehand::FHCollector::_findTextObject(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHTextObject>::const_iterator iter = m_textObjects.find(id);
  if (iter != m_textObjects.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHTransform *libfreehand::FHCollector::_findTransform(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHTransform>::const_iterator iter = m_transforms.find(id);
  if (iter != m_transforms.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHParagraph *libfreehand::FHCollector::_findParagraph(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHParagraph>::const_iterator iter = m_paragraphs.find(id);
  if (iter != m_paragraphs.end())
    return &(iter->second);
  return 0;
}

const std::vector<unsigned> *libfreehand::FHCollector::_findTStringElements(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, std::vector<unsigned> >::const_iterator iter = m_tStrings.find(id);
  if (iter != m_tStrings.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHPropList *libfreehand::FHCollector::_findPropList(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHPropList>::const_iterator iter = m_propertyLists.find(id);
  if (iter != m_propertyLists.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHGraphicStyle *libfreehand::FHCollector::_findGraphicStyle(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHGraphicStyle>::const_iterator iter = m_graphicStyles.find(id);
  if (iter != m_graphicStyles.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHBasicFill *libfreehand::FHCollector::_findBasicFill(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHBasicFill>::const_iterator iter = m_basicFills.find(id);
  if (iter != m_basicFills.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHLinearFill *libfreehand::FHCollector::_findLinearFill(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHLinearFill>::const_iterator iter = m_linearFills.find(id);
  if (iter != m_linearFills.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHLensFill *libfreehand::FHCollector::_findLensFill(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHLensFill>::const_iterator iter = m_lensFills.find(id);
  if (iter != m_lensFills.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHRadialFill *libfreehand::FHCollector::_findRadialFill(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHRadialFill>::const_iterator iter = m_radialFills.find(id);
  if (iter != m_radialFills.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHTileFill *libfreehand::FHCollector::_findTileFill(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHTileFill>::const_iterator iter = m_tileFills.find(id);
  if (iter != m_tileFills.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHPatternFill *libfreehand::FHCollector::_findPatternFill(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHPatternFill>::const_iterator iter = m_patternFills.find(id);
  if (iter != m_patternFills.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHBasicLine *libfreehand::FHCollector::_findBasicLine(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHBasicLine>::const_iterator iter = m_basicLines.find(id);
  if (iter != m_basicLines.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHRGBColor *libfreehand::FHCollector::_findRGBColor(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHRGBColor>::const_iterator iter = m_rgbColors.find(id);
  if (iter != m_rgbColors.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHTintColor *libfreehand::FHCollector::_findTintColor(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHTintColor>::const_iterator iter = m_tints.find(id);
  if (iter != m_tints.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHDisplayText *libfreehand::FHCollector::_findDisplayText(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHDisplayText>::const_iterator iter = m_displayTexts.find(id);
  if (iter != m_displayTexts.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHImageImport *libfreehand::FHCollector::_findImageImport(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHImageImport>::const_iterator iter = m_images.find(id);
  if (iter != m_images.end())
    return &(iter->second);
  return 0;
}

const ::librevenge::RVNGBinaryData *libfreehand::FHCollector::_findData(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, ::librevenge::RVNGBinaryData>::const_iterator iter = m_data.find(id);
  if (iter != m_data.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHSymbolClass *libfreehand::FHCollector::_findSymbolClass(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHSymbolClass>::const_iterator iter = m_symbolClasses.find(id);
  if (iter != m_symbolClasses.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FHSymbolInstance *libfreehand::FHCollector::_findSymbolInstance(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHSymbolInstance>::const_iterator iter = m_symbolInstances.find(id);
  if (iter != m_symbolInstances.end())
    return &(iter->second);
  return 0;
}

const ::libfreehand::FHFilterAttributeHolder *libfreehand::FHCollector::_findFilterAttributeHolder(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHFilterAttributeHolder>::const_iterator iter = m_filterAttributeHolders.find(id);
  if (iter != m_filterAttributeHolders.end())
    return &(iter->second);
  return 0;
}

const std::vector<libfreehand::FHColorStop> *libfreehand::FHCollector::_findMultiColorList(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, std::vector<libfreehand::FHColorStop> >::const_iterator iter = m_multiColorLists.find(id);
  if (iter != m_multiColorLists.end())
    return &(iter->second);
  return 0;
}

const double *libfreehand::FHCollector::_findOpacityFilter(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, double>::const_iterator iter = m_opacityFilters.find(id);
  if (iter != m_opacityFilters.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FWShadowFilter *libfreehand::FHCollector::_findFWShadowFilter(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FWShadowFilter>::const_iterator iter = m_shadowFilters.find(id);
  if (iter != m_shadowFilters.end())
    return &(iter->second);
  return 0;
}

const libfreehand::FWGlowFilter *libfreehand::FHCollector::_findFWGlowFilter(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FWGlowFilter>::const_iterator iter = m_glowFilters.find(id);
  if (iter != m_glowFilters.end())
    return &(iter->second);
  return 0;
}

unsigned libfreehand::FHCollector::_findStrokeId(const libfreehand::FHGraphicStyle &graphicStyle)
{
  unsigned listId = graphicStyle.m_attrId;
  if (!listId)
    return 0;
  std::map<unsigned, FHList>::const_iterator iter = m_lists.find(listId);
  if (iter == m_lists.end())
    return 0;
  unsigned strokeId = 0;
  for (unsigned i = 0; i < iter->second.m_elements.size(); ++i)
  {
    unsigned valueId = _findValueFromAttribute(iter->second.m_elements[i]);
    if (_findBasicLine(valueId))
      strokeId = valueId;
  }
  return strokeId;
}

unsigned libfreehand::FHCollector::_findFillId(const libfreehand::FHGraphicStyle &graphicStyle)
{
  unsigned listId = graphicStyle.m_attrId;
  if (!listId)
    return 0;
  std::map<unsigned, FHList>::const_iterator iter = m_lists.find(listId);
  if (iter == m_lists.end())
    return 0;
  unsigned fillId = 0;
  for (unsigned i = 0; i < iter->second.m_elements.size(); ++i)
  {
    unsigned valueId = _findValueFromAttribute(iter->second.m_elements[i]);
    // Add other fills if we support them
    if (_findBasicFill(valueId) || _findLinearFill(valueId)
        || _findLensFill(valueId) || _findRadialFill(valueId)
        || _findTileFill(valueId) || _findPatternFill(valueId))
      fillId = valueId;
  }
  return fillId;
}

const libfreehand::FHFilterAttributeHolder *libfreehand::FHCollector::_findFilterAttributeHolder(const libfreehand::FHGraphicStyle &graphicStyle)
{
  unsigned listId = graphicStyle.m_attrId;
  if (!listId)
    return 0;
  std::map<unsigned, FHList>::const_iterator iter = m_lists.find(listId);
  if (iter == m_lists.end())
    return 0;
  for (unsigned i = 0; i < iter->second.m_elements.size(); ++i)
  {
    const FHFilterAttributeHolder *attributeHolder = _findFilterAttributeHolder(iter->second.m_elements[i]);

    if (attributeHolder)
      return attributeHolder;
  }
  return 0;
}


unsigned libfreehand::FHCollector::_findValueFromAttribute(unsigned id)
{
  if (!id)
    return 0;
  std::map<unsigned, FHAttributeHolder>::const_iterator iter = m_attributeHolders.find(id);
  if (iter == m_attributeHolders.end())
    return 0;
  unsigned value = 0;
  if (iter->second.m_parentId)
    value = _findValueFromAttribute(iter->second.m_parentId);
  if (iter->second.m_attrId)
    value = iter->second.m_attrId;
  return value;
}

::librevenge::RVNGBinaryData libfreehand::FHCollector::getImageData(unsigned id)
{
  std::map<unsigned, FHDataList>::const_iterator iter = m_dataLists.find(id);
  ::librevenge::RVNGBinaryData data;
  if (iter == m_dataLists.end())
    return data;
  for (unsigned i = 0; i < iter->second.m_elements.size(); ++i)
  {
    const ::librevenge::RVNGBinaryData *pData = _findData(iter->second.m_elements[i]);
    if (pData)
      data.append(*pData);
  }
  return data;
}

::librevenge::RVNGString libfreehand::FHCollector::getColorString(unsigned id)
{
  const FHRGBColor *color = _findRGBColor(id);
  if (color)
    return _getColorString(*color);
  const FHTintColor *tint = _findTintColor(id);
  if (tint)
    return _getColorString(getRGBFromTint(*tint));
  return ::librevenge::RVNGString();
}

libfreehand::FHRGBColor libfreehand::FHCollector::getRGBFromTint(const FHTintColor &tint)
{
  if (!tint.m_baseColorId)
    return FHRGBColor();
  const FHRGBColor *rgbColor = _findRGBColor(tint.m_baseColorId);
  if (!rgbColor)
    return FHRGBColor();
  unsigned red = rgbColor->m_red * tint.m_tint + (65536 - tint.m_tint) * 65536;
  unsigned green = rgbColor->m_green * tint.m_tint + (65536 - tint.m_tint) * 65536;
  unsigned blue = rgbColor->m_blue * tint.m_tint + (65536 - tint.m_tint) * 65536;
  FHRGBColor color;
  color.m_red = (red >> 16);
  color.m_green = (green >> 16);
  color.m_blue = (blue >> 16);
  return color;
}

void libfreehand::FHCollector::_generateBitmapFromPattern(librevenge::RVNGBinaryData &bitmap, unsigned colorId, const std::vector<unsigned char> &pattern)
{
  unsigned height = 8;
  unsigned width = 8;
  unsigned tmpPixelSize = (unsigned)(height * width);

  unsigned tmpDIBImageSize = tmpPixelSize * 4;

  unsigned tmpDIBOffsetBits = 14 + 40;
  unsigned tmpDIBFileSize = tmpDIBOffsetBits + tmpDIBImageSize;

  // Create DIB file header
  writeU16(bitmap, 0x4D42);  // Type
  writeU32(bitmap, (int)tmpDIBFileSize); // Size
  writeU16(bitmap, 0); // Reserved1
  writeU16(bitmap, 0); // Reserved2
  writeU32(bitmap, (int)tmpDIBOffsetBits); // OffsetBits

  // Create DIB Info header
  writeU32(bitmap, 40); // Size

  writeU32(bitmap, (int)width);  // Width
  writeU32(bitmap, (int)height); // Height

  writeU16(bitmap, 1); // Planes
  writeU16(bitmap, 32); // BitCount
  writeU32(bitmap, 0); // Compression
  writeU32(bitmap, (int)tmpDIBImageSize); // SizeImage
  writeU32(bitmap, 0); // XPelsPerMeter
  writeU32(bitmap, 0); // YPelsPerMeter
  writeU32(bitmap, 0); // ColorsUsed
  writeU32(bitmap, 0); // ColorsImportant

  unsigned foreground = 0x000000; // Initialize to black and override after
  unsigned background = 0xffffff; // Initialize to white, since that is Freehand behaviour even if overlapping other colors

  const FHRGBColor *color = _findRGBColor(colorId);
  if (color)
    foreground = ((unsigned)(color->m_red & 0xff00) << 8)|((unsigned)color->m_green & 0xff00)|((unsigned)color->m_blue >> 8);
  else
  {
    const FHTintColor *tintColor = _findTintColor(colorId);
    if (tintColor)
    {
      FHRGBColor rgbColor = getRGBFromTint(*tintColor);
      foreground = ((unsigned)(rgbColor.m_red & 0xff00) << 8)|((unsigned)rgbColor.m_green & 0xff00)|((unsigned)rgbColor.m_blue >> 8);
    }
  }
  for (unsigned j = height; j > 0; --j)
  {
    unsigned char c(pattern[j-1]);
    for (unsigned i = 0; i < width; ++i)
    {
      if (c & 0x80)
        writeU32(bitmap, foreground);
      else
        writeU32(bitmap, background);
      c <<= 1;
    }
  }
}


/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
