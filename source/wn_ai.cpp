//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-11 16:32:01
//

#include "wn_ai.h"
#include "wn_output.h"

class navmesh_ctx : public rcContext
{
public:
    navmesh_ctx()
        : rcContext()
    {}

protected:
    void doLog(const rcLogCategory category, const char* msg, const int len) override   
    {
        if (category != rcLogCategory::RC_LOG_PROGRESS) {
            ::log("%s", msg);
        }
    }
};

void navmesh_init(navmesh *mesh, const navmesh_build_info& info)
{
    /// @note(ame): calculate bounding box
    std::vector<f32> vertices;
    std::vector<i32> triangles;

    for (auto& vertex : info.vertices) {
        vertices.push_back(vertex.Position.x);
        vertices.push_back(vertex.Position.y);
        vertices.push_back(vertex.Position.z);
    }
    for (auto& tri : info.indices) {
        triangles.push_back(i32(tri));
    }

    const f32 agent_height = 2.0f;
    const f32 agent_max_climb = 0.9f;
    const f32 agent_radius = 0.5f;

    mesh->config = {};
    mesh->config.cs = 0.3f;
    mesh->config.ch = 0.2f;
    mesh->config.walkableHeight = (i32)ceilf(agent_height / mesh->config.ch);
	mesh->config.walkableClimb = (i32)floorf(agent_max_climb / mesh->config.ch);
	mesh->config.walkableRadius = (i32)ceilf(agent_radius / mesh->config.cs);
    mesh->config.walkableSlopeAngle = 45.0f;
	mesh->config.maxEdgeLen = (i32)(12.0f / mesh->config.cs);
    mesh->config.maxSimplificationError = 1.3f;
	mesh->config.minRegionArea = (i32)rcSqr(8);		// Note: area = size*size
	mesh->config.mergeRegionArea = (i32)rcSqr(20);	// Note: area = size*size
	mesh->config.maxVertsPerPoly = (i32)6.0f;
	mesh->config.detailSampleDist = 6.0f < 0.9f ? 0 : mesh->config.cs * 6.0f;
	mesh->config.detailSampleMaxError = mesh->config.ch * 1.0f;

    rcVcopy(mesh->config.bmin, glm::value_ptr(info.min));
	rcVcopy(mesh->config.bmax, glm::value_ptr(info.max));
    rcCalcGridSize(mesh->config.bmin, mesh->config.bmax, mesh->config.cs, &mesh->config.width, &mesh->config.height);

    mesh->height_field = rcAllocHeightfield();
    if (!mesh->height_field) {
        log("[navmesh] failed to allocate height field!");
        throw_error("AI error");
    }

    if (!rcCreateHeightfield(nullptr,
                             *mesh->height_field,
                             mesh->config.width,
                             mesh->config.height,
                             mesh->config.bmin,
                             mesh->config.bmax,
                             mesh->config.cs,
                             mesh->config.ch)) {
        log("[navmesh] failed to create height field!");
        throw_error("AI error");
    }

    u8* tri_areas = new u8[triangles.size()];
    memset(tri_areas, 0, triangles.size());
    /// @note(ame): mark walkable triangles
    {
        const f32 threshold = std::cosf(glm::radians(mesh->config.walkableSlopeAngle));

        for (i32 i = 0; i < info.indices.size(); i++) {
            if (info.vertices[info.indices[i]].Normals.y > threshold) {
                tri_areas[i] = RC_WALKABLE_AREA;
            }
        }
    }

    rcContext *ctx = new navmesh_ctx;
    ctx->enableLog(true);

    if (!rcRasterizeTriangles(ctx,
                              vertices.data(),
                              vertices.size(),
                              triangles.data(),
                              tri_areas,
                              triangles.size() / 3,
                              *mesh->height_field,
                              mesh->config.walkableClimb)) {
        log("[navmesh] failed to rasterize triangles!");
        throw_error("AI error");
    }

    mesh->compact_height_field = rcAllocCompactHeightfield();
    if (!mesh->compact_height_field) {
        log("[navmesh] failed to allocate compact heightfield!");
        throw_error("AI error");
    }

    if (!rcBuildCompactHeightfield(ctx, mesh->config.walkableHeight, mesh->config.walkableClimb, *mesh->height_field, *mesh->compact_height_field)) {
        log("[navmesh] failed to build compact heightfield!");
        throw_error("AI error");
    }

    if (!rcErodeWalkableArea(ctx, mesh->config.walkableRadius, *mesh->compact_height_field)) {
        log("[navmesh] failed to erode walkable area!");
        throw_error("AI error");
    }

    /// @note(ame): This uses the watershed navmesh approach -- its slow to compute but great since we'll bake them anyway.
    /// Check out this: https://github.com/recastnavigation/recastnavigation/blob/main/RecastDemo/Source/Sample_SoloMesh.cpp#L521

    if (!rcBuildDistanceField(ctx, *mesh->compact_height_field)) {
        log("[navmesh] failed to build distance field!");
        throw_error("AI error");
    }

    if (!rcBuildRegions(ctx, *mesh->compact_height_field, 0, mesh->config.minRegionArea, mesh->config.mergeRegionArea)) {
        log("[navmesh] failed to build navmesh regions!");
        throw_error("AI error");
    }

    mesh->contour_set = rcAllocContourSet();
    if (!mesh->contour_set) {
        log("[navmesh] failed to allocate contour set!");
        throw_error("AI error");
    }

    if (!rcBuildContours(ctx, *mesh->compact_height_field, mesh->config.maxSimplificationError, mesh->config.maxEdgeLen, *mesh->contour_set)) {
        log("[navmesh] failed to build contour set!");
        throw_error("AI error");
    }

    mesh->poly_mesh = rcAllocPolyMesh();
    if (!mesh->poly_mesh) {
        log("[navmesh] failed to allocate poly mesh!");
        throw_error("AI error");
    }

    if (!rcBuildPolyMesh(ctx, *mesh->contour_set, mesh->config.maxVertsPerPoly, *mesh->poly_mesh)) {
        log("[navmesh] failed to build poly mesh!");
        throw_error("AI error");
    }

    mesh->poly_mesh_detail = rcAllocPolyMeshDetail();
    if (!mesh->poly_mesh_detail) {
        log("[navmesh] failed to allocate poly mesh detail!");
        throw_error("AI error");
    }

    if (!rcBuildPolyMeshDetail(ctx, *mesh->poly_mesh, *mesh->compact_height_field, mesh->config.detailSampleDist, mesh->config.detailSampleMaxError, *mesh->poly_mesh_detail)) {
        log("[navmesh] failed to build poly mesh detail!");
        throw_error("AI error");
    }

    u8* nav_data;
    i32 nav_data_size;

    dtNavMeshCreateParams params;
	memset(&params, 0, sizeof(params));
	params.verts = mesh->poly_mesh->verts;
	params.vertCount = mesh->poly_mesh->nverts;
	params.polys = mesh->poly_mesh->polys;
	params.polyAreas = mesh->poly_mesh->areas;
	params.polyFlags = mesh->poly_mesh->flags;
	params.polyCount = mesh->poly_mesh->npolys;
	params.nvp = mesh->poly_mesh->nvp;
	params.detailMeshes = mesh->poly_mesh_detail->meshes;
	params.detailVerts = mesh->poly_mesh_detail->verts;
	params.detailVertsCount = mesh->poly_mesh_detail->nverts;
	params.detailTris = mesh->poly_mesh_detail->tris;
	params.detailTriCount = mesh->poly_mesh_detail->ntris;
	params.walkableHeight = agent_height;
	params.walkableRadius = agent_radius;
	params.walkableClimb = agent_max_climb;
	rcVcopy(params.bmin, mesh->poly_mesh->bmin);
	rcVcopy(params.bmax, mesh->poly_mesh->bmax);
	params.cs = mesh->config.cs;
	params.ch = mesh->config.ch;
	params.buildBvTree = true;

    if (!dtCreateNavMeshData(&params, &nav_data, &nav_data_size)) {
        log("[navmesh] failed to create navmesh data!");
        throw_error("AI error");
    }

    /// @todo(ame): Bake that
    ///
    //////

    mesh->mesh = dtAllocNavMesh();
    if (!mesh->mesh) {
        log("[navmesh] failed to allocate navmesh");
        throw_error("AI error");
    }

    dtStatus status = mesh->mesh->init(nav_data, nav_data_size, DT_TILE_FREE_DATA);
    if (dtStatusFailed(status)) {
        dtFree(nav_data);
        log("[navmesh] failed to create navmesh!");
        throw_error("AI error");
    }

    delete tri_areas;
    delete ctx;
}

void navmesh_free(navmesh *mesh)
{
    dtFreeNavMesh(mesh->mesh);

    rcFreePolyMeshDetail(mesh->poly_mesh_detail);
    rcFreePolyMesh(mesh->poly_mesh);
    rcFreeContourSet(mesh->contour_set);
    rcFreeCompactHeightfield(mesh->compact_height_field);
    rcFreeHeightField(mesh->height_field);
}
