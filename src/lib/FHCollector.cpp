/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <algorithm>
#include <cassert>
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
#ifndef DUMP_CLIP_GROUPS
#define DUMP_CLIP_GROUPS 0
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
  librevenge::RVNGString colorString;
  colorString.sprintf("#%.2x%.2x%.2x", color.m_red >> 8, color.m_green >> 8, color.m_blue >> 8);
  return colorString;
}

static void _composePath(librevenge::RVNGPropertyListVector &path, bool isClosed)
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

class ObjectRecursionGuard
{
public:
  ObjectRecursionGuard(std::deque<unsigned> &objectStack, const unsigned id)
    : m_objectStack(objectStack)
    , m_id(id)
  {
    m_objectStack.push_front(m_id);
  }

  ~ObjectRecursionGuard()
  {
    assert(!m_objectStack.empty());
    assert(m_objectStack.front() == m_id);
    m_objectStack.pop_front();
  }

private:
  std::deque<unsigned> &m_objectStack;
  const unsigned m_id;
};

}

libfreehand::FHCollector::FHCollector() :
  m_pageInfo(), m_fhTail(), m_block(), m_transforms(), m_paths(), m_strings(), m_names(), m_lists(),
  m_layers(), m_groups(), m_clipGroups(), m_currentTransforms(), m_fakeTransforms(), m_compositePaths(),
  m_pathTexts(), m_tStrings(), m_fonts(), m_tEffects(), m_paragraphs(), m_tabs(), m_textBloks(), m_textObjects(), m_charProperties(),
  m_paragraphProperties(), m_rgbColors(), m_basicFills(), m_propertyLists(),
  m_basicLines(), m_customProcs(), m_patternLines(), m_displayTexts(), m_graphicStyles(),
  m_attributeHolders(), m_data(), m_dataLists(), m_images(), m_multiColorLists(), m_linearFills(),
  m_tints(), m_lensFills(), m_radialFills(), m_newBlends(), m_filterAttributeHolders(), m_opacityFilters(),
  m_shadowFilters(), m_glowFilters(), m_tileFills(), m_symbolClasses(), m_symbolInstances(), m_patternFills(),
  m_linePatterns(), m_arrowPaths(),
  m_strokeId(0), m_fillId(0), m_contentId(0), m_textBoxNumberId(0), m_visitedObjects()
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

void libfreehand::FHCollector::collectPathText(unsigned recordId, const libfreehand::FHPathText &pathText)
{
  m_pathTexts[recordId] = pathText;
}

void libfreehand::FHCollector::collectTString(unsigned recordId, const std::vector<unsigned> &elements)
{
  m_tStrings[recordId] = elements;
}

void libfreehand::FHCollector::collectAGDFont(unsigned recordId, const FHAGDFont &font)
{
  m_fonts[recordId] = font;
}

void libfreehand::FHCollector::collectTEffect(unsigned recordId, const FHTEffect &tEffect)
{
  m_tEffects[recordId] = tEffect;
}

void libfreehand::FHCollector::collectParagraph(unsigned recordId, const FHParagraph &paragraph)
{
  m_paragraphs[recordId] = paragraph;
}

void libfreehand::FHCollector::collectTabTable(unsigned recordId, const std::vector<FHTab> &tabs)
{
  if (tabs.empty()) return;
  m_tabs[recordId] = tabs;
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

void libfreehand::FHCollector::collectParagraphProps(unsigned recordId, const FHParagraphProperties &paragraphProps)
{
  m_paragraphProperties[recordId] = paragraphProps;
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

void libfreehand::FHCollector::collectCustomProc(unsigned recordId, const FHCustomProc &line)
{
  m_customProcs[recordId] = line;
}

void libfreehand::FHCollector::collectPatternLine(unsigned recordId, const FHPatternLine &line)
{
  m_patternLines[recordId] = line;
}

void libfreehand::FHCollector::collectTileFill(unsigned recordId, const FHTileFill &fill)
{
  m_tileFills[recordId] = fill;
}

void libfreehand::FHCollector::collectPatternFill(unsigned recordId, const FHPatternFill &fill)
{
  m_patternFills[recordId] = fill;
}

void libfreehand::FHCollector::collectLinePattern(unsigned recordId, const FHLinePattern &line)
{
  m_linePatterns[recordId] = line;
}

void libfreehand::FHCollector::collectArrowPath(unsigned recordId, const FHPath &path)
{
  // osnola: useme
  m_arrowPaths[recordId] = path;
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

void libfreehand::FHCollector::collectData(unsigned recordId, const librevenge::RVNGBinaryData &data)
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
  for (std::vector<FHTransform>::const_iterator iter = m_fakeTransforms.begin(); iter != m_fakeTransforms.end(); ++iter)
  {
    fhPath.transform(*iter);
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

  for (unsigned int element : *elements)
  {
    FHBoundingBox tmpBBox;
    _getBBofSomething(element, tmpBBox);
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

void libfreehand::FHCollector::_getBBofPathText(const FHPathText *pathText, libfreehand::FHBoundingBox &bBox)
{
  if (!pathText)
    return;

  _getBBofDisplayText(_findDisplayText(pathText->m_displayTextId),bBox);
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

  for (std::vector<FHTransform>::const_iterator iter = m_fakeTransforms.begin(); iter != m_fakeTransforms.end(); ++iter)
  {
    iter->applyToPoint(xa, ya);
    iter->applyToPoint(xb, yb);
    iter->applyToPoint(xc, yc);
    iter->applyToPoint(xd, yd);
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

  for (std::vector<FHTransform>::const_iterator iter = m_fakeTransforms.begin(); iter != m_fakeTransforms.end(); ++iter)
  {
    iter->applyToPoint(xa, ya);
    iter->applyToPoint(xb, yb);
    iter->applyToPoint(xc, yc);
    iter->applyToPoint(xd, yd);
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

  for (std::vector<FHTransform>::const_iterator iter = m_fakeTransforms.begin(); iter != m_fakeTransforms.end(); ++iter)
  {
    iter->applyToPoint(xa, ya);
    iter->applyToPoint(xb, yb);
    iter->applyToPoint(xc, yc);
    iter->applyToPoint(xd, yd);
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
  _getBBofPathText(_findPathText(somethingId), tmpBBox);
  _getBBofPath(_findPath(somethingId), tmpBBox);
  _getBBofCompositePath(_findCompositePath(somethingId), tmpBBox);
  _getBBofTextObject(_findTextObject(somethingId), tmpBBox);
  _getBBofDisplayText(_findDisplayText(somethingId), tmpBBox);
  _getBBofImageImport(_findImageImport(somethingId), tmpBBox);
  _getBBofNewBlend(_findNewBlend(somethingId), tmpBBox);
  _getBBofSymbolInstance(_findSymbolInstance(somethingId), tmpBBox);
  bBox.merge(tmpBBox);
}


void libfreehand::FHCollector::_outputPath(const libfreehand::FHPath *path, librevenge::RVNGDrawingInterface *painter)
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

  for (std::vector<FHTransform>::const_iterator iter = m_fakeTransforms.begin(); iter != m_fakeTransforms.end(); ++iter)
  {
    fhPath.transform(*iter);
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
    painter->openGroup(librevenge::RVNGPropertyList());
  painter->setStyle(propList);
  painter->drawPath(pList);
  if (contentId)
  {
    FHBoundingBox bBox;
    fhPath.getBoundingBox(bBox.m_xmin, bBox.m_ymin, bBox.m_xmax, bBox.m_ymax);
    FHTransform trafo(1.0, 0.0, 0.0, 1.0, - bBox.m_xmin, - bBox.m_ymin);
    m_fakeTransforms.push_back(trafo);
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
      propList.insert("draw:fill-image", output);
      painter->setStyle(propList);
      painter->drawPath(pList);
    }
    if (!m_fakeTransforms.empty())
      m_fakeTransforms.pop_back();
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

void libfreehand::FHCollector::_outputSomething(unsigned somethingId, librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !somethingId)
    return;
  if (find(m_visitedObjects.begin(), m_visitedObjects.end(), somethingId) != m_visitedObjects.end())
    return;

  const ObjectRecursionGuard guard(m_visitedObjects, somethingId);

  _outputGroup(_findGroup(somethingId), painter);
  _outputClipGroup(_findClipGroup(somethingId), painter);
  _outputPathText(_findPathText(somethingId), painter);
  _outputPath(_findPath(somethingId), painter);
  _outputCompositePath(_findCompositePath(somethingId), painter);
  _outputTextObject(_findTextObject(somethingId), painter);
  _outputDisplayText(_findDisplayText(somethingId), painter);
  _outputImageImport(_findImageImport(somethingId), painter);
  _outputNewBlend(_findNewBlend(somethingId), painter);
  _outputSymbolInstance(_findSymbolInstance(somethingId), painter);
}

void libfreehand::FHCollector::_outputGroup(const libfreehand::FHGroup *group, librevenge::RVNGDrawingInterface *painter)
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
    painter->openGroup(librevenge::RVNGPropertyList());
    for (unsigned int element : *elements)
      _outputSomething(element, painter);
    painter->closeGroup();
  }

  if (!m_currentTransforms.empty())
    m_currentTransforms.pop();
}

void libfreehand::FHCollector::_outputClipGroup(const libfreehand::FHGroup *group, librevenge::RVNGDrawingInterface *painter)
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

      for (std::vector<FHTransform>::const_iterator iterVec = m_fakeTransforms.begin(); iterVec != m_fakeTransforms.end(); ++iterVec)
      {
        fhPath.transform(*iterVec);
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
      m_fakeTransforms.push_back(trafo);
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
#if DUMP_CLIP_GROUPS
        {
          librevenge::RVNGString filename;
          filename.sprintf("freehandclipgroup%.4x.svg", group->m_elementsId);
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
        propList.insert("draw:stroke", "none");
        propList.insert("draw:fill", "bitmap");
        propList.insert("librevenge:mime-type", "image/svg+xml");
        propList.insert("style:repeat", "stretch");
        propList.insert("draw:fill-image", output);
        painter->setStyle(propList);
        painter->drawPath(pList);
      }
      if (!m_fakeTransforms.empty())
        m_fakeTransforms.pop_back();
    }
  }
}

void libfreehand::FHCollector::_outputPathText(const libfreehand::FHPathText *pathText, librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !pathText)
    return;

  _outputDisplayText(_findDisplayText(pathText->m_displayTextId), painter);
}

void libfreehand::FHCollector::_outputNewBlend(const libfreehand::FHNewBlend *newBlend, librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !newBlend)
    return;

  m_currentTransforms.push(libfreehand::FHTransform());

  painter->openGroup(librevenge::RVNGPropertyList());
  const std::vector<unsigned> *elements1 = _findListElements(newBlend->m_list1Id);
  if (elements1 && !elements1->empty())
  {
    for (unsigned int iterVec : *elements1)
      _outputSomething(iterVec, painter);
  }
  const std::vector<unsigned> *elements2 = _findListElements(newBlend->m_list2Id);
  if (elements2 && !elements2->empty())
  {
    for (unsigned int iterVec : *elements2)
      _outputSomething(iterVec, painter);
  }
  const std::vector<unsigned> *elements3 = _findListElements(newBlend->m_list3Id);
  if (elements3 && !elements3->empty())
  {
    for (unsigned int iterVec : *elements3)
      _outputSomething(iterVec, painter);
  }
  painter->closeGroup();

  if (!m_currentTransforms.empty())
    m_currentTransforms.pop();
}

void libfreehand::FHCollector::_outputSymbolInstance(const libfreehand::FHSymbolInstance *symbolInstance, librevenge::RVNGDrawingInterface *painter)
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

void libfreehand::FHCollector::outputDrawing(librevenge::RVNGDrawingInterface *painter)
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
    for (unsigned int element : *elements)
    {
      _outputLayer(element, painter);
    }
  }
  painter->endPage();
  painter->endDocument();
}

void libfreehand::FHCollector::_outputLayer(unsigned layerId, librevenge::RVNGDrawingInterface *painter)
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

  for (unsigned int element : *elements)
    _outputSomething(element, painter);
}

void libfreehand::FHCollector::_outputCompositePath(const libfreehand::FHCompositePath *compositePath, librevenge::RVNGDrawingInterface *painter)
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

void libfreehand::FHCollector::_outputTextObject(const libfreehand::FHTextObject *textObject, librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !textObject)
    return;

  double width=textObject->m_width;
  double height=textObject->m_height;
  unsigned num[]= {textObject->m_colNum,textObject->m_rowNum};
  double decalX[]= {width+textObject->m_colSep,0};
  double decalY[]= {0, height+textObject->m_rowSep};
  if (textObject->m_rowBreakFirst)
  {
    std::swap(num[0],num[1]);
    std::swap(decalX[0],decalX[1]);
    std::swap(decalY[0],decalY[1]);
  }
  for (unsigned int &i : num)
  {
    if (i<=0 || i>10)
    {
      FH_DEBUG_MSG(("libfreehand::FHCollector::_outputTextObject: the number of row/col seems bad\n"));
      i=1;
    }
  }
  ++m_textBoxNumberId;
  for (unsigned dim0=0; dim0<num[0]; ++dim0)
  {
    for (unsigned dim1=0; dim1<num[1]; ++dim1)
    {
      unsigned id = dim0*num[1]+dim1;
      double rotation = 0, finalHeight = 0, finalWidth = 0, xmid=0, ymid=0;
      bool useShapeBox=false;
      if ((width<=0 || height<=0) && textObject->m_pathId)
      {
        /* the position are not set for TFOnPath, so we must look for the shape box

           Note: the width and height seem better, the x,y position are still quite random :-~
        */
        FHBoundingBox bbox;
        _getBBofSomething(textObject->m_pathId, bbox);
        useShapeBox=true;
        xmid=0.5*(bbox.m_xmin+bbox.m_xmax);
        ymid=0.5*(bbox.m_ymin+bbox.m_ymax);
        width=finalWidth=(bbox.m_xmax-bbox.m_xmin);
        height=finalHeight=(bbox.m_ymax-bbox.m_ymin);
      }
      if (!useShapeBox)
      {
#ifdef HAVE_CHAINED_TEXTBOX
        // useme when we can chain frames in Draw
        double startX=textObject->m_startX+dim0*decalX[0]+dim1*decalX[1];
        double startY=textObject->m_startY+dim0*decalY[0]+dim1*decalY[1];
#else
        /* if the number of row/column is greater than 1, we have a
           big problem. Let increase the text-box size to contain all
           the chained text-boxes...
         */
        double startX=textObject->m_startX;
        double startY=textObject->m_startY;
        width += (num[0]-1)*decalX[0]+(num[1]-1)*decalX[1];
        height += (num[0]-1)*decalY[0]+(num[1]-1)*decalY[1];
#endif
        double xa = startX;
        double ya = startY;
        double xb = startX + width;
        double yb = startY + height;
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

        for (std::vector<FHTransform>::const_iterator iter = m_fakeTransforms.begin(); iter != m_fakeTransforms.end(); ++iter)
        {
          iter->applyToPoint(xa, ya);
          iter->applyToPoint(xb, yb);
          iter->applyToPoint(xc, yc);
        }

        rotation = atan2(yb-yc, xb-xc);
        finalHeight = sqrt((xc-xa)*(xc-xa) + (yc-ya)*(yc-ya));
        finalWidth = sqrt((xc-xb)*(xc-xb) + (yc-yb)*(yc-yb));
        xmid = (xa + xb) / 2.0;
        ymid = (ya + yb) / 2.0;
      }

      librevenge::RVNGPropertyList textObjectProps;
      textObjectProps.insert("svg:x", xmid - width / 2.0);
      textObjectProps.insert("svg:y", ymid + height / 2.0);
      textObjectProps.insert("svg:height", finalHeight);
      textObjectProps.insert("svg:width", finalWidth);
      if (!FH_ALMOST_ZERO(rotation))
      {
        textObjectProps.insert("librevenge:rotate", rotation * 180.0 / M_PI);
        textObjectProps.insert("librevenge:rotate-cx",xmid);
        textObjectProps.insert("librevenge:rotate-cy",ymid);
      }
#ifdef HAVE_CHAINED_TEXTBOX
      if (id)
      {
        librevenge::RVNGString name;
        name.sprintf("Textbox%d-%d",m_textBoxNumberId,id);
        textObjectProps.insert("librevenge:frame-name",name);
      }
      if (id+1!=num[0]*num[1])
      {
        librevenge::RVNGString name;
        name.sprintf("Textbox%d-%d",m_textBoxNumberId,id+1);
        textObjectProps.insert("librevenge:next-frame-name",name);
      }
#endif
      painter->startTextObject(textObjectProps);

      if (id==0)
      {
        const std::vector<unsigned> *elements = _findTStringElements(textObject->m_tStringId);
        unsigned actPos=0;
        if (elements && !elements->empty())
        {
          for (unsigned int element : *elements)
            _outputParagraph(_findParagraph(element), painter, actPos, textObject->m_beginPos, textObject->m_endPos);
        }
      }
      painter->endTextObject();

#if !defined(HAVE_CHAINED_TEXTBOX)
      break;
#endif
    }
#if !defined(HAVE_CHAINED_TEXTBOX)
    break;
#endif
  }
}

void libfreehand::FHCollector::_outputParagraph(const libfreehand::FHParagraph *paragraph, librevenge::RVNGDrawingInterface *painter, unsigned &actPos, unsigned minPos, unsigned maxPos)
{
  if (!painter || !paragraph)
    return;
  bool paragraphOpened=false;
  std::map<unsigned, std::vector<unsigned short> >::const_iterator iter = m_textBloks.find(paragraph->m_textBlokId);
  if (iter != m_textBloks.end())
  {

    for (std::vector<std::pair<unsigned, unsigned> >::size_type i = 0; i < paragraph->m_charStyleIds.size(); ++i)
    {
      if (actPos>=maxPos) break;
      unsigned lastChar=i+1 < paragraph->m_charStyleIds.size() ? paragraph->m_charStyleIds[i+1].first : iter->second.size();
      unsigned numChar=lastChar-paragraph->m_charStyleIds[i].first;
      unsigned nextPos=actPos+numChar;
      if (nextPos<minPos)
      {
        actPos=nextPos;
        continue;
      }
      if (!paragraphOpened)
      {
        librevenge::RVNGPropertyList propList;
        _appendParagraphProperties(propList, paragraph->m_paraStyleId);
        painter->openParagraph(propList);
        paragraphOpened=true;
      }
      unsigned fChar=paragraph->m_charStyleIds[i].first + (actPos<minPos ? minPos-actPos : 0);
      numChar=lastChar-fChar;
      if (actPos+numChar>maxPos) numChar=maxPos-actPos;
      _outputTextRun(&(iter->second), fChar, numChar, paragraph->m_charStyleIds[i].second, painter);
      actPos=nextPos;
    }
  }
  ++actPos; // EOL
  if (paragraphOpened)
    painter->closeParagraph();
}

void libfreehand::FHCollector::_appendCharacterProperties(librevenge::RVNGPropertyList &propList, unsigned charPropsId)
{
  std::map<unsigned, FHCharProperties>::const_iterator iter = m_charProperties.find(charPropsId);
  if (iter == m_charProperties.end())
    return;
  const FHCharProperties &charProps = iter->second;
  if (charProps.m_fontNameId)
  {
    std::map<unsigned, librevenge::RVNGString>::const_iterator iterString = m_strings.find(charProps.m_fontNameId);
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
  FHTEffect const *eff=_findTEffect(charProps.m_tEffectId);
  if (eff && eff->m_nameId)
  {
    std::map<unsigned, librevenge::RVNGString>::const_iterator iterString = m_strings.find(eff->m_nameId);
    if (iterString != m_strings.end())
    {
      librevenge::RVNGString const &type=iterString->second;
      if (type=="InlineEffect")   // inside col1, outside col0
      {
        propList.insert("fo:font-weight", "bold");
        librevenge::RVNGString color = getColorString(eff->m_colorId[1]);
        if (!color.empty())
          propList.insert("fo:color", color);
      }
      else if (type=="ShadowEffect")
        propList.insert("fo:text-shadow", "1pt 1pt");
      else if (type=="ZoomEffect")
      {
        propList.insert("style:font-relief", "embossed");
        propList.insert("fo:text-shadow", "1pt -1pt");
        librevenge::RVNGString color = getColorString(eff->m_colorId[0]);
        if (!color.empty())
          propList.insert("fo:color", color);
      }
      else
      {
        FH_DEBUG_MSG(("libfreehand::FHCollector::_appendCharacterProperties: find unknown effect %s\n",type.cstr()));
      }
    }
  }
  for (std::map<unsigned,double>::const_iterator it=charProps.m_idToDoubleMap.begin(); it!=charProps.m_idToDoubleMap.end(); ++it)
  {
    switch (it->first)
    {
    case FH_BASELN_SHIFT:
    {
      if (it->second<=0 && it->second>=0) break;
      librevenge::RVNGString value;
      double fontSize=(charProps.m_fontSize>0) ? charProps.m_fontSize : 24.;
      value.sprintf("%g%%",100.*it->second/fontSize);
      propList.insert("style:text-position", value);
      break;
    }
    case FH_HOR_SCALE:
      if (it->second<=1 && it->second>=1) break;
      propList.insert("style:text-scale", it->second, librevenge::RVNG_PERCENT);
      break;
    case FH_RNG_KERN:
      if (it->second<=0 && it->second>=0) break;
      propList.insert("fo:letter-spacing", it->second*charProps.m_fontSize, librevenge::RVNG_POINT);
      break;
    default:
      break;
    }
  }
}

void libfreehand::FHCollector::_appendCharacterProperties(librevenge::RVNGPropertyList &propList, const FH3CharProperties &charProps)
{
  if (charProps.m_fontNameId)
  {
    std::map<unsigned, librevenge::RVNGString>::const_iterator iterString = m_strings.find(charProps.m_fontNameId);
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
  if (charProps.m_letterSpacing<0 || charProps.m_letterSpacing>0)
    propList.insert("fo:letter-spacing", charProps.m_letterSpacing, librevenge::RVNG_POINT);
  if (charProps.m_horizontalScale<1 || charProps.m_horizontalScale>1)
    propList.insert("style:text-scale", charProps.m_horizontalScale, librevenge::RVNG_PERCENT);
  if (charProps.m_baselineShift<0 || charProps.m_baselineShift>0)
  {
    librevenge::RVNGString value;
    double fontSize=(charProps.m_fontSize>0) ? charProps.m_fontSize : 24.;
    value.sprintf("%g%%",100.*charProps.m_baselineShift/fontSize);
    propList.insert("style:text-position", value);
  }
  FHTEffect const *eff=_findTEffect(charProps.m_textEffsId);
  if (eff && eff->m_shortNameId)
  {
    std::map<unsigned, librevenge::RVNGString>::const_iterator iterString = m_strings.find(eff->m_shortNameId);
    if (iterString != m_strings.end())
    {
      librevenge::RVNGString const &type=iterString->second;
      if (type=="inlin")   // inside col1, outside col0
        propList.insert("fo:font-weight", "bold");
      else if (type=="otw stol")
        propList.insert("style:text-outline", "true");
      else if (type=="stob")
        propList.insert("fo:font-style", "italic");
      else if (type=="stsh")
        propList.insert("fo:text-shadow", "1pt 1pt");
      else if (type=="sthv")
        propList.insert("fo:font-weight", "bold");
      else if (type=="extrude")
      {
        propList.insert("style:font-relief", "embossed");
        propList.insert("fo:text-shadow", "1pt -1pt");
        librevenge::RVNGString color = getColorString(eff->m_colorId[0]);
        if (!color.empty())
          propList.insert("fo:color", color);
      }
      else
      {
        FH_DEBUG_MSG(("libfreehand::FHCollector::_appendCharacterProperties: find unknown effect %s\n",type.cstr()));
      }
    }
  }
}

void libfreehand::FHCollector::_appendTabProperties(librevenge::RVNGPropertyList &propList, const libfreehand::FHTab &tab)
{
  switch (tab.m_type)
  {
  case 0:
  case 4: // unsure, look like a left tab
  default:
    break;
  case 1:
    propList.insert("style:type", "right");
    break;
  case 2:
    propList.insert("style:type", "center");
    break;
  case 3:
    propList.insert("style:type", "char");
    propList.insert("style:char", "."); // checkme
    break;
  }
  propList.insert("style:position", tab.m_position, librevenge::RVNG_POINT);
}

void libfreehand::FHCollector::_appendParagraphProperties(librevenge::RVNGPropertyList & /* propList */, const FH3ParaProperties & /* paraProps */)
{
}

void libfreehand::FHCollector::_appendParagraphProperties(librevenge::RVNGPropertyList &propList, unsigned paragraphPropsId)
{
  std::map<unsigned, FHParagraphProperties>::const_iterator iter = m_paragraphProperties.find(paragraphPropsId);
  if (iter == m_paragraphProperties.end())
    return;
  FHParagraphProperties const &para=iter->second;
  for (const auto &it : para.m_idToZoneIdMap)
  {
    switch (it.first)
    {
    case FH_PARA_TAB_TABLE_ID:
      if (m_tabs.find(it.second)!=m_tabs.end())
      {
        std::vector<FHTab> const &tabs=m_tabs.find(it.second)->second;
        if (tabs.empty())
          break;
        librevenge::RVNGPropertyListVector tabVect;
        for (auto tab : tabs)
        {
          librevenge::RVNGPropertyList tabList;
          _appendTabProperties(tabList, tab);
          tabVect.append(tabList);
        }
        propList.insert("style:tab-stops", tabVect);
      }
      break;
    default:
      break;
    }
  }
  for (std::map<unsigned,double>::const_iterator it=para.m_idToDoubleMap.begin(); it!=para.m_idToDoubleMap.end(); ++it)
  {
    switch (it->first)
    {
    case FH_PARA_LEFT_INDENT:
      propList.insert("fo:margin-left", it->second, librevenge::RVNG_POINT);
      break;
    case FH_PARA_RIGHT_INDENT:
      propList.insert("fo:margin-right", it->second, librevenge::RVNG_POINT);
      break;
    case FH_PARA_TEXT_INDENT:
      propList.insert("fo:text-indent", it->second, librevenge::RVNG_POINT);
      break;
    case FH_PARA_SPC_ABOVE:
      propList.insert("fo:margin-top", it->second, librevenge::RVNG_POINT);
      break;
    case FH_PARA_SPC_BELLOW:
      propList.insert("fo:margin-bottom", it->second, librevenge::RVNG_POINT);
      break;
    case FH_PARA_LEADING:
      if (it->second<=0 && it->second>=0) break;
      if (para.m_idToIntMap.find(FH_PARA_LEADING_TYPE)==para.m_idToIntMap.end())
      {
        FH_DEBUG_MSG(("libfreehand::FHCollector::_appendParagraphProperties: can not find the leading type\n"));
        break;
      }
      switch (para.m_idToIntMap.find(FH_PARA_LEADING_TYPE)->second)
      {
      case 0: // delta in point
        propList.insert("fo:line-height",1.+it->second/(it->second>0 ? 12 : 24), librevenge::RVNG_PERCENT);
        break;
      case 1:
        propList.insert("fo:line-height",it->second, librevenge::RVNG_POINT);
        break;
      case 2:
        propList.insert("fo:line-height",it->second, librevenge::RVNG_PERCENT);
        break;
      default:
        break;
      }
      break;
    default:
      break;
    }
  }
  for (const auto &it : para.m_idToIntMap)
  {
    switch (it.first)
    {
    case FH_PARA_TEXT_ALIGN:
      switch (it.second)
      {
      case 0:
        propList.insert("fo:text-align", "left");
        break;
      case 1:
        propList.insert("fo:text-align", "end");
        break;
      case 2:
        propList.insert("fo:text-align", "center");
        break;
      case 3:
        propList.insert("fo:text-align", "justify");
        break;
      default:
        break;
      }
      break;
    case FH_PARA_KEEP_SAME_LINE:
      if (it.second==1)
        propList.insert("fo:keep-together", "always");
      break;
    case FH_PARA_LEADING_TYPE: // done with FH_PARA_LEADING
    default:
      break;
    }
  }
}

void libfreehand::FHCollector::_outputDisplayText(const libfreehand::FHDisplayText *displayText, librevenge::RVNGDrawingInterface *painter)
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

  for (std::vector<FHTransform>::const_iterator iter = m_fakeTransforms.begin(); iter != m_fakeTransforms.end(); ++iter)
  {
    iter->applyToPoint(xa, ya);
    iter->applyToPoint(xb, yb);
    iter->applyToPoint(xc, yc);
  }

  double rotation = atan2(yb-yc, xb-xc);
  double height = sqrt((xc-xa)*(xc-xa) + (yc-ya)*(yc-ya));
  double width = sqrt((xc-xb)*(xc-xb) + (yc-yb)*(yc-yb));
  double xmid = (xa + xb) / 2.0;
  double ymid = (ya + yb) / 2.0;

  librevenge::RVNGPropertyList textObjectProps;
  textObjectProps.insert("svg:x", xmid - displayText->m_width / 2.0);
  textObjectProps.insert("svg:y", ymid + displayText->m_height / 2.0);
  textObjectProps.insert("svg:height", height);
  textObjectProps.insert("svg:width", width);
  for (int i=0; i<4; ++i) // osnola: let assume that there is no padding
  {
    char const *(padding[])= {"fo:padding-left","fo:padding-right","fo:padding-top","fo:padding-bottom"};
    textObjectProps.insert(padding[i],0,librevenge::RVNG_POINT);
  }
  if (!FH_ALMOST_ZERO(rotation))
  {
    textObjectProps.insert("librevenge:rotate", rotation * 180.0 / M_PI);
    textObjectProps.insert("librevenge:rotate-cx",xmid);
    textObjectProps.insert("librevenge:rotate-cy",ymid);
  }
  if (displayText->m_justify==4)  // top-down: checkme do nothing
    textObjectProps.insert("style:writing-mode", "tb-lr");
  painter->startTextObject(textObjectProps);

  std::vector<FH3ParaProperties>::const_iterator iterPara = displayText->m_paraProps.begin();
  std::vector<FH3CharProperties>::const_iterator iterChar = displayText->m_charProps.begin();

  FH3ParaProperties paraProps = *iterPara++;
  FH3CharProperties charProps = *iterChar++;
  librevenge::RVNGString text;
  std::vector<unsigned char>::size_type i = 0;

  librevenge::RVNGPropertyList paraPropList;
  _appendParagraphProperties(paraPropList, paraProps);
  switch (displayText->m_justify)
  {
  case 0: // left
    break;
  case 1:
    paraPropList.insert("fo:text-align", "center");
    break;
  case 2:
    paraPropList.insert("fo:text-align", "end");
    break;
  case 3:
    paraPropList.insert("fo:text-align", "justify");
    break;
  case 4:
  default:
    break;
  }
  if (charProps.m_leading>0)
    paraPropList.insert("fo:line-height",charProps.m_leading,librevenge::RVNG_POINT);
  else
    paraPropList.insert("fo:line-height",1.,librevenge::RVNG_PERCENT);
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
#if 0
      paraPropList.clear();
      _appendParagraphProperties(paraPropList, paraProps);
#endif
      if (charProps.m_leading>0)
        paraPropList.insert("fo:line-height",charProps.m_leading,librevenge::RVNG_POINT);
      else
        paraPropList.insert("fo:line-height",1.,librevenge::RVNG_PERCENT);
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

void libfreehand::FHCollector::_outputImageImport(const FHImageImport *image, librevenge::RVNGDrawingInterface *painter)
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

  for (std::vector<FHTransform>::const_iterator iter = m_fakeTransforms.begin(); iter != m_fakeTransforms.end(); ++iter)
  {
    iter->applyToPoint(xa, ya);
    iter->applyToPoint(xb, yb);
    iter->applyToPoint(xc, yc);
  }

  double rotation = atan2(yb-yc, xb-xc);
  double height = sqrt((xc-xa)*(xc-xa) + (yc-ya)*(yc-ya));
  double width = sqrt((xc-xb)*(xc-xb) + (yc-yb)*(yc-yb));
  double xmid = (xa + xb) / 2.0;
  double ymid = (ya + yb) / 2.0;

  librevenge::RVNGPropertyList imageProps;
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
                                              unsigned charStyleId, librevenge::RVNGDrawingInterface *painter)
{
  if (!painter || !characters || characters->empty())
    return;
  librevenge::RVNGPropertyList propList;
  _appendCharacterProperties(propList, charStyleId);
  painter->openSpan(propList);
  std::vector<unsigned short> tmpChars;
  bool lastIsSpace=false;
  for (unsigned i = offset; i < length+offset && i < characters->size(); ++i)
  {
    unsigned c=(*characters)[i];
    if (c=='\t' || (c==' ' && lastIsSpace))
    {
      if (!tmpChars.empty())
      {
        librevenge::RVNGString text;
        _appendUTF16(text, tmpChars);
        painter->insertText(text);
        tmpChars.clear();
      }
      if (c=='\t')
        painter->insertTab();
      else
        painter->insertSpace();
      continue;
    }
    else
    {
      if (c<=0x1f)
      {
        switch (c)
        {
        case 0xb: // end of column
          break;
        case 0x1f: // optional hyphen
          break;
        default:
          FH_DEBUG_MSG(("libfreehand::FHCollector::_outputTextRun: find character %x\n", c));
          break;
        }
      }
      else
        tmpChars.push_back(c);
    }
    lastIsSpace=c==' ';
  }
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
  return nullptr;
}


void libfreehand::FHCollector::_appendFontProperties(librevenge::RVNGPropertyList &propList, unsigned agdFontId)
{
  std::map<unsigned, FHAGDFont>::const_iterator iter = m_fonts.find(agdFontId);
  if (iter == m_fonts.end())
    return;
  const FHAGDFont &font = iter->second;
  if (font.m_fontNameId)
  {
    std::map<unsigned, librevenge::RVNGString>::const_iterator iterString = m_strings.find(font.m_fontNameId);
    if (iterString != m_strings.end())
      propList.insert("fo:font-name", iterString->second);
  }
  propList.insert("fo:font-size", font.m_fontSize, librevenge::RVNG_POINT);
  if (font.m_fontStyle & 1)
    propList.insert("fo:font-weight", "bold");
  if (font.m_fontStyle & 2)
    propList.insert("fo:font-style", "italic");
}

void libfreehand::FHCollector::_appendFillProperties(librevenge::RVNGPropertyList &propList, unsigned graphicStyleId)
{
  if (!propList["draw:fill"])
    propList.insert("draw:fill", "none");
  if (graphicStyleId && find(m_visitedObjects.begin(), m_visitedObjects.end(), graphicStyleId) == m_visitedObjects.end())
  {
    const ObjectRecursionGuard guard(m_visitedObjects, graphicStyleId);
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
        _appendCustomProcFill(propList, _findCustomProc(iter->second));
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
          _appendCustomProcFill(propList, _findCustomProc(fillId));
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

void libfreehand::FHCollector::_appendStrokeProperties(librevenge::RVNGPropertyList &propList, unsigned graphicStyleId)
{
  if (!propList["draw:stroke"])
    propList.insert("draw:stroke", "none");
  if (graphicStyleId && find(m_visitedObjects.begin(), m_visitedObjects.end(), graphicStyleId) == m_visitedObjects.end())
  {
    const ObjectRecursionGuard guard(m_visitedObjects, graphicStyleId);
    const FHPropList *propertyList = _findPropList(graphicStyleId);
    if (propertyList)
    {
      if (propertyList->m_parentId)
        _appendStrokeProperties(propList, propertyList->m_parentId);
      std::map<unsigned, unsigned>::const_iterator iter = propertyList->m_elements.find(m_strokeId);
      if (iter != propertyList->m_elements.end())
      {
        _appendBasicLine(propList, _findBasicLine(iter->second));
        _appendPatternLine(propList, _findPatternLine(iter->second));
        _appendCustomProcLine(propList, _findCustomProc(iter->second));
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
        {
          _appendBasicLine(propList, _findBasicLine(strokeId));
          _appendPatternLine(propList, _findPatternLine(strokeId));
          _appendCustomProcLine(propList, _findCustomProc(strokeId));
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

void libfreehand::FHCollector::_appendBasicFill(librevenge::RVNGPropertyList &propList, const libfreehand::FHBasicFill *basicFill)
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

void libfreehand::FHCollector::_appendCustomProcFill(librevenge::RVNGPropertyList &propList, const libfreehand::FHCustomProc *fill)
{
  if (!fill || fill->m_ids.empty())
    return;
  propList.insert("draw:fill", "solid");
  librevenge::RVNGString color = getColorString(fill->m_ids[0]);
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

void libfreehand::FHCollector::_appendLinearFill(librevenge::RVNGPropertyList &propList, const libfreehand::FHLinearFill *linearFill)
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

void libfreehand::FHCollector::_applyFilter(librevenge::RVNGPropertyList &propList, unsigned filterId)
{
  if (!filterId)
    return;
  _appendOpacity(propList, _findOpacityFilter(filterId));
  _appendShadow(propList, _findFWShadowFilter(filterId));
  _appendGlow(propList, _findFWGlowFilter(filterId));
}

void libfreehand::FHCollector::_appendOpacity(librevenge::RVNGPropertyList &propList, const double *opacity)
{
  if (!opacity)
    return;
  if (propList["draw:fill"] && propList["draw:fill"]->getStr() != "none")
    propList.insert("draw:opacity", *opacity, librevenge::RVNG_PERCENT);
  if (propList["draw:stroke"] && propList["draw:stroke"]->getStr() != "none")
    propList.insert("svg:stroke-opacity", *opacity, librevenge::RVNG_PERCENT);
}

void libfreehand::FHCollector::_appendShadow(librevenge::RVNGPropertyList &propList, const libfreehand::FWShadowFilter *filter)
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

void libfreehand::FHCollector::_appendGlow(librevenge::RVNGPropertyList & /* propList */, const libfreehand::FWGlowFilter *filter)
{
  if (!filter)
    return;
}

void libfreehand::FHCollector::_appendLensFill(librevenge::RVNGPropertyList &propList, const libfreehand::FHLensFill *lensFill)
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

void libfreehand::FHCollector::_appendRadialFill(librevenge::RVNGPropertyList &propList, const libfreehand::FHRadialFill *radialFill)
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

void libfreehand::FHCollector::_appendTileFill(librevenge::RVNGPropertyList &propList, const libfreehand::FHTileFill *tileFill)
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
    m_fakeTransforms.push_back(fakeTrafo);

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
      propList.insert("draw:fill-image", output);
      propList.insert("draw:fill-image-width", tileFill->m_scaleX * (bBox.m_xmax - bBox.m_xmin));
      propList.insert("draw:fill-image-height", tileFill->m_scaleY * (bBox.m_ymax - bBox.m_ymin));
      propList.insert("librevenge:mime-type", "image/svg+xml");
      propList.insert("style:repeat", "repeat");
    }

    if (!m_fakeTransforms.empty())
      m_fakeTransforms.pop_back();
  }
  if (!m_currentTransforms.empty())
    m_currentTransforms.pop();
}

void libfreehand::FHCollector::_appendPatternFill(librevenge::RVNGPropertyList &propList, const libfreehand::FHPatternFill *patternFill)
{
  if (!patternFill)
    return;
  librevenge::RVNGBinaryData output;
  _generateBitmapFromPattern(output, patternFill->m_colorId, patternFill->m_pattern);
  propList.insert("draw:fill", "bitmap");
  propList.insert("draw:fill-image", output);
  propList.insert("librevenge:mime-type", "image/bmp");
  propList.insert("style:repeat", "repeat");
}

void libfreehand::FHCollector::_appendLinePattern(librevenge::RVNGPropertyList &propList, const libfreehand::FHLinePattern *linePattern)
{
  if (!linePattern || linePattern->m_dashes.size()<=1)
    return;
  int nDots1=0, nDots2=0;
  double size1=0, size2=0, totalGap=0.0;
  for (size_t c=0; c+1 < linePattern->m_dashes.size();)
  {
    double sz=linePattern->m_dashes[c++];
    if (nDots2 && (sz<size2||sz>size2))
    {
      static bool first=true;
      if (first)
      {
        FH_DEBUG_MSG(("libfreehand::FHCollector::_appendLinePattern: can not set some dash\n"));
        first = false;
      }
      break;
    }
    if (nDots2)
      nDots2++;
    else if (!nDots1 || (sz>=size1 && sz <= size1))
    {
      nDots1++;
      size1=sz;
    }
    else
    {
      nDots2=1;
      size2=sz;
    }
    totalGap += linePattern->m_dashes[c++];
  }
  propList.insert("draw:stroke", "dash");
  propList.insert("draw:dots1", nDots1);
  propList.insert("draw:dots1-length", double(size1), librevenge::RVNG_POINT);
  if (nDots2)
  {
    propList.insert("draw:dots2", nDots2);
    propList.insert("draw:dots2-length", double(size2), librevenge::RVNG_POINT);
  }
  const double distance = ((nDots1 + nDots2) > 0) ? double(totalGap)/double(nDots1+nDots2) : double(totalGap);
  propList.insert("draw:distance", distance, librevenge::RVNG_POINT);;
}

void libfreehand::FHCollector::_appendArrowPath(librevenge::RVNGPropertyList &propList, const FHPath *arrow, bool startArrow)
{
  if (!arrow)
    return;

  FHPath path=*arrow;
  path.transform(FHTransform(0,-1,1,0,0,0));
  std::string pString=path.getPathString();
  if (pString.empty()) return;
  std::string wh(startArrow ? "start" : "end");
  propList.insert((std::string("draw:marker-")+wh+"-path").c_str(), pString.c_str());
  FHBoundingBox box;
  path.getBoundingBox(box.m_xmin, box.m_ymin, box.m_xmax, box.m_ymax);
  librevenge::RVNGString boxString;
  boxString.sprintf("%d %d %d %d", int(box.m_xmin*35), int(box.m_ymin*35), int(35*(box.m_xmax-box.m_xmin)), int(35*(box.m_ymax-box.m_ymin)));
  propList.insert((std::string("draw:marker-")+wh+"-viewbox").c_str(), boxString);
  propList.insert((std::string("draw:marker-")+wh+"-width").c_str(), 10, librevenge::RVNG_POINT); // change me
}

void libfreehand::FHCollector::_appendBasicLine(librevenge::RVNGPropertyList &propList, const libfreehand::FHBasicLine *basicLine)
{
  if (!basicLine)
    return;
  // osnola: we do not want to change draw:stroke, if stroke is defined recursively
  propList.insert("draw:stroke", "solid");
  librevenge::RVNGString color = getColorString(basicLine->m_colorId);
  if (!color.empty())
    propList.insert("svg:stroke-color", color);
  else if (!propList["svg:stroke-color"]) // set to default
    propList.insert("svg:stroke-color", "#000000");
  propList.insert("svg:stroke-width", basicLine->m_width);
  _appendLinePattern(propList, _findLinePattern(basicLine->m_linePatternId));
  _appendArrowPath(propList, _findArrowPath(basicLine->m_startArrowId), true);
  _appendArrowPath(propList, _findArrowPath(basicLine->m_endArrowId), false);
}

void libfreehand::FHCollector::_appendCustomProcLine(librevenge::RVNGPropertyList &propList, const libfreehand::FHCustomProc *customProc)
{
  if (!customProc)
    return;
  propList.insert("draw:stroke", "solid");
  librevenge::RVNGString color;
  if (!customProc->m_ids.empty())
    color= getColorString(customProc->m_ids[0]);
  if (!color.empty())
    propList.insert("svg:stroke-color", color);
  if (!customProc->m_widths.empty())
    propList.insert("svg:stroke-width", customProc->m_widths[0], librevenge::RVNG_POINT);
}

void libfreehand::FHCollector::_appendPatternLine(librevenge::RVNGPropertyList &propList, const libfreehand::FHPatternLine *patternLine)
{
  if (!patternLine)
    return;
  propList.insert("draw:stroke", "solid");
  librevenge::RVNGString color = getColorString(patternLine->m_colorId, patternLine->m_percentPattern);
  if (!color.empty())
    propList.insert("svg:stroke-color", color);
  else if (!propList["svg:stroke-color"]) // set to default
    propList.insert("svg:stroke-color", "#000000");
  propList.insert("svg:stroke-width", patternLine->m_width);
}

const libfreehand::FHPath *libfreehand::FHCollector::_findPath(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHPath>::const_iterator iter = m_paths.find(id);
  if (iter != m_paths.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHNewBlend *libfreehand::FHCollector::_findNewBlend(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHNewBlend>::const_iterator iter = m_newBlends.find(id);
  if (iter != m_newBlends.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHGroup *libfreehand::FHCollector::_findGroup(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHGroup>::const_iterator iter = m_groups.find(id);
  if (iter != m_groups.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHGroup *libfreehand::FHCollector::_findClipGroup(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHGroup>::const_iterator iter = m_clipGroups.find(id);
  if (iter != m_clipGroups.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHCompositePath *libfreehand::FHCollector::_findCompositePath(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHCompositePath>::const_iterator iter = m_compositePaths.find(id);
  if (iter != m_compositePaths.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHPathText *libfreehand::FHCollector::_findPathText(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHPathText>::const_iterator iter = m_pathTexts.find(id);
  if (iter != m_pathTexts.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHTextObject *libfreehand::FHCollector::_findTextObject(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHTextObject>::const_iterator iter = m_textObjects.find(id);
  if (iter != m_textObjects.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHTransform *libfreehand::FHCollector::_findTransform(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHTransform>::const_iterator iter = m_transforms.find(id);
  if (iter != m_transforms.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHTEffect *libfreehand::FHCollector::_findTEffect(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHTEffect>::const_iterator iter = m_tEffects.find(id);
  if (iter != m_tEffects.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHParagraph *libfreehand::FHCollector::_findParagraph(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHParagraph>::const_iterator iter = m_paragraphs.find(id);
  if (iter != m_paragraphs.end())
    return &(iter->second);
  return nullptr;
}

const std::vector<libfreehand::FHTab> *libfreehand::FHCollector::_findTabTable(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, std::vector<libfreehand::FHTab> >::const_iterator iter = m_tabs.find(id);
  if (iter != m_tabs.end())
    return &(iter->second);
  return nullptr;
}

const std::vector<unsigned> *libfreehand::FHCollector::_findTStringElements(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, std::vector<unsigned> >::const_iterator iter = m_tStrings.find(id);
  if (iter != m_tStrings.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHPropList *libfreehand::FHCollector::_findPropList(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHPropList>::const_iterator iter = m_propertyLists.find(id);
  if (iter != m_propertyLists.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHGraphicStyle *libfreehand::FHCollector::_findGraphicStyle(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHGraphicStyle>::const_iterator iter = m_graphicStyles.find(id);
  if (iter != m_graphicStyles.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHBasicFill *libfreehand::FHCollector::_findBasicFill(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHBasicFill>::const_iterator iter = m_basicFills.find(id);
  if (iter != m_basicFills.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHLinearFill *libfreehand::FHCollector::_findLinearFill(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHLinearFill>::const_iterator iter = m_linearFills.find(id);
  if (iter != m_linearFills.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHLensFill *libfreehand::FHCollector::_findLensFill(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHLensFill>::const_iterator iter = m_lensFills.find(id);
  if (iter != m_lensFills.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHRadialFill *libfreehand::FHCollector::_findRadialFill(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHRadialFill>::const_iterator iter = m_radialFills.find(id);
  if (iter != m_radialFills.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHTileFill *libfreehand::FHCollector::_findTileFill(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHTileFill>::const_iterator iter = m_tileFills.find(id);
  if (iter != m_tileFills.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHPatternFill *libfreehand::FHCollector::_findPatternFill(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHPatternFill>::const_iterator iter = m_patternFills.find(id);
  if (iter != m_patternFills.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHLinePattern *libfreehand::FHCollector::_findLinePattern(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHLinePattern>::const_iterator iter = m_linePatterns.find(id);
  if (iter != m_linePatterns.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHPath *libfreehand::FHCollector::_findArrowPath(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHPath>::const_iterator iter = m_arrowPaths.find(id);
  if (iter != m_arrowPaths.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHBasicLine *libfreehand::FHCollector::_findBasicLine(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHBasicLine>::const_iterator iter = m_basicLines.find(id);
  if (iter != m_basicLines.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHCustomProc *libfreehand::FHCollector::_findCustomProc(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHCustomProc>::const_iterator iter = m_customProcs.find(id);
  if (iter != m_customProcs.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHPatternLine *libfreehand::FHCollector::_findPatternLine(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHPatternLine>::const_iterator iter = m_patternLines.find(id);
  if (iter != m_patternLines.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHRGBColor *libfreehand::FHCollector::_findRGBColor(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHRGBColor>::const_iterator iter = m_rgbColors.find(id);
  if (iter != m_rgbColors.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHTintColor *libfreehand::FHCollector::_findTintColor(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHTintColor>::const_iterator iter = m_tints.find(id);
  if (iter != m_tints.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHDisplayText *libfreehand::FHCollector::_findDisplayText(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHDisplayText>::const_iterator iter = m_displayTexts.find(id);
  if (iter != m_displayTexts.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHImageImport *libfreehand::FHCollector::_findImageImport(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHImageImport>::const_iterator iter = m_images.find(id);
  if (iter != m_images.end())
    return &(iter->second);
  return nullptr;
}

const librevenge::RVNGBinaryData *libfreehand::FHCollector::_findData(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, librevenge::RVNGBinaryData>::const_iterator iter = m_data.find(id);
  if (iter != m_data.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHSymbolClass *libfreehand::FHCollector::_findSymbolClass(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHSymbolClass>::const_iterator iter = m_symbolClasses.find(id);
  if (iter != m_symbolClasses.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHSymbolInstance *libfreehand::FHCollector::_findSymbolInstance(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHSymbolInstance>::const_iterator iter = m_symbolInstances.find(id);
  if (iter != m_symbolInstances.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FHFilterAttributeHolder *libfreehand::FHCollector::_findFilterAttributeHolder(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FHFilterAttributeHolder>::const_iterator iter = m_filterAttributeHolders.find(id);
  if (iter != m_filterAttributeHolders.end())
    return &(iter->second);
  return nullptr;
}

const std::vector<libfreehand::FHColorStop> *libfreehand::FHCollector::_findMultiColorList(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, std::vector<libfreehand::FHColorStop> >::const_iterator iter = m_multiColorLists.find(id);
  if (iter != m_multiColorLists.end())
    return &(iter->second);
  return nullptr;
}

const double *libfreehand::FHCollector::_findOpacityFilter(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, double>::const_iterator iter = m_opacityFilters.find(id);
  if (iter != m_opacityFilters.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FWShadowFilter *libfreehand::FHCollector::_findFWShadowFilter(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FWShadowFilter>::const_iterator iter = m_shadowFilters.find(id);
  if (iter != m_shadowFilters.end())
    return &(iter->second);
  return nullptr;
}

const libfreehand::FWGlowFilter *libfreehand::FHCollector::_findFWGlowFilter(unsigned id)
{
  if (!id)
    return nullptr;
  std::map<unsigned, FWGlowFilter>::const_iterator iter = m_glowFilters.find(id);
  if (iter != m_glowFilters.end())
    return &(iter->second);
  return nullptr;
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
  for (unsigned int element : iter->second.m_elements)
  {
    unsigned valueId = _findValueFromAttribute(element);
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
  for (unsigned int element : iter->second.m_elements)
  {
    unsigned valueId = _findValueFromAttribute(element);
    // Add other fills if we support them
    if (_findBasicFill(valueId) || _findLinearFill(valueId)
        || _findLensFill(valueId) || _findRadialFill(valueId)
        || _findTileFill(valueId) || _findPatternFill(valueId) || _findCustomProc(valueId))
      fillId = valueId;
  }
  return fillId;
}

const libfreehand::FHFilterAttributeHolder *libfreehand::FHCollector::_findFilterAttributeHolder(const libfreehand::FHGraphicStyle &graphicStyle)
{
  unsigned listId = graphicStyle.m_attrId;
  if (!listId)
    return nullptr;
  std::map<unsigned, FHList>::const_iterator iter = m_lists.find(listId);
  if (iter == m_lists.end())
    return nullptr;
  for (unsigned int element : iter->second.m_elements)
  {
    const FHFilterAttributeHolder *attributeHolder = _findFilterAttributeHolder(element);

    if (attributeHolder)
      return attributeHolder;
  }
  return nullptr;
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

librevenge::RVNGBinaryData libfreehand::FHCollector::getImageData(unsigned id)
{
  std::map<unsigned, FHDataList>::const_iterator iter = m_dataLists.find(id);
  librevenge::RVNGBinaryData data;
  if (iter == m_dataLists.end())
    return data;
  for (unsigned int element : iter->second.m_elements)
  {
    const librevenge::RVNGBinaryData *pData = _findData(element);
    if (pData)
      data.append(*pData);
  }
  return data;
}

librevenge::RVNGString libfreehand::FHCollector::getColorString(unsigned id, double tintVal)
{
  FHRGBColor col;
  const FHRGBColor *color = _findRGBColor(id);
  if (color)
    col=*color;
  else
  {
    const FHTintColor *tint = _findTintColor(id);
    if (!tint)
      return librevenge::RVNGString();
    col=getRGBFromTint(*tint);
  }
  if (tintVal<=0 || tintVal>=1)
    return _getColorString(col);

  FHRGBColor finalColor;
  finalColor.m_red = col.m_red * tintVal + (1 - tintVal) * 65536;
  finalColor.m_green = col.m_green * tintVal + (1 - tintVal) * 65536;
  finalColor.m_blue = col.m_blue * tintVal + (1 - tintVal) * 65536;
  return _getColorString(finalColor);
}

libfreehand::FHRGBColor libfreehand::FHCollector::getRGBFromTint(const FHTintColor &tint)
{
  if (!tint.m_baseColorId)
    return FHRGBColor();
  const FHRGBColor *rgbColor = _findRGBColor(tint.m_baseColorId);
  if (!rgbColor)
    return FHRGBColor();
  unsigned red = rgbColor->m_red * tint.m_tint + ((65536 - tint.m_tint) << 16);
  unsigned green = rgbColor->m_green * tint.m_tint + ((65536 - tint.m_tint) << 16);
  unsigned blue = rgbColor->m_blue * tint.m_tint + ((65536 - tint.m_tint) << 16);
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
