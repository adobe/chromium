// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/custom_filter_cache.h"

#include "base/debug/trace_event.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebGraphicsContext3D.h"
#include "cc/custom_filter_mesh.h"
#include "cc/gl_renderer.h"

namespace cc {

scoped_ptr<CustomFilterCache> CustomFilterCache::create(WebKit::WebGraphicsContext3D* context) {
    return make_scoped_ptr(new CustomFilterCache(context));
}

CustomFilterCache::CustomFilterCache(WebKit::WebGraphicsContext3D* context)
    : m_context(context)
    , m_sharedFrameBuffer(0) {
}

CustomFilterCache::~CustomFilterCache() {
    m_context->makeContextCurrent();
    if (m_sharedFrameBuffer)
        GLC(m_context, m_context->deleteFramebuffer(m_sharedFrameBuffer));
    for (unsigned i = 0; i < m_depthBuffers.size(); ++i) {
        const CustomFilterCacheBuffer& buffer = m_depthBuffers[i];
        deleteRenderBuffer(buffer);
    }
    for (unsigned i = 0; i < m_meshBuffers.size(); ++i) {
        const CustomFilterCacheMesh& cacheMesh = m_meshBuffers[i];
        deleteMesh(cacheMesh);
    }
}

void CustomFilterCache::deleteRenderBuffer(const CustomFilterCacheBuffer& buffer) {
    GLC(m_context, m_context->deleteRenderbuffer(buffer.id()));
}

void CustomFilterCache::deleteMesh(const CustomFilterCacheMesh& cacheMesh) {
    GLC(m_context, m_context->deleteBuffer(cacheMesh.vertexBuffer()));
    GLC(m_context, m_context->deleteBuffer(cacheMesh.indexBuffer()));
}

void CustomFilterCache::resetUseCount() {
    for (unsigned i = 0; i < m_depthBuffers.size(); ++i) {
        CustomFilterCacheBuffer& buffer = m_depthBuffers[i];
        buffer.resetUseCount();
    }
    for (unsigned i = 0; i < m_meshBuffers.size(); ++i) {
        CustomFilterCacheMesh& cacheMesh = m_meshBuffers[i];
        cacheMesh.resetUseCount();
    }
}

static bool sortDepthBuffersFunctor(const CustomFilterCacheBuffer& a, const CustomFilterCacheBuffer& b) {
    return a.useCount() > b.useCount();
}

static bool sortMeshBuffersFunctor(const CustomFilterCacheMesh& a, const CustomFilterCacheMesh& b) {
    return a.useCount() > b.useCount();
}

void CustomFilterCache::purgeUnusedResources() {
    TRACE_EVENT0("cc", "CustomFilterCache::purgeUnusedResources");

    m_context->makeContextCurrent();

    std::sort(m_depthBuffers.begin(), m_depthBuffers.end(), sortDepthBuffersFunctor);
    std::sort(m_meshBuffers.begin(), m_meshBuffers.end(), sortMeshBuffersFunctor);

    unsigned firstUnusedDepthBuffer = m_depthBuffers.size();
    for (unsigned i = 0; i < m_depthBuffers.size(); ++i) {
        if (!m_depthBuffers[i].useCount()) {
            firstUnusedDepthBuffer = i;
            break;
        }
    }
    for (unsigned i = firstUnusedDepthBuffer; i < m_depthBuffers.size(); ++i) {
        CustomFilterCacheBuffer& buffer = m_depthBuffers[i];
        deleteRenderBuffer(buffer);
    }
    m_depthBuffers.resize(firstUnusedDepthBuffer);

    unsigned firstUnusedMeshBuffer = m_meshBuffers.size();
    for (unsigned i = 0; i < m_meshBuffers.size(); ++i) {
        if (!m_meshBuffers[i].useCount()) {
            firstUnusedMeshBuffer = i;
            break;
        }
    }
    for (unsigned i = firstUnusedMeshBuffer; i < m_meshBuffers.size(); ++i) {
        CustomFilterCacheMesh& cacheMesh = m_meshBuffers[i];
        deleteMesh(cacheMesh);
    }
    m_meshBuffers.resize(firstUnusedMeshBuffer);
}

int CustomFilterCache::sharedFrameBuffer() {
    if (!m_sharedFrameBuffer)
        m_sharedFrameBuffer = GLC(m_context, m_context->createFramebuffer());
    return m_sharedFrameBuffer;
}

const CustomFilterCacheBuffer& CustomFilterCache::allocateRenderBuffer(int width, int height) {
    for (unsigned i = 0; i < m_depthBuffers.size(); ++i) {
        CustomFilterCacheBuffer& buffer = m_depthBuffers[i];
        if (buffer.width() == width && buffer.height() == height) {
            buffer.incrementUseCount();
            return buffer;
        }
    }
    TRACE_EVENT0("cc", "CustomFilterCache::allocateRenderBuffer");
    WebKit::WebGLId depthBuffer = GLC(m_context, m_context->createRenderbuffer());
    GLC(m_context, m_context->bindRenderbuffer(GL_RENDERBUFFER, depthBuffer));
    GLC(m_context, m_context->renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height));
    m_depthBuffers.push_back(CustomFilterCacheBuffer(width, height, depthBuffer));
    m_depthBuffers.back().incrementUseCount();
    return m_depthBuffers.back();
}

const CustomFilterCacheMesh& CustomFilterCache::allocateMesh(unsigned columns, unsigned rows, WebKit::WebCustomFilterMeshType meshType) {
    for (unsigned i = 0; i < m_meshBuffers.size(); ++i) {
        CustomFilterCacheMesh& cacheMesh = m_meshBuffers[i];
        if (cacheMesh.columns() == columns 
            && cacheMesh.rows() == rows
            && cacheMesh.meshType() == meshType) {
            cacheMesh.incrementUseCount();
            return cacheMesh;
        }
    }
    TRACE_EVENT0("cc", "CustomFilterCache::allocateMesh");
    gfx::RectF meshBox(0.0, 0.0, 1.0, 1.0);
    CustomFilterMesh mesh(columns, rows, meshBox, meshType);

    // Set up vertex buffer.
    WebKit::WebGLId vertexBuffer = GLC(m_context, m_context->createBuffer());
    GLC(m_context, m_context->bindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
    GLC(m_context, m_context->bufferData(GL_ARRAY_BUFFER, mesh.vertices().size() * sizeof(float), &mesh.vertices().at(0), GL_STATIC_DRAW));

    // Set up index buffer.
    WebKit::WebGLId indexBuffer = GLC(m_context, m_context->createBuffer());
    GLC(m_context, m_context->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer));
    GLC(m_context, m_context->bufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices().size() * sizeof(uint16_t), &mesh.indices().at(0), GL_STATIC_DRAW));

    m_meshBuffers.push_back(CustomFilterCacheMesh(vertexBuffer, indexBuffer, mesh.bytesPerVertex(), mesh.indicesCount(), columns, rows, meshType));
    m_meshBuffers.back().incrementUseCount();
    return m_meshBuffers.back();
}

}  // namespace cc
