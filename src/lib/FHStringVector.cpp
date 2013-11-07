/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <vector>
#include <libfreehand/libfreehand.h>

namespace libfreehand
{
class FHStringVectorImpl
{
public:
  FHStringVectorImpl() : m_strings() {}
  FHStringVectorImpl(const FHStringVectorImpl &impl) : m_strings(impl.m_strings) {}
  ~FHStringVectorImpl() {}
  std::vector<librevenge::RVNGString> m_strings;
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
  delete m_pImpl;
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

const librevenge::RVNGString &libfreehand::FHStringVector::operator[](unsigned idx) const
{
  return m_pImpl->m_strings[idx];
}

void libfreehand::FHStringVector::append(const librevenge::RVNGString &str)
{
  m_pImpl->m_strings.push_back(str);
}

void libfreehand::FHStringVector::clear()
{
  m_pImpl->m_strings.clear();
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
