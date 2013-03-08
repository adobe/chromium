// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layer_tree_host_client.h"

#include "cc/context_provider.h"

namespace cc {

scoped_refptr<cc::ContextProvider> LayerTreeHostClient::CustomFilterContextProviderForMainThread() {
	return 0;
}

scoped_refptr<cc::ContextProvider> LayerTreeHostClient::CustomFilterContextProviderForCompositorThread() {
	return 0;
}

}
