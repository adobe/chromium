// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_DNS_RESPONSE_H_
#define NET_BASE_DNS_RESPONSE_H_
#pragma once

#include "net/base/dns_query.h"

namespace net{

class AddressList;

// A class that encapsulates bits and pieces related to DNS response
// processing.
class DnsResponse {
 public:
  // Constructs an object with an IOBuffer large enough to read
  // one byte more than largest possible response, to detect malformed
  // responses; |query| is a pointer to the DnsQuery for which |this|
  // is supposed to be a response.
  explicit DnsResponse(DnsQuery* query);

  // Internal buffer accessor into which actual bytes of response will be
  // read.
  IOBufferWithSize* io_buffer() { return io_buffer_.get(); }

  // Parses response of size nbytes and puts address into |results|,
  // returns net_error code in case of failure.
  int Parse(int nbytes, AddressList* results);

 private:
  // The matching query; |this| is the response for |query_|.  We do not
  // own it, lifetime of |this| should be within the limits of lifetime of
  // |query_|.
  const DnsQuery* const query_;

  // Buffer into which response bytes are read.
  scoped_refptr<IOBufferWithSize> io_buffer_;

  DISALLOW_COPY_AND_ASSIGN(DnsResponse);
};

}  // namespace net

#endif  // NET_BASE_DNS_RESPONSE_H_
