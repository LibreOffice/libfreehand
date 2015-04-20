/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __FHCOLLECTOR_H__
#define __FHCOLLECTOR_H__

#include <map>
#include <stack>
#include <librevenge/librevenge.h>
#include "FHCollector.h"
#include "FHTransform.h"
#include "FHTypes.h"
#include "FHPath.h"

namespace libfreehand
{

class FHCollector
{
public:
  FHCollector();
  virtual ~FHCollector();

  // collector functions
  void collectString(unsigned recordId, const librevenge::RVNGString &str);
  void collectPath(unsigned recordId, unsigned graphicStyle, unsigned layer,
                   const FHPath &path, bool evenodd);
  void collectXform(unsigned recordId, double m11, double m21,
                    double m12, double m22, double m13, double m23);
  void collectFHTail(unsigned recordId, const FHTail &fhTail);
  void collectBlock(unsigned recordId, const FHBlock &block);
  void collectList(unsigned recordId, const FHList &lst);
  void collectLayer(unsigned recordId, const FHLayer &layer);
  void collectGroup(unsigned recordId, const FHGroup &group);
  void collectCompositePath(unsigned recordId, const FHCompositePath &compositePath);
  void collectTString(unsigned recordId, const std::vector<unsigned> &elements);
  void collectAGDFont(unsigned recordId, const FHAGDFont &font);
  void collectParagraph(unsigned recordId, const FHParagraph &paragraph);
  void collectTextBlok(unsigned recordId, const std::vector<unsigned short> &characters);

  void collectPageInfo(const FHPageInfo &pageInfo);

  void outputContent(::librevenge::RVNGDrawingInterface *painter);

private:
  FHCollector(const FHCollector &);
  FHCollector &operator=(const FHCollector &);

  void _normalizePath(FHPath &path);
  void _outputPath(const FHPath *path, ::librevenge::RVNGDrawingInterface *painter);
  void _outputLayer(unsigned layerId, ::librevenge::RVNGDrawingInterface *painter);
  void _outputGroup(const FHGroup *group, ::librevenge::RVNGDrawingInterface *painter);
  void _outputCompositePath(const FHCompositePath *compositePath, ::librevenge::RVNGDrawingInterface *painter);

  bool _findListElements(std::vector<unsigned> &elements, unsigned id);

  const FHPath *_findPath(unsigned id);
  const FHGroup *_findGroup(unsigned id);
  const FHCompositePath *_findCompositePath(unsigned id);

  FHPageInfo m_pageInfo;
  FHTail m_fhTail;
  std::pair<unsigned, FHBlock> m_block;
  std::map<unsigned, FHTransform> m_transforms;
  std::map<unsigned, FHPath> m_paths;
  std::map<unsigned, librevenge::RVNGString> m_strings;
  std::map<unsigned, FHList> m_lists;
  std::map<unsigned, FHLayer> m_layers;
  std::map<unsigned, FHGroup> m_groups;
  std::stack<FHTransform> m_currentTransforms;
  std::map<unsigned, FHCompositePath> m_compositePaths;
  std::map<unsigned, std::vector<unsigned> > m_tStrings;
  std::map<unsigned, FHAGDFont> m_fonts;
  std::map<unsigned, FHParagraph> m_paragraphs;
  std::map<unsigned, std::vector<unsigned short> > m_textBloks;
};

} // namespace libfreehand

#endif /* __FHCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
