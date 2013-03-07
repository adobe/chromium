// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/custom_filter_renderer.h"

#include <iostream>

#include "cc/custom_filter_compiled_program.h"
#include "cc/custom_filter_mesh.h"
#include "cc/gl_renderer.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebCustomFilterProgram.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebCString.h"

namespace cc {

scoped_ptr<CustomFilterRenderer> CustomFilterRenderer::create(WebKit::WebGraphicsContext3D* context)
{
    scoped_ptr<CustomFilterRenderer> renderer(make_scoped_ptr(new CustomFilterRenderer(context)));
    return renderer.Pass();
}

CustomFilterRenderer::CustomFilterRenderer(WebKit::WebGraphicsContext3D* context)
    : m_context(context)
{
}

CustomFilterRenderer::~CustomFilterRenderer()
{
}

static int inx(int row, int column)
{
    return 4 * (column - 1) + (row - 1);
}

static void identityMatrix(float matrix[16])
{
    for (int i = 0; i < 16; i++)
        matrix[i] = (i % 5 == 0) ? 1.0 : 0.0;
}

static void orthogonalProjectionMatrix(float matrix[16], float left, float right, float bottom, float top)
{
    // Start with an identity matrix.
    identityMatrix(matrix);

    float deltaX = right - left;
    float deltaY = top - bottom;
    if (!deltaX || !deltaY)
        return;
    matrix[inx(1, 1)] = 2.0 / deltaX;
    matrix[inx(4, 1)] = -(right + left) / deltaX;
    matrix[inx(2, 2)] = 2.0 / deltaY;
    matrix[inx(4, 2)] = -(top + bottom) / deltaY;

    // Use big enough near/far values, so that simple rotations of rather large objects will not
    // get clipped. 10000 should cover most of the screen resolutions.
    const float farValue = 10000;
    const float nearValue = -10000;
    matrix[inx(3, 3)] = -2.0 / (farValue - nearValue);
    matrix[inx(4, 3)] = -(farValue + nearValue) / (farValue - nearValue);
}

void CustomFilterRenderer::bindVertexAttribute(WebKit::WGC3Duint attributeLocation, WebKit::WGC3Dint size, WebKit::WGC3Duint bytesPerVertex, WebKit::WGC3Duint offset)
{
    if (attributeLocation != -1) {
        GLC(m_context, m_context->vertexAttribPointer(attributeLocation, size, GL_FLOAT, false, bytesPerVertex, offset));
        GLC(m_context, m_context->enableVertexAttribArray(attributeLocation));
    }
}

void CustomFilterRenderer::unbindVertexAttribute(WebKit::WGC3Duint attributeLocation)
{
    if (attributeLocation != -1)
        GLC(m_context, m_context->disableVertexAttribArray(attributeLocation));
}

void CustomFilterRenderer::render(const WebKit::WebFilterOperation& op, WebKit::WebGLId sourceTextureId, WebKit::WGC3Dsizei width, WebKit::WGC3Dsizei height, WebKit::WebGLId destinationTextureId)
{
    std::cerr << "Applying custom filter with destination texture id " << destinationTextureId << " and source texture id " << sourceTextureId << "." << std::endl;

    // Set up m_context.
    if (!m_context->makeContextCurrent()) {
        std::cerr << "Failed to make custom filters m_context current." << std::endl;
        return;
    }
    GLC(m_context, m_context->enable(GL_DEPTH_TEST));

    // Create frame buffer.
    WebKit::WebGLId frameBuffer = GLC(m_context, m_context->createFramebuffer());
    GLC(m_context, m_context->bindFramebuffer(GL_FRAMEBUFFER, frameBuffer));

    // Attach texture to frame buffer.
    GLC(m_context, m_context->framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, destinationTextureId, 0));

    // Set up depth buffer.
    WebKit::WebGLId depthBuffer = GLC(m_context, m_context->createRenderbuffer());
    GLC(m_context, m_context->bindRenderbuffer(GL_RENDERBUFFER, depthBuffer));
    GLC(m_context, m_context->renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height));

    // Attach depth buffer to frame buffer.
    GLC(m_context, m_context->framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer));

    // Set up viewport.
    GLC(m_context, m_context->viewport(0, 0, width, height));

    // Clear render buffers.
    GLC(m_context, m_context->clearColor(1.0, 0.0, 0.0, 1.0));
    GLC(m_context, m_context->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    scoped_ptr<CustomFilterCompiledProgram> compiledProgram = CustomFilterCompiledProgram::create(
        m_context, op.customFilterProgram()->vertexShader(), op.customFilterProgram()->fragmentShader(), 
        PROGRAM_TYPE_BLENDS_ELEMENT_TEXTURE); // TODO(mvujovic): Use the program type from WebCustomFilterProgram, when it's available there.
    WebKit::WebGLId program = compiledProgram->program();
    GLC(m_context, m_context->useProgram(program));

    // Generate a mesh.
    gfx::RectF meshBox(0.0, 0.0, 1.0, 1.0);
    CustomFilterMesh mesh(op.meshRows(), op.meshColumns(), meshBox, (CustomFilterMeshType)op.meshType());

    // Set up vertex buffer.
    WebKit::WebGLId vertexBuffer = GLC(m_context, m_context->createBuffer());
    GLC(m_context, m_context->bindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
    m_context->bufferData(GL_ARRAY_BUFFER, mesh.vertices().size() * sizeof(float), mesh.vertices().data(), GL_STATIC_DRAW);

    // Set up index buffer.
    WebKit::WebGLId indexBuffer = GLC(m_context, m_context->createBuffer());
    m_context->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    m_context->bufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices().size() * sizeof(uint16_t), mesh.indices().data(), GL_STATIC_DRAW);

    // Bind all vertex attributes.
    bindVertexAttribute(compiledProgram->positionAttribLocation(), PositionAttribSize, mesh.bytesPerVertex(), PositionAttribOffset);
    bindVertexAttribute(compiledProgram->texAttribLocation(), TexAttribSize, mesh.bytesPerVertex(), TexAttribOffset);
    bindVertexAttribute(compiledProgram->meshAttribLocation(), MeshAttribSize, mesh.bytesPerVertex(), MeshAttribOffset);
    if ((CustomFilterMeshType)op.meshType() == MeshTypeDetached)
        bindVertexAttribute(compiledProgram->triangleAttribLocation(), TriangleAttribSize, mesh.bytesPerVertex(), TriangleAttribOffset);

    // Bind source texture.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sourceTextureId);

    // Bind uniform pointing to source texture.
    WebKit::WGC3Dint sourceTextureUniformLocation = GLC(m_context, m_context->getUniformLocation(program, "css_u_texture"));
    glUniform1i(sourceTextureUniformLocation, 0);

    // Bind projection matrix uniform.
    WebKit::WGC3Dint projectionMatrixLocation = GLC(m_context, m_context->getUniformLocation(program, "u_projectionMatrix"));
    float projectionMatrix[16];
    orthogonalProjectionMatrix(projectionMatrix, -0.5, 0.5, -0.5, 0.5);
    GLC(m_context, m_context->uniformMatrix4fv(projectionMatrixLocation, 1, false, projectionMatrix));

    // Draw!
    GLC(m_context, m_context->drawElements(GL_TRIANGLES, mesh.indicesCount(), GL_UNSIGNED_SHORT, 0));
    std::cerr << "Draw elements!" << std::endl;

    // Unbind vertex attributes.
    unbindVertexAttribute(compiledProgram->positionAttribLocation());
    unbindVertexAttribute(compiledProgram->texAttribLocation());
    unbindVertexAttribute(compiledProgram->meshAttribLocation());
    if ((CustomFilterMeshType)op.meshType() == MeshTypeDetached)
        unbindVertexAttribute(compiledProgram->triangleAttribLocation());

    // TODO(mvujovic): Unbind built-in and custom uniforms.

    // Free resources.
    m_context->deleteFramebuffer(frameBuffer);
    m_context->deleteRenderbuffer(depthBuffer);
    m_context->deleteBuffer(vertexBuffer);
    m_context->deleteBuffer(indexBuffer);

    // Flush.
    GLC(m_context, m_context->flush());
    std::cerr << "Flush custom filter m_context." << std::endl;
}

}  // namespace cc