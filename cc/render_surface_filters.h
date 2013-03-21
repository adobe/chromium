// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#ifndef CC_RENDER_SURFACE_FILTERS_H_
#define CC_RENDER_SURFACE_FILTERS_H_

#include "cc/cc_export.h"

class GrContext;
class SkBitmap;

namespace gfx {
class SizeF;
}

namespace WebKit {
class WebFilterOperations;
class WebGraphicsContext3D;
}

namespace cc {

class CustomFilterCache;

class CC_EXPORT RenderSurfaceFilters {
public:
    static SkBitmap apply(const WebKit::WebFilterOperations& filters, unsigned textureId, const gfx::SizeF& surfaceSize, const gfx::SizeF& textureSize, WebKit::WebGraphicsContext3D*, GrContext*, WebKit::WebGraphicsContext3D*, CustomFilterCache*);
    static WebKit::WebFilterOperations optimize(const WebKit::WebFilterOperations& filters);

private:
    RenderSurfaceFilters();
};

}
#endif  // CC_RENDER_SURFACE_FILTERS_H_
