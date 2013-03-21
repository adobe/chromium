// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_CUSTOM_FILTER_CACHE_H
#define CC_CUSTOM_FILTER_CACHE_H

#include <base/memory/scoped_ptr.h>
#include "third_party/WebKit/Source/Platform/chromium/public/WebFilterOperation.h"

namespace WebKit {
    class WebGraphicsContext3D;
}

namespace cc {

class CustomFilterCacheMesh {
public:
    CustomFilterCacheMesh(int vertexBuffer, int indexBuffer, int bytesPerVertex, int indicesCount)
        : m_vertexBuffer(vertexBuffer)
        , m_indexBuffer(indexBuffer)
        , m_bytesPerVertex(bytesPerVertex)
        , m_indicesCount(indicesCount) {
    }

    int vertexBuffer() const { return m_vertexBuffer; }
    int indexBuffer() const { return m_indexBuffer; }
    int bytesPerVertex() const { return m_bytesPerVertex; }
    int indicesCount() const { return m_indicesCount; }

private:
    int m_vertexBuffer;
    int m_indexBuffer;
    int m_bytesPerVertex;
    int m_indicesCount;
};

class CustomFilterCache {
public:
    static scoped_ptr<CustomFilterCache> create(WebKit::WebGraphicsContext3D* context);
    ~CustomFilterCache();

    int allocateRenderBuffer(int width, int height);
    void releaseRenderBuffer(int id);

    CustomFilterCacheMesh allocateMesh(unsigned columns, unsigned rows, WebKit::WebCustomFilterMeshType);
    void releaseMesh(const CustomFilterCacheMesh& mesh);

    int sharedFrameBuffer();

private:
    CustomFilterCache(WebKit::WebGraphicsContext3D* context);

    WebKit::WebGraphicsContext3D* m_context;
    int m_sharedFrameBuffer;
};

} // namespace cc

#endif // CC_CUSTOM_FILTER_CACHE_H
