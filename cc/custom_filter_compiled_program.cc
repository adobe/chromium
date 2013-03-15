// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/custom_filter_compiled_program.h"

#include <iostream>

#include "cc/gl_renderer.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebCString.h"

namespace cc {

CustomFilterCompiledProgram::CustomFilterCompiledProgram(WebKit::WebGraphicsContext3D* context, const WebKit::WebString& validatedVertexShader, const WebKit::WebString& validatedFragmentShader, WebKit::WebCustomFilterProgramType programType)
    : m_context(context)
    , m_program(0)
    , m_positionAttribLocation(-1)
    , m_texAttribLocation(-1)
    , m_meshAttribLocation(-1)
    , m_triangleAttribLocation(-1)
    , m_meshBoxLocation(-1)
    , m_projectionMatrixLocation(-1)
    , m_tileSizeLocation(-1)
    , m_meshSizeLocation(-1)
    , m_samplerLocation(-1)
    , m_samplerScaleLocation(-1)
    , m_samplerSizeLocation(-1)
    , m_contentSamplerLocation(-1)
    , m_isInitialized(false)
{
    m_context->makeContextCurrent();

    WebKit::WebGLId vertexShader = compileShader(GL_VERTEX_SHADER, validatedVertexShader);
    if (!vertexShader)
        return;
    
    WebKit::WebGLId fragmentShader = compileShader(GL_FRAGMENT_SHADER, validatedFragmentShader);
    if (!fragmentShader) {
        m_context->deleteShader(vertexShader);
        return;
    }
    
    m_program = linkProgram(vertexShader, fragmentShader);
    
    m_context->deleteShader(vertexShader);
    m_context->deleteShader(fragmentShader);
    
    if (!m_program)
        return;
    
    initializeParameterLocations(programType);
    
    m_isInitialized = true;
}

WebKit::WebGLId CustomFilterCompiledProgram::compileShader(WebKit::WGC3Denum shaderType, const WebKit::WebString& shaderString)
{
    assert(!shaderString.isNull());

    WebKit::WebGLId shader = m_context->createShader(shaderType);
    const WebKit::WebCString shaderCString = shaderString.utf8(); // CC-added code.
    m_context->shaderSource(shader, shaderCString.data());
    m_context->compileShader(shader);
    
    int compiled = 0;
    m_context->getShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        std::cerr << "Failed to compile shader." << std::endl; // CC-added code.
        // FIXME: This is an invalid shader. Throw some errors.
        // https://bugs.webkit.org/show_bug.cgi?id=74416
        m_context->deleteShader(shader);
        return 0;
    }
    
    return shader;
}

WebKit::WebGLId CustomFilterCompiledProgram::linkProgram(WebKit::WebGLId vertexShader, WebKit::WebGLId fragmentShader)
{
    WebKit::WebGLId program = m_context->createProgram();
    m_context->attachShader(program, vertexShader);
    m_context->attachShader(program, fragmentShader);
    m_context->linkProgram(program);
    
    int linked = 0;
    m_context->getProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        std::cerr << "Failed to link program." << std::endl; // CC-added code.
        // FIXME: Invalid vertex/fragment shader combination. Throw some errors here.
        // https://bugs.webkit.org/show_bug.cgi?id=74416
        m_context->deleteProgram(program);
        return 0;
    }
    
    return program;
}

void CustomFilterCompiledProgram::initializeParameterLocations(WebKit::WebCustomFilterProgramType programType)
{
    m_positionAttribLocation = m_context->getAttribLocation(m_program, "a_position");
    m_texAttribLocation = m_context->getAttribLocation(m_program, "a_texCoord");
    m_meshAttribLocation = m_context->getAttribLocation(m_program, "a_meshCoord");
    m_triangleAttribLocation = m_context->getAttribLocation(m_program, "a_triangleCoord");
    m_meshBoxLocation = m_context->getUniformLocation(m_program, "u_meshBox");
    m_tileSizeLocation = m_context->getUniformLocation(m_program, "u_tileSize");
    m_meshSizeLocation = m_context->getUniformLocation(m_program, "u_meshSize");
    m_projectionMatrixLocation = m_context->getUniformLocation(m_program, "u_projectionMatrix");
    m_samplerSizeLocation = m_context->getUniformLocation(m_program, "u_textureSize");
    m_contentSamplerLocation = m_context->getUniformLocation(m_program, "u_contentTexture");
    if (programType == WebKit::WebProgramTypeBlendsElementTexture) {
        // When the author uses the CSS mix function in a custom filter, WebKit adds the internal
        // symbol css_u_texture to the shader code, which references the texture of the element.
        m_samplerLocation = m_context->getUniformLocation(m_program, "css_u_texture");
        m_samplerScaleLocation = m_context->getUniformLocation(m_program, "css_u_textureScale");
    }
}

int CustomFilterCompiledProgram::uniformLocationByName(const WebKit::WebString& name)
{
    assert(m_isInitialized);
    // FIXME: Improve this by caching the uniform locations.
    const WebKit::WebCString nameCString = name.utf8(); // CC-added code.
    return m_context->getUniformLocation(m_program, nameCString.data());
}
    
CustomFilterCompiledProgram::~CustomFilterCompiledProgram()
{
    if (m_program) {
        m_context->makeContextCurrent();
        m_context->deleteProgram(m_program);
    }
}

}  // namespace cc
