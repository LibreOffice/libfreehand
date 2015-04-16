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
  void collectUString(unsigned recordId, const librevenge::RVNGString &str);
  void collectMName(unsigned recordId, const librevenge::RVNGString &name);
  void collectPath(unsigned recordId, unsigned graphicStyle, unsigned layer,
                   const FHPath &path, bool evenodd);
  void collectXform(unsigned recordId, double m11, double m21,
                    double m12, double m22, double m13, double m23);
  void collectFHTail(unsigned recordId, unsigned blockId, unsigned propLstId, unsigned fontId);
  void collectBlock(unsigned recordId, unsigned layerListId, unsigned defaultLayerId);

  void collectPageInfo(const FHPageInfo &pageInfo);

  void outputContent(::librevenge::RVNGDrawingInterface *painter);

private:
  FHCollector(const FHCollector &);
  FHCollector &operator=(const FHCollector &);

  void _normalizePath(FHPath &path);
  void _outputPath(const FHPath &path, ::librevenge::RVNGDrawingInterface *painter);

  FHPageInfo m_pageInfo;
  FHTail m_fhTail;
  std::pair<unsigned, FHBlock> m_block;
  std::map<unsigned, FHTransform> m_transforms;
  std::map<unsigned, FHPath> m_paths;
  std::map<unsigned, librevenge::RVNGString> m_uStrings;
  std::map<unsigned, librevenge::RVNGString> m_mNames;
};

} // namespace libfreehand

#endif /* __FHCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
