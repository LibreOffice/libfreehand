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

#include <librevenge-stream/librevenge-stream.h>

namespace libfreehand
{

class FHInternalStream : public librevenge::RVNGInputStream
{
public:
  FHInternalStream(librevenge::RVNGInputStream *input, unsigned long size, bool compressed=false);
  ~FHInternalStream() override {}
  bool isStructured() override
  {
    return false;
  }
  unsigned subStreamCount() override
  {
    return 0;
  }
  const char *subStreamName(unsigned) override
  {
    return nullptr;
  }
  bool existsSubStream(const char *) override
  {
    return false;
  }
  librevenge::RVNGInputStream *getSubStreamByName(const char *) override
  {
    return nullptr;
  }
  librevenge::RVNGInputStream *getSubStreamById(unsigned) override
  {
    return nullptr;
  }
  const unsigned char *read(unsigned long numBytes, unsigned long &numBytesRead) override;
  int seek(long offset, librevenge::RVNG_SEEK_TYPE seekType) override;
  long tell() override;
  bool isEnd() override;
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
