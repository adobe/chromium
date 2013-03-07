// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_CUSTOM_FILTER_RENDERER_H_
#define CC_CUSTOM_FILTER_RENDERER_H_

#include "base/memory/scoped_ptr.h"
#include "cc/cc_export.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebGraphicsContext3D.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebFilterOperation.h"

namespace cc {

class CC_EXPORT CustomFilterRenderer {
public:
    static scoped_ptr<CustomFilterRenderer> create(WebKit::WebGraphicsContext3D* context);

    virtual ~CustomFilterRenderer();

    void render(const WebKit::WebFilterOperation& op, WebKit::WebGLId sourceTextureId, WebKit::WGC3Dsizei width, WebKit::WGC3Dsizei height, WebKit::WebGLId destinationTextureId);

protected:
    CustomFilterRenderer(WebKit::WebGraphicsContext3D* context);

private:
    DISALLOW_COPY_AND_ASSIGN(CustomFilterRenderer);

    void bindVertexAttribute(WebKit::WGC3Duint attributeLocation, WebKit::WGC3Dint size, WebKit::WGC3Duint bytesPerVertex, WebKit::WGC3Duint offset);
    void unbindVertexAttribute(WebKit::WGC3Duint attributeLocation);

    WebKit::WebGraphicsContext3D* m_context;
};

}

#endif  // CC_CUSTOM_FILTER_RENDERER_H_
