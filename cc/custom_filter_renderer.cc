// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/custom_filter_renderer.h"

#include <iostream>

#include "ui/gfx/transform.h"
#include "cc/custom_filter_compiled_program.h"
#include "cc/custom_filter_mesh.h"
#include "cc/gl_renderer.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebCustomFilterParameter.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebCustomFilterProgram.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebCString.h"

namespace cc {

CustomFilterRenderer::CustomFilterRenderer(WebKit::WebGraphicsContext3D* context)
    : m_context(context)
{
}

CustomFilterRenderer::~CustomFilterRenderer()
{
}

static gfx::Transform orthoProjectionMatrix(float left, float right, float bottom, float top)
{
    // Use the standard formula to map the clipping frustum to the cube from
    // [-1, -1, -1] to [1, 1, 1].
    float deltaX = right - left;
    float deltaY = top - bottom;
    gfx::Transform proj;
    if (!deltaX || !deltaY)
        return proj;
    proj.matrix().setDouble(0, 0, 2.0f / deltaX);
    proj.matrix().setDouble(0, 3, -(right + left) / deltaX);
    proj.matrix().setDouble(1, 1, 2.0f / deltaY);
    proj.matrix().setDouble(1, 3, -(top + bottom) / deltaY);

    const float farValue = 10000;
    const float nearValue = -10000;
    proj.matrix().setDouble(2, 2, -2.0 / (farValue - nearValue));
    proj.matrix().setDouble(2, 3, -(farValue + nearValue) / (farValue - nearValue));

    return proj;
}

void CustomFilterRenderer::bindProgramArrayParameters(int uniformLocation, const WebKit::WebCustomFilterParameter& arrayParameter)
{
    assert(arrayParameter.type == WebKit::WebCustomFilterParameter::ParameterTypeArray);

    unsigned parameterSize = arrayParameter.values.size();
    std::vector<WebKit::WGC3Dfloat> floatVector;
    floatVector.reserve(parameterSize);

    for (unsigned i = 0; i < parameterSize; ++i)
        floatVector.push_back(arrayParameter.values[i]);

    GLC(m_context, m_context->uniform1fv(uniformLocation, parameterSize, &floatVector[0]));
}

void CustomFilterRenderer::bindProgramNumberParameters(int uniformLocation, const WebKit::WebCustomFilterParameter& numberParameter)
{
    assert(numberParameter.type == WebKit::WebCustomFilterParameter::ParameterTypeNumber);

    switch (numberParameter.values.size()) {
    case 1:
        GLC(m_context, m_context->uniform1f(uniformLocation, numberParameter.values[0]));
        break;
    case 2:
        GLC(m_context, m_context->uniform2f(uniformLocation, numberParameter.values[0], numberParameter.values[1]));
        break;
    case 3:
        GLC(m_context, m_context->uniform3f(uniformLocation, numberParameter.values[0], numberParameter.values[1], numberParameter.values[2]));
        break;
    case 4:
        GLC(m_context, m_context->uniform4f(uniformLocation, numberParameter.values[0], numberParameter.values[1], numberParameter.values[2], numberParameter.values[3]));
        break;
    default:
        NOTREACHED();
    }
}

static void matrixToColumnMajorFloatArray(const WebKit::WebTransformationMatrix& matrix, WebKit::WGC3Dfloat floatArray[16])
{
    // These are conversions from double to float.
    floatArray[0] = matrix.m11();
    floatArray[1] = matrix.m12();
    floatArray[2] = matrix.m13();
    floatArray[3] = matrix.m14();
    floatArray[4] = matrix.m21();
    floatArray[5] = matrix.m22();
    floatArray[6] = matrix.m23();
    floatArray[7] = matrix.m24();
    floatArray[8] = matrix.m31();
    floatArray[9] = matrix.m32();
    floatArray[10] = matrix.m33();
    floatArray[11] = matrix.m34();
    floatArray[12] = matrix.m41();
    floatArray[13] = matrix.m42();
    floatArray[14] = matrix.m43();
    floatArray[15] = matrix.m44();
}

void CustomFilterRenderer::bindProgramTransformParameter(WebKit::WGC3Dint uniformLocation, const WebKit::WebCustomFilterParameter& transformParameter, WebKit::WGC3Dsizei viewportWidth, WebKit::WGC3Dsizei viewportHeight)
{
    WebKit::WebTransformationMatrix matrix;
    matrix.makeIdentity();
    if (viewportWidth && viewportHeight) {
        // The viewport is a box with the size of 1 unit, so we are scaling up here to make sure that translations happen using real pixel
        // units. At the end we scale back down in order to map it back to the original box. Note that transforms come in reverse order, because it is
        // supposed to multiply to the left of the coordinates of the vertices.
        // Note that the origin (0, 0) of the viewport is in the middle of the context, so there's no need to change the origin of the transform
        // in order to rotate around the middle of mesh.
        matrix.scale3d(1.0 / viewportWidth, 1.0 / viewportHeight, 1.0);
        matrix.multiply(transformParameter.matrix);
        matrix.scale3d(viewportWidth, viewportHeight, 1.0);
    }
    WebKit::WGC3Dfloat glMatrix[16];
    matrixToColumnMajorFloatArray(matrix, glMatrix);
    m_context->uniformMatrix4fv(uniformLocation, 1, false, &glMatrix[0]);
}

void CustomFilterRenderer::bindProgramParameters(const WebKit::WebVector<WebKit::WebCustomFilterParameter>& parameters, CustomFilterCompiledProgram* compiledProgram, WebKit::WGC3Dsizei viewportWidth, WebKit::WGC3Dsizei viewportHeight)
{
    // FIXME: Find a way to reset uniforms that are not specified in CSS. This is needed to avoid using values
    // set by other previous rendered filters.
    // https://bugs.webkit.org/show_bug.cgi?id=76440
    size_t parametersSize = parameters.size();
    for (size_t i = 0; i < parametersSize; ++i) {
        WebKit::WebCustomFilterParameter parameter = parameters[i];
        WebKit::WGC3Dint uniformLocation = compiledProgram->uniformLocationByName(parameter.name);
        if (uniformLocation == -1)
            continue;
        switch (parameter.type) {
        case WebKit::WebCustomFilterParameter::ParameterTypeArray:
            bindProgramArrayParameters(uniformLocation, parameter);
            break;
        case WebKit::WebCustomFilterParameter::ParameterTypeNumber:
            bindProgramNumberParameters(uniformLocation, parameter);
            break;
        case WebKit::WebCustomFilterParameter::ParameterTypeTransform:
            bindProgramTransformParameter(uniformLocation, parameter, viewportWidth, viewportHeight);
            break;
        }
    }
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

void CustomFilterRenderer::render(const WebKit::WebFilterOperation& op, WebKit::WebGLId sourceTextureId, const gfx::SizeF& surfaceSize, const gfx::SizeF& textureSize, WebKit::WebGLId destinationTextureId)
{
    // Set up m_context.
    if (!m_context->makeContextCurrent())
        return;

    CustomFilterProgramImpl* programImpl = static_cast<CustomFilterProgramImpl*>(op.customFilterProgram());
    CustomFilterCompiledProgram* compiledProgram = programImpl->compiledProgram();
    if (!compiledProgram) {
        compiledProgram = new CustomFilterCompiledProgram(
            m_context, op.customFilterProgram()->vertexShader(), op.customFilterProgram()->fragmentShader(), op.customFilterProgram()->programType());
        // CustomFilterProgramImpl is going to take ownership of the compiled program.
        programImpl->setCompiledProgram(compiledProgram);
    }

    if (!compiledProgram->isInitialized()) {
        // FIXME: make sure we are just reusing the texture in this case.
        return;
    }

    // Surface size is the actual size of the area that we need to apply the filter,
    // but the texture can be bigger than that.
    WebKit::WGC3Dsizei width = surfaceSize.width();
    WebKit::WGC3Dsizei height = surfaceSize.height();
    WebKit::WGC3Dsizei textureWidth = textureSize.width();
    WebKit::WGC3Dsizei textureHeight = textureSize.height();

    float widthScale = (float)textureWidth / width;
    float heightScale = (float)textureHeight / height;

    GLC(m_context, m_context->enable(GL_DEPTH_TEST));

    // Create frame buffer.
    WebKit::WebGLId frameBuffer = GLC(m_context, m_context->createFramebuffer());
    GLC(m_context, m_context->bindFramebuffer(GL_FRAMEBUFFER, frameBuffer));

    // Attach texture to frame buffer.
    GLC(m_context, m_context->framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, destinationTextureId, 0));

    // Set up depth buffer.
    WebKit::WebGLId depthBuffer = GLC(m_context, m_context->createRenderbuffer());
    GLC(m_context, m_context->bindRenderbuffer(GL_RENDERBUFFER, depthBuffer));
    GLC(m_context, m_context->renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, textureWidth, textureHeight));

    // Attach depth buffer to frame buffer.
    GLC(m_context, m_context->framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer));

    // Set up viewport.
    GLC(m_context, m_context->enable(GL_SCISSOR_TEST));
    GLC(m_context, m_context->scissor(0, 0, width, height));
    GLC(m_context, m_context->viewport(0, 0, textureWidth, textureHeight));

    // Clear render buffers.
    GLC(m_context, m_context->clearColor(0.0, 0.0, 0.0, 0.0));
    GLC(m_context, m_context->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    WebKit::WebGLId program = compiledProgram->program();
    GLC(m_context, m_context->useProgram(program));

    // Generate a mesh.
    gfx::RectF meshBox(0.0, 0.0, 1.0, 1.0);
    CustomFilterMesh mesh(op.meshColumns(), op.meshRows(), meshBox, op.meshType());

    // Set up vertex buffer.
    WebKit::WebGLId vertexBuffer = GLC(m_context, m_context->createBuffer());
    GLC(m_context, m_context->bindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
    m_context->bufferData(GL_ARRAY_BUFFER, mesh.vertices().size() * sizeof(float), &mesh.vertices().at(0), GL_STATIC_DRAW);

    // Set up index buffer.
    WebKit::WebGLId indexBuffer = GLC(m_context, m_context->createBuffer());
    m_context->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    m_context->bufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices().size() * sizeof(uint16_t), &mesh.indices().at(0), GL_STATIC_DRAW);

    // Bind all vertex attributes.
    bindVertexAttribute(compiledProgram->positionAttribLocation(), PositionAttribSize, mesh.bytesPerVertex(), PositionAttribOffset);
    bindVertexAttribute(compiledProgram->texAttribLocation(), TexAttribSize, mesh.bytesPerVertex(), TexAttribOffset);
    bindVertexAttribute(compiledProgram->meshAttribLocation(), MeshAttribSize, mesh.bytesPerVertex(), MeshAttribOffset);
    if (op.meshType() == WebKit::WebMeshTypeDetached)
        bindVertexAttribute(compiledProgram->triangleAttribLocation(), TriangleAttribSize, mesh.bytesPerVertex(), TriangleAttribOffset);

    // Bind source texture.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sourceTextureId);

    // Bind uniform pointing to source texture.
    if (compiledProgram->samplerLocation() != -1) {
        DCHECK(op.customFilterProgram()->programType() == WebKit::WebProgramTypeBlendsElementTexture);
        glUniform1i(compiledProgram->samplerLocation(), 0);
    }

    if (compiledProgram->samplerScaleLocation() != -1) {
        DCHECK(op.customFilterProgram()->programType() == WebKit::WebProgramTypeBlendsElementTexture);
        GLC(m_context, m_context->uniform2f(compiledProgram->samplerScaleLocation(), 1.0 / widthScale, 1.0 / heightScale));
    }

    if (compiledProgram->projectionMatrixLocation() != -1) {
        // Bind projection matrix uniform.
        gfx::Transform projectionTransform = orthoProjectionMatrix(-widthScale / 2, widthScale / 2, -heightScale / 2, heightScale / 2);
        projectionTransform.matrix().preTranslate(-(widthScale - 1) / 2, -(heightScale - 1) / 2, 0.0);

        static float glMatrix[16];
        projectionTransform.matrix().asColMajorf(&glMatrix[0]);
        GLC(m_context, m_context->uniformMatrix4fv(compiledProgram->projectionMatrixLocation(), 1, false, &glMatrix[0]));
    }

    if (compiledProgram->meshSizeLocation() != -1)
        GLC(m_context, m_context->uniform2f(compiledProgram->meshSizeLocation(), op.meshColumns(), op.meshRows()));

    if (compiledProgram->tileSizeLocation() != -1)
        GLC(m_context, m_context->uniform2f(compiledProgram->tileSizeLocation(), 1.0 / op.meshColumns(), 1.0 / op.meshRows()));

    if (compiledProgram->meshBoxLocation() != -1) {
        // FIXME: This will change when filter margins will be implemented,
        // see https://bugs.webkit.org/show_bug.cgi?id=71400
        GLC(m_context, m_context->uniform4f(compiledProgram->meshBoxLocation(), -0.5, -0.5, 1.0, 1.0));
    }

    if (compiledProgram->samplerSizeLocation() != -1)
        GLC(m_context, m_context->uniform2f(compiledProgram->samplerSizeLocation(), width, height));

    // Bind author defined parameters.
    WebKit::WebVector<WebKit::WebCustomFilterParameter> parameters;
    op.customFilterParameters(parameters);
    bindProgramParameters(parameters, compiledProgram, width, height);

    // Draw!
    GLC(m_context, m_context->drawElements(GL_TRIANGLES, mesh.indicesCount(), GL_UNSIGNED_SHORT, 0));

    // Unbind vertex attributes.
    unbindVertexAttribute(compiledProgram->positionAttribLocation());
    unbindVertexAttribute(compiledProgram->texAttribLocation());
    unbindVertexAttribute(compiledProgram->meshAttribLocation());
    if (op.meshType() == WebKit::WebMeshTypeDetached)
        unbindVertexAttribute(compiledProgram->triangleAttribLocation());

    // TODO(mvujovic): Unbind built-in and custom uniforms.

    // Free resources.
    m_context->deleteFramebuffer(frameBuffer);
    m_context->deleteRenderbuffer(depthBuffer);
    m_context->deleteBuffer(vertexBuffer);
    m_context->deleteBuffer(indexBuffer);

    // Flush.
    GLC(m_context, m_context->flush());
}

}  // namespace cc