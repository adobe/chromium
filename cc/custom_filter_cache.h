// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_CUSTOM_FILTER_CACHE_H
#define CC_CUSTOM_FILTER_CACHE_H

#include <base/memory/scoped_ptr.h>
#include "third_party/WebKit/Source/Platform/chromium/public/WebFilterOperation.h"

#include <vector>

namespace WebKit {
    class WebGraphicsContext3D;
}

namespace cc {

class CustomFilterCacheMesh {
public:
    CustomFilterCacheMesh()
        : m_vertexBuffer(0)
        , m_indexBuffer(0)
        , m_bytesPerVertex(0)
        , m_indicesCount(0)
        , m_columns(0)
        , m_rows(0)
        , m_meshType(WebKit::WebMeshTypeAttached)
        , m_useCount(0) {
    }
    CustomFilterCacheMesh(int vertexBuffer, int indexBuffer, int bytesPerVertex, 
        int indicesCount, int columns, int rows, WebKit::WebCustomFilterMeshType meshType)
        : m_vertexBuffer(vertexBuffer)
        , m_indexBuffer(indexBuffer)
        , m_bytesPerVertex(bytesPerVertex)
        , m_indicesCount(indicesCount)
        , m_columns(columns)
        , m_rows(rows)
        , m_meshType(meshType)
        , m_useCount(0) {
    }

    int vertexBuffer() const { return m_vertexBuffer; }
    int indexBuffer() const { return m_indexBuffer; }
    int bytesPerVertex() const { return m_bytesPerVertex; }
    int indicesCount() const { return m_indicesCount; }

    int columns() const { return m_columns; }
    int rows() const { return m_rows; }
    WebKit::WebCustomFilterMeshType meshType() const { return m_meshType; }

    int useCount() const { return m_useCount; }
    void resetUseCount() { m_useCount = 0; }
    void incrementUseCount() { ++m_useCount; }
private:
    int m_vertexBuffer;
    int m_indexBuffer;
    int m_bytesPerVertex;
    int m_indicesCount;

    int m_columns;
    int m_rows;
    WebKit::WebCustomFilterMeshType m_meshType;
    unsigned m_useCount;
};

class CustomFilterCacheBuffer {
public:
    CustomFilterCacheBuffer()
        : m_width(0)
        , m_height(0)
        , m_id(0)
        , m_useCount(0) {
    }
    CustomFilterCacheBuffer(int width, int height, int id)
        : m_width(width)
        , m_height(height)
        , m_id(id)
        , m_useCount(0) {
    }

    int width() const { return m_width; }
    int height() const { return m_height; }
    int id() const { return m_id; }

    int useCount() const { return m_useCount; }
    void resetUseCount() { m_useCount = 0; }
    void incrementUseCount() { ++m_useCount; }
private:
    int m_width;
    int m_height;
    int m_id;
    unsigned m_useCount;
};

class CustomFilterCache {
public:
    static scoped_ptr<CustomFilterCache> create(WebKit::WebGraphicsContext3D* context);
    ~CustomFilterCache();

    const CustomFilterCacheBuffer& allocateRenderBuffer(int width, int height);
    void releaseRenderBuffer(const CustomFilterCacheBuffer& renderBuffer);

    const CustomFilterCacheMesh& allocateMesh(unsigned columns, unsigned rows, WebKit::WebCustomFilterMeshType);
    void releaseMesh(const CustomFilterCacheMesh& mesh);

    int sharedFrameBuffer();

    void resetUseCount();
    void purgeUnusedResources();

private:
    CustomFilterCache(WebKit::WebGraphicsContext3D* context);
    void deleteRenderBuffer(const CustomFilterCacheBuffer& renderBuffer);
    void deleteMesh(const CustomFilterCacheMesh& mesh);

    std::vector<CustomFilterCacheBuffer> m_depthBuffers;
    std::vector<CustomFilterCacheMesh> m_meshBuffers;

    WebKit::WebGraphicsContext3D* m_context;
    int m_sharedFrameBuffer;
};

} // namespace cc

#endif // CC_CUSTOM_FILTER_CACHE_H
