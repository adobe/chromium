// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/custom_filter_mesh.h"

namespace cc {

#ifndef NDEBUG
// Use "call 'WebCore::s_dumpCustomFilterMeshBuffers' = 1" in GDB to activate printing of the mesh buffers.
static bool s_dumpCustomFilterMeshBuffers = false;
#endif

CustomFilterMesh::CustomFilterMesh(unsigned columns, unsigned rows, const gfx::RectF& meshBox, WebKit::WebCustomFilterMeshType meshType)
    : m_meshType(meshType)
    , m_points(columns + 1, rows + 1)
    , m_tiles(columns, rows)
    , m_tileSizeInPixels(meshBox.width() / m_tiles.width(), meshBox.height() / m_tiles.height())
    , m_tileSizeInDeviceSpace(1.0f / m_tiles.width(), 1.0f / m_tiles.height())
    , m_meshBox(meshBox)
{
    // Build the two buffers needed to draw triangles:
    // * m_vertices has a number of float attributes that will be passed to the vertex shader
    // for each computed vertex. This number is calculated in floatsPerVertex() based on the meshType.
    // * m_indices is a buffer that will have 3 indices per triangle. Each index will point inside
    // the m_vertices buffer.
    m_vertices.reserve(verticesCount() * floatsPerVertex());
    m_indices.reserve(indicesCount());

    // Based on the meshType there can be two types of meshes.
    // * attached: each triangle uses vertices from the neighbor triangles. This is useful to save some GPU memory
    // when there's no need to explode the tiles.
    // * detached: each triangle has its own vertices. This means each triangle can be moved independently and a vec3
    // attribute is passed, so that each vertex can be uniquely identified.
    if (m_meshType == WebKit::WebMeshTypeAttached)
        generateAttachedMesh();
    else
        generateDetachedMesh();

#ifndef NDEBUG
    if (s_dumpCustomFilterMeshBuffers)
        dumpBuffers();
#endif
}

CustomFilterMesh::~CustomFilterMesh()
{
}

void CustomFilterMesh::addAttachedMeshIndex(int quadX, int quadY, int triangleX, int triangleY, int triangle)
{
    // UNUSED_PARAM(triangle); // Macro unavailable in cc.
    m_indices.push_back((quadY + triangleY) * m_points.width() + (quadX + triangleX));
}

void CustomFilterMesh::generateAttachedMesh()
{
    for (int j = 0; j < m_points.height(); ++j) {
        for (int i = 0; i < m_points.width(); ++i)
            addAttachedMeshVertexAttributes(i, j);
    }

    for (int j = 0; j < m_tiles.height(); ++j) {
        for (int i = 0; i < m_tiles.width(); ++i)
            addTile<&CustomFilterMesh::addAttachedMeshIndex>(i, j);
    }
}

void CustomFilterMesh::addDetachedMeshVertexAndIndex(int quadX, int quadY, int triangleX, int triangleY, int triangle)
{
    addDetachedMeshVertexAttributes(quadX, quadY, triangleX, triangleY, triangle);
    m_indices.push_back(m_indices.size());
}

void CustomFilterMesh::generateDetachedMesh()
{
    for (int j = 0; j < m_tiles.height(); ++j) {
        for (int i = 0; i < m_tiles.width(); ++i)
            addTile<&CustomFilterMesh::addDetachedMeshVertexAndIndex>(i, j);
    }
}

void CustomFilterMesh::addPositionAttribute(int quadX, int quadY)
{
    // vec4 a_position
    m_vertices.push_back(m_tileSizeInPixels.width() * quadX - 0.5f + m_meshBox.x());
    m_vertices.push_back(m_tileSizeInPixels.height() * quadY - 0.5f + m_meshBox.y());
    m_vertices.push_back(0.0f); // z
    m_vertices.push_back(1.0f);
}

void CustomFilterMesh::addTexCoordAttribute(int quadX, int quadY)
{
    // vec2 a_texCoord
    m_vertices.push_back(m_tileSizeInPixels.width() * quadX + m_meshBox.x());
    m_vertices.push_back(m_tileSizeInPixels.height() * quadY + m_meshBox.y());
}

void CustomFilterMesh::addMeshCoordAttribute(int quadX, int quadY)
{
    // vec2 a_meshCoord
    m_vertices.push_back(m_tileSizeInDeviceSpace.width() * quadX);
    m_vertices.push_back(m_tileSizeInDeviceSpace.height() * quadY);
}

void CustomFilterMesh::addTriangleCoordAttribute(int quadX, int quadY, int triangle)
{
    // vec3 a_triangleCoord
    m_vertices.push_back(quadX);
    m_vertices.push_back(quadY);
    m_vertices.push_back(triangle);
}

void CustomFilterMesh::addAttachedMeshVertexAttributes(int quadX, int quadY)
{
    addPositionAttribute(quadX, quadY);
    addTexCoordAttribute(quadX, quadY);
    addMeshCoordAttribute(quadX, quadY);
}

void CustomFilterMesh::addDetachedMeshVertexAttributes(int quadX, int quadY, int triangleX, int triangleY, int triangle)
{
    addAttachedMeshVertexAttributes(quadX + triangleX, quadY + triangleY);
    addTriangleCoordAttribute(quadX, quadY, triangle);
}

#ifndef NDEBUG
void CustomFilterMesh::dumpBuffers() const
{
    printf("Mesh buffers: Points.width(): %d, Points.height(): %d meshBox: %f, %f, %f, %f, type: %s\n",
        m_points.width(), m_points.height(), m_meshBox.x(), m_meshBox.y(), m_meshBox.width(), m_meshBox.height(),
        (m_meshType == WebKit::WebMeshTypeAttached) ? "Attached" : "Detached");
    printf("---Vertex:\n\t");
    for (unsigned i = 0; i < m_vertices.size(); ++i) {
        printf("%f ", m_vertices.at(i));
        if (!((i + 1) % floatsPerVertex()))
            printf("\n\t");
    }
    printf("\n---Indices: ");
    for (unsigned i = 0; i < m_indices.size(); ++i)
        printf("%d ", m_indices.at(i));
    printf("\n");
}
#endif

}  // namespace cc
