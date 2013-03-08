// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/custom_filter_renderer.h"

#include <iostream>

#include "cc/custom_filter_compiled_program.h"
#include "cc/custom_filter_mesh.h"
#include "cc/gl_renderer.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebCustomFilterParameter.h"
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
    GLC(m_context, m_context->clearColor(0.0, 0.0, 0.0, 0.0));
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
    m_context->bufferData(GL_ARRAY_BUFFER, mesh.vertices().size() * sizeof(float), &mesh.vertices().at(0), GL_STATIC_DRAW);

    // Set up index buffer.
    WebKit::WebGLId indexBuffer = GLC(m_context, m_context->createBuffer());
    m_context->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    m_context->bufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices().size() * sizeof(uint16_t), &mesh.indices().at(0), GL_STATIC_DRAW);

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

    // Bind author defined parameters.
    WebKit::WebVector<WebKit::WebCustomFilterParameter> parameters;
    op.customFilterParameters(parameters);
    bindProgramParameters(parameters, compiledProgram.get(), width, height);

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