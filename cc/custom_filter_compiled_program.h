// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_CUSTOM_FILTER_COMPILED_PROGRAM_H_
#define CC_CUSTOM_FILTER_COMPILED_PROGRAM_H_

#include "base/memory/scoped_ptr.h"
#include "cc/cc_export.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebGraphicsContext3D.h"

namespace cc {

// This file is largely copied from WebKit's CustomFilterCompiledProgram.h/.cpp.
//
// These are the search/replace changes that were made when porting from WebKit to CC:
//
// int -> WebKit::WGC3Dint
// Platform3DObject -> WebKit::WebGLId
// GraphicsContext3D -> WebKit::WebGraphicsContext3D
//
// Additionally, some conversions from WebString -> WebCString -> char* had to be done for the WebGraphicsContext3D functions.
//

// Constants from WebKit's CustomFilterConstants.h:

enum CustomFilterProgramType {
    PROGRAM_TYPE_NO_ELEMENT_TEXTURE,
    PROGRAM_TYPE_BLENDS_ELEMENT_TEXTURE
};

class CC_EXPORT CustomFilterCompiledProgram {
public:
    static scoped_ptr<CustomFilterCompiledProgram> create(WebKit::WebGraphicsContext3D* context, const WebKit::WebString& vertexShaderString, const WebKit::WebString& fragmentShaderString, CustomFilterProgramType programType)
    {
        scoped_ptr<CustomFilterCompiledProgram> renderer(make_scoped_ptr(new CustomFilterCompiledProgram(context, vertexShaderString, fragmentShaderString, programType)));
        return renderer.Pass();
    }
    
    ~CustomFilterCompiledProgram();
    
    WebKit::WGC3Dint positionAttribLocation() const { return m_positionAttribLocation; }
    WebKit::WGC3Dint texAttribLocation() const { return m_texAttribLocation; }
    WebKit::WGC3Dint meshAttribLocation() const { return m_meshAttribLocation; }
    WebKit::WGC3Dint triangleAttribLocation() const { return m_triangleAttribLocation; }
    WebKit::WGC3Dint meshBoxLocation() const { return m_meshBoxLocation; }
    WebKit::WGC3Dint projectionMatrixLocation() const { return m_projectionMatrixLocation; }
    WebKit::WGC3Dint tileSizeLocation() const { return m_tileSizeLocation; }
    WebKit::WGC3Dint meshSizeLocation() const { return m_meshSizeLocation; }
    WebKit::WGC3Dint samplerLocation() const { return m_samplerLocation; }
    WebKit::WGC3Dint contentSamplerLocation() const { return m_contentSamplerLocation; }
    WebKit::WGC3Dint samplerSizeLocation() const { return m_samplerSizeLocation; }

    WebKit::WGC3Dint uniformLocationByName(const WebKit::WebString&);
    
    bool isInitialized() const { return m_isInitialized; }
    
    WebKit::WebGLId program() const { return m_program; }
private:
    CustomFilterCompiledProgram(WebKit::WebGraphicsContext3D* context, const WebKit::WebString& vertexShaderString, const WebKit::WebString& fragmentShaderString, CustomFilterProgramType programType);
    
    WebKit::WebGLId compileShader(WebKit::WGC3Denum shaderType, const WebKit::WebString& shaderString);
    WebKit::WebGLId linkProgram(WebKit::WebGLId vertexShader, WebKit::WebGLId fragmentShader);
    void initializeParameterLocations(CustomFilterProgramType);
    
    WebKit::WebGraphicsContext3D* m_context;
    WebKit::WebGLId m_program;
    
    WebKit::WGC3Dint m_positionAttribLocation;
    WebKit::WGC3Dint m_texAttribLocation;
    WebKit::WGC3Dint m_meshAttribLocation;
    WebKit::WGC3Dint m_triangleAttribLocation;
    WebKit::WGC3Dint m_meshBoxLocation;
    WebKit::WGC3Dint m_projectionMatrixLocation;
    WebKit::WGC3Dint m_tileSizeLocation;
    WebKit::WGC3Dint m_meshSizeLocation;
    WebKit::WGC3Dint m_samplerLocation;
    WebKit::WGC3Dint m_samplerSizeLocation;
    WebKit::WGC3Dint m_contentSamplerLocation;
    
    bool m_isInitialized;
};

}

#endif
