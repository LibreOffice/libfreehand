/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* libfreehand
 * Version: MPL 1.1 / GPLv2+ / LGPLv2+
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License or as specified alternatively below. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * Major Contributor(s):
 * Copyright (C) 2012 Fridrich Strba <fridrich.strba@bluewin.ch>
 *
 *
 * All Rights Reserved.
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPLv2+"), or
 * the GNU Lesser General Public License Version 2 or later (the "LGPLv2+"),
 * in which case the provisions of the GPLv2+ or the LGPLv2+ are applicable
 * instead of those above.
 */

#include <vector>
#include "FHStringVector.h"

namespace libfreehand
{
class FHStringVectorImpl
{
public:
  FHStringVectorImpl() : m_strings() {}
  FHStringVectorImpl(const FHStringVectorImpl &impl) : m_strings(impl.m_strings) {}
  ~FHStringVectorImpl() {}
  std::vector<WPXString> m_strings;
};

} // namespace libfreehand

libfreehand::FHStringVector::FHStringVector()
  : m_pImpl(new FHStringVectorImpl())
{
}

libfreehand::FHStringVector::FHStringVector(const FHStringVector &vec)
  : m_pImpl(new FHStringVectorImpl(*(vec.m_pImpl)))
{
}

libfreehand::FHStringVector::~FHStringVector()
{
}

libfreehand::FHStringVector &libfreehand::FHStringVector::operator=(const FHStringVector &vec)
{
  if (m_pImpl)
    delete m_pImpl;
  m_pImpl = new FHStringVectorImpl(*(vec.m_pImpl));
  return *this;
}

unsigned libfreehand::FHStringVector::size() const
{
  return (unsigned)(m_pImpl->m_strings.size());
}

bool libfreehand::FHStringVector::empty() const
{
  return m_pImpl->m_strings.empty();
}

const WPXString &libfreehand::FHStringVector::operator[](unsigned idx) const
{
  return m_pImpl->m_strings[idx];
}

void libfreehand::FHStringVector::append(const WPXString &str)
{
  m_pImpl->m_strings.push_back(str);
}

void libfreehand::FHStringVector::clear()
{
  m_pImpl->m_strings.clear();
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
