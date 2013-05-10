/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __FHSTRINGVECTOR_H__
#define __FHSTRINGVECTOR_H__

#include <libwpd/libwpd.h>

namespace libfreehand
{
class FHStringVectorImpl;

class FHStringVector
{
public:
  FHStringVector();
  FHStringVector(const FHStringVector &vec);
  ~FHStringVector();

  FHStringVector &operator=(const FHStringVector &vec);

  unsigned size() const;
  bool empty() const;
  const WPXString &operator[](unsigned idx) const;
  void append(const WPXString &str);
  void clear();

private:
  FHStringVectorImpl *m_pImpl;
};

} // namespace libfreehand

#endif /* __FHSTRINGVECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
