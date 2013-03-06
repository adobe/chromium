// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_CUSTOM_FILTER_MESH_H
#define CC_CUSTOM_FILTER_MESH_H

#include <vector>

#include "cc/cc_export.h"
#include "ui/gfx/rect_f.h"
#include "ui/gfx/size.h"
#include "ui/gfx/size_f.h"

namespace cc {

// This file is largely copied from WebKit's CustomFilterMeshGenerator.h/.cpp.
//
// These are the search/replace changes that were made when porting from WebKit to CC:
//
// CustomFilterMeshGenerator -> CustomFilterMesh
// IntSize -> gfx::Size
// FloatSize -> gfx::SizeF
// FloatRect -> gfx::RectF
// Vector -> std::vector
// reserveCapacity -> reserve
// append -> push_back
//

// Constants from WebKit's CustomFilterConstants.h:

enum CustomFilterMeshConstants {
    // Vertex attribute sizes
    PositionAttribSize = 4,
    TexAttribSize = 2,
    MeshAttribSize = 2,
    TriangleAttribSize = 3,
    // Vertex attribute offsets
    PositionAttribOffset = 0,
    TexAttribOffset = PositionAttribOffset + PositionAttribSize * sizeof(float),
    MeshAttribOffset = TexAttribOffset + TexAttribSize * sizeof(float),
    TriangleAttribOffset = MeshAttribOffset + MeshAttribSize * sizeof(float)
};

enum CustomFilterMeshType {
    MeshTypeAttached,
    MeshTypeDetached
};

class CC_EXPORT CustomFilterMesh {
public:
    // Lines and columns are the values passed in CSS. The result is vertex mesh that has 'rows' numbers of rows
    // and 'columns' number of columns with a total of 'rows + 1' * 'columns + 1' vertices.
    // MeshBox is the filtered area calculated defined using the border-box, padding-box, content-box or filter-box
    // attributes. A value of (0, 0, 1, 1) will cover the entire output surface.
    CustomFilterMesh(unsigned columns, unsigned rows, const gfx::RectF& meshBox, CustomFilterMeshType);
    ~CustomFilterMesh();

    const std::vector<float>& vertices() const { return m_vertices; }
    const std::vector<uint16_t>& indices() const { return m_indices; }

    const gfx::Size& points() const { return m_points; }
    unsigned pointsCount() const { return m_points.width() * m_points.height(); }

    const gfx::Size& tiles() const { return m_tiles; }
    unsigned tilesCount() const { return m_tiles.width() * m_tiles.height(); }

    unsigned indicesCount() const
    {
        const unsigned trianglesPerTile = 2;
        const unsigned indicesPerTriangle = 3;
        return tilesCount() * trianglesPerTile * indicesPerTriangle;
    }

    unsigned floatsPerVertex() const
    {
        static const unsigned AttachedMeshVertexSize = PositionAttribSize + TexAttribSize + MeshAttribSize;
        static const unsigned DetachedMeshVertexSize = AttachedMeshVertexSize + TriangleAttribSize;
        return m_meshType == MeshTypeAttached ? AttachedMeshVertexSize : DetachedMeshVertexSize;
    }

    unsigned verticesCount() const
    {
        return m_meshType == MeshTypeAttached ? pointsCount() : indicesCount();
    }

private:
    typedef void (CustomFilterMesh::*AddTriangleVertexFunction)(int quadX, int quadY, int triangleX, int triangleY, int triangle);

    template <AddTriangleVertexFunction addTriangleVertex>
    void addTile(int quadX, int quadY)
    {
        ((*this).*(addTriangleVertex))(quadX, quadY, 0, 0, 1);
        ((*this).*(addTriangleVertex))(quadX, quadY, 1, 0, 2);
        ((*this).*(addTriangleVertex))(quadX, quadY, 1, 1, 3);
        ((*this).*(addTriangleVertex))(quadX, quadY, 0, 0, 4);
        ((*this).*(addTriangleVertex))(quadX, quadY, 1, 1, 5);
        ((*this).*(addTriangleVertex))(quadX, quadY, 0, 1, 6);
    }

    void addAttachedMeshIndex(int quadX, int quadY, int triangleX, int triangleY, int triangle);

    void generateAttachedMesh();

    void addDetachedMeshVertexAndIndex(int quadX, int quadY, int triangleX, int triangleY, int triangle);

    void generateDetachedMesh();
    void addPositionAttribute(int quadX, int quadY);
    void addTexCoordAttribute(int quadX, int quadY);
    void addMeshCoordAttribute(int quadX, int quadY);
    void addTriangleCoordAttribute(int quadX, int quadY, int triangle);
    void addAttachedMeshVertexAttributes(int quadX, int quadY);
    void addDetachedMeshVertexAttributes(int quadX, int quadY, int triangleX, int triangleY, int triangle);

#ifndef NDEBUG
    void dumpBuffers() const;
#endif

private:
    std::vector<float> m_vertices;
    std::vector<uint16_t> m_indices;

    CustomFilterMeshType m_meshType;
    gfx::Size m_points;
    gfx::Size m_tiles;
    gfx::SizeF m_tileSizeInPixels;
    gfx::SizeF m_tileSizeInDeviceSpace;
    gfx::RectF m_meshBox;
};

} // namespace cc

#endif // CC_CUSTOM_FILTER_MESH_H
