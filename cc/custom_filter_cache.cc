// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/custom_filter_cache.h"

#include "third_party/WebKit/Source/Platform/chromium/public/WebGraphicsContext3D.h"
#include "cc/custom_filter_mesh.h"
#include "cc/gl_renderer.h"
//#include "third_party/khronos/GLES2/gl2.h"

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
}

int CustomFilterCache::sharedFrameBuffer() {
    if (!m_sharedFrameBuffer)
        m_sharedFrameBuffer = GLC(m_context, m_context->createFramebuffer());
    return m_sharedFrameBuffer;
}

int CustomFilterCache::allocateRenderBuffer(int width, int height) { 
    WebKit::WebGLId depthBuffer = GLC(m_context, m_context->createRenderbuffer());
    GLC(m_context, m_context->bindRenderbuffer(GL_RENDERBUFFER, depthBuffer));
    GLC(m_context, m_context->renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height));
    return depthBuffer;
}

void CustomFilterCache::releaseRenderBuffer(int id) {
    GLC(m_context, m_context->deleteRenderbuffer(id));
}

CustomFilterCacheMesh CustomFilterCache::allocateMesh(unsigned columns, unsigned rows, WebKit::WebCustomFilterMeshType meshType) {
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

    return CustomFilterCacheMesh(vertexBuffer, indexBuffer, mesh.bytesPerVertex(), mesh.indicesCount());
}

void CustomFilterCache::releaseMesh(const CustomFilterCacheMesh& mesh) {
    GLC(m_context, m_context->deleteBuffer(mesh.vertexBuffer()));
    GLC(m_context, m_context->deleteBuffer(mesh.indexBuffer()));
}

}  // namespace cc
