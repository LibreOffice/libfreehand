/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libfreehand project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */


#ifndef __FHINTERNALSTREAM_H__
#define __FHINTERNALSTREAM_H__

#include <vector>

#include <libwpd-stream/libwpd-stream.h>

namespace libfreehand
{

class FHInternalStream : public WPXInputStream
{
public:
  FHInternalStream(WPXInputStream *input, unsigned long size, bool compressed=false);
  ~FHInternalStream() {}

  bool isOLEStream()
  {
    return false;
  }
  WPXInputStream *getDocumentOLEStream(const char *)
  {
    return 0;
  }

  const unsigned char *read(unsigned long numBytes, unsigned long &numBytesRead);
  int seek(long offset, WPX_SEEK_TYPE seekType);
  long tell();
  bool atEOS();
  unsigned long getSize() const
  {
    return m_buffer.size();
  }

private:
  volatile long m_offset;
  std::vector<unsigned char> m_buffer;
  FHInternalStream(const FHInternalStream &);
  FHInternalStream &operator=(const FHInternalStream &);
};

} // namespace libfreehand

#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
