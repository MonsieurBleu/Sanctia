#include <Subapps.hpp>
#include "App.hpp"
#include "Constants.hpp"
#include "EnvironementGenerator.hpp"
#include "Flags.hpp"
#include <SanctiaEntity.hpp>
#include "GameGlobals.hpp"
#include "JoltIntegration/PhysicsCommons.hpp"
#include "Navigation/NavMesh.hpp"
#include "PlayerController2.hpp"
#include "Scripting/ScriptInstance.hpp"
#include "Timer.hpp"
#include <AssetManager.hpp>
#include <EntityBlueprint.hpp>
#include "Game.hpp"

#include <recastnavigation/Recast.h>
#include <recastnavigation/DetourNavMesh.h>
#include <recastnavigation/DetourNavMeshQuery.h>
#include <recastnavigation/DetourNavMeshBuilder.h>

// debug, probably move this somewhere better when done debuggin'
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Geometry/OrientedBox.h>

#include <Blueprint/EngineBlueprintUI.hpp>



// namespace NavPath
// {
//     void resetGhost(const vec3 &pos);
// };

SpectatorController spectator;
extern PlayerController2 playerControl;

vec3 prevPlayerPosition;

Apps::LunaTesting::LunaTesting() : SubApps("Luna Testing")
{
    // inputs.push_back(&
    //     InputManager::addEventInput(
    //         "execute script", GLFW_KEY_R, 0, GLFW_PRESS, [&]() {    
    //             appRoot->comp<Script>().setInitialized(false);
    //         },
    //         InputManager::Filters::always, false)
    // );

    inputs.push_back(&
        InputManager::addEventInput(
            "test input", GLFW_KEY_T, 0, GLFW_PRESS, "test_input",
            InputManager::Filters::always, false)
    );

    // inputs.push_back(&
    //     InputManager::addEventInput(
    //         "gen navmesh", GLFW_KEY_L, 0, GLFW_PRESS, testRecast
    //     ));

    // inputs.push_back(&
    //     InputManager::addEventInput(
    //         "pathfind", GLFW_KEY_O, 0, GLFW_PRESS, [&]()
    //         {
    //             NavPath::resetGhost(GG::playerEntity->comp<State3D>().position + vec3(5, 0, 0));
    //         }
    //     ));

    inputs.push_back(&
        InputManager::addEventInput(
            "test gamepad input", VULPINE_GAMEPAD_BUTTON_A, 0, GLFW_PRESS, []() {    
                NOTIF_MESSAGE("Gamepad A button pressed!");
            }
        )
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle free cam", GLFW_KEY_F12, 0, GLFW_PRESS, [&]() { 
            if (globals.getController() == &playerControl && GG::playerEntity)
            {
                ComponentModularity::removeChild(*appRoot, GG::playerEntity);

                GG::playerEntity = newEntity();

                GG::ManageEntityGarbage__WithPhysics();

                App::setController(&spectator);
            }
            else if (globals.getController() == &spectator && GG::playerEntity)
            {
                GG::playerEntity = spawnEntityToParent("Jolt Player", *appRoot, HierarchyState3D({.position = prevPlayerPosition}));

                App::setController(&playerControl);
            }
        })
    );

    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::LunaTesting::UImenu()
{
    return newEntity("Lua Testing APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
    );
}

/*

// adapted from the recast demo
class BuildContext final : public rcContext
{
	std::array<BenchTimer, RC_MAX_TIMERS> timers;

	std::vector<std::string> logMessages;

public:
	BuildContext();

	/// Dumps the log to stdout.
	void dumpLog(const char* format, ...);
	/// Returns number of log messages.
	[[nodiscard]] int getLogCount() const;
	/// Returns log message text.
	[[nodiscard]] const char* getLogText(int i) const;

protected:
	void doResetLog() override;
	void doLog(rcLogCategory category, const char* msg, const int len) override;
	void doResetTimers() override;
	void doStartTimer(rcTimerLabel label) override;
	void doStopTimer(rcTimerLabel label) override;
	int doGetAccumulatedTime(rcTimerLabel label) const override;
};

BuildContext::BuildContext()
{
	resetTimers();
}

void BuildContext::doResetLog()
{
	logMessages.clear();
}

void BuildContext::doLog(const rcLogCategory category, const char* msg, const int len)
{
	if (len == 0)
	{
		return;
	}

	std::string& message = logMessages.emplace_back();
	switch (category)
	{
	case RC_LOG_PROGRESS:
		message.append("INFO:\t");
		break;
	case RC_LOG_WARNING:
		message.append("WARN:\t");
		break;
	case RC_LOG_ERROR:
		message.append("ERROR:\t");
		break;
	}
	message.append(msg);
}

void BuildContext::doResetTimers()
{
	for (int i = 0; i < RC_MAX_TIMERS; ++i)
	{
        rcTimerLabel labelID = (rcTimerLabel)i;
        const char* label = "";
        switch (labelID)
        {
            case RC_TIMER_TOTAL:
                label = "RC_TIMER_TOTAL";
                break;
            case RC_TIMER_TEMP:
                label = "RC_TIMER_TEMP";
                break;
            case RC_TIMER_RASTERIZE_TRIANGLES:
                label = "RC_TIMER_RASTERIZE_TRIANGLES";
                break;
            case RC_TIMER_BUILD_COMPACTHEIGHTFIELD:
                label = "RC_TIMER_BUILD_COMPACTHEIGHTFIELD";
                break;
            case RC_TIMER_BUILD_CONTOURS:
                label = "RC_TIMER_BUILD_CONTOURS";
                break;
            case RC_TIMER_BUILD_CONTOURS_TRACE:
                label = "RC_TIMER_BUILD_CONTOURS_TRACE";
                break;
            case RC_TIMER_BUILD_CONTOURS_SIMPLIFY:
                label = "RC_TIMER_BUILD_CONTOURS_SIMPLIFY";
                break;
            case RC_TIMER_FILTER_BORDER:
                label = "RC_TIMER_FILTER_BORDER";
                break;
            case RC_TIMER_FILTER_WALKABLE:
                label = "RC_TIMER_FILTER_WALKABLE";
                break;
            case RC_TIMER_MEDIAN_AREA:
                label = "RC_TIMER_MEDIAN_AREA";
                break;
            case RC_TIMER_FILTER_LOW_OBSTACLES:
                label = "RC_TIMER_FILTER_LOW_OBSTACLES";
                break;
            case RC_TIMER_BUILD_POLYMESH:
                label = "RC_TIMER_BUILD_POLYMESH";
                break;
            case RC_TIMER_MERGE_POLYMESH:
                label = "RC_TIMER_MERGE_POLYMESH";
                break;
            case RC_TIMER_ERODE_AREA:
                label = "RC_TIMER_ERODE_AREA";
                break;
            case RC_TIMER_MARK_BOX_AREA:
                label = "RC_TIMER_MARK_BOX_AREA";
                break;
            case RC_TIMER_MARK_CYLINDER_AREA:
                label = "RC_TIMER_MARK_CYLINDER_AREA";
                break;
            case RC_TIMER_MARK_CONVEXPOLY_AREA:
                label = "RC_TIMER_MARK_CONVEXPOLY_AREA";
                break;
            case RC_TIMER_BUILD_DISTANCEFIELD:
                label = "RC_TIMER_BUILD_DISTANCEFIELD";
                break;
            case RC_TIMER_BUILD_DISTANCEFIELD_DIST:
                label = "RC_TIMER_BUILD_DISTANCEFIELD_DIST";
                break;
            case RC_TIMER_BUILD_DISTANCEFIELD_BLUR:
                label = "RC_TIMER_BUILD_DISTANCEFIELD_BLUR";
                break;
            case RC_TIMER_BUILD_REGIONS:
                label = "RC_TIMER_BUILD_REGIONS";
                break;
            case RC_TIMER_BUILD_REGIONS_WATERSHED:
                label = "RC_TIMER_BUILD_REGIONS_WATERSHED";
                break;
            case RC_TIMER_BUILD_REGIONS_EXPAND:
                label = "RC_TIMER_BUILD_REGIONS_EXPAND";
                break;
            case RC_TIMER_BUILD_REGIONS_FLOOD:
                label = "RC_TIMER_BUILD_REGIONS_FLOOD";
                break;
            case RC_TIMER_BUILD_REGIONS_FILTER:
                label = "RC_TIMER_BUILD_REGIONS_FILTER";
                break;
            case RC_TIMER_BUILD_LAYERS:
                label = "RC_TIMER_BUILD_LAYERS,";
                break;
            case RC_TIMER_BUILD_POLYMESHDETAIL:
                label = "RC_TIMER_BUILD_POLYMESHDETAIL";
                break;
            case RC_TIMER_MERGE_POLYMESHDETAIL:
                label = "RC_TIMER_MERGE_POLYMESHDETAIL";
                break;
            case RC_MAX_TIMERS:
                label = "???";
                break;
        }
        timers[i] = BenchTimer(label);
	}
}

void BuildContext::doStartTimer(const rcTimerLabel label)
{
    timers[label].reset();
	timers[label].start();
}

void BuildContext::doStopTimer(const rcTimerLabel label)
{
	timers[label].stop();
}

int BuildContext::doGetAccumulatedTime(const rcTimerLabel label) const
{
	return timers[label].getElapsedTime() * 1000; // ms
}

void BuildContext::dumpLog(const char* format, ...)
{
	// Print header.
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	printf("\n");

	// Print messages
	const int TAB_STOPS[4] = {28, 36, 44, 52};
	for (int i = 0; i < static_cast<int>(logMessages.size()); ++i)
	{
		std::string& message = logMessages[i];
		const char* msg = message.c_str() + 1;
		int n = 0;
		while (*msg)
		{
			if (*msg == '\t')
			{
				int count = 1;
				for (int j = 0; j < 4; ++j)
				{
					if (n < TAB_STOPS[j])
					{
						count = TAB_STOPS[j] - n;
						break;
					}
				}
				while (--count)
				{
					putchar(' ');
					n++;
				}
			}
			else
			{
				putchar(*msg);
				n++;
			}
			msg++;
		}
		putchar('\n');
	}
}

int BuildContext::getLogCount() const
{
	return static_cast<int>(logMessages.size());
}

const char* BuildContext::getLogText(const int i) const
{
	return logMessages[i].c_str() + 1;
}

// for some frigging reason ceil is not registering as constexpr correctly in clang (even though it should)
constexpr int32_t constCeil(float num)
{
    return (static_cast<float>(static_cast<int32_t>(num)) == num)
        ? static_cast<int32_t>(num)
        : static_cast<int32_t>(num) + ((num > 0) ? 1 : 0);
}

// https://stackoverflow.com/questions/23781506/compute-at-compile-time-the-number-of-bits-needed-to-encode-n-different-states
constexpr unsigned floorlog2(unsigned x)
{
    return x == 1 ? 0 : 1+floorlog2(x >> 1);
}

constexpr unsigned ceillog2(unsigned x)
{
    return x == 1 ? 0 : floorlog2(x - 1) + 1;
}

inline rcPolyMeshDetail* detailMesh = nullptr;
inline rcHeightfield* heightfield = nullptr;
inline rcCompactHeightfield* compactHeightfield = nullptr;
inline rcContourSet* contourSet = nullptr;
inline rcPolyMesh* polyMesh = nullptr;

inline dtNavMesh* navMesh = nullptr;
inline dtNavMeshQuery* navQuery = nullptr;

void testRecast()
{
    const vec3 origin = GG::playerEntity->comp<State3D>().position;
    constexpr float radius = 300.0f;
    const JPH::AABox bounds = {(Vvec3)(origin - vec3(radius)), (Vvec3)(origin + vec3(radius))};

    struct NavGeometry {
        std::vector<vec3> verts;
        std::vector<ivec3>   tris;
        std::vector<u8> userAreas;
    };

    JPH::AllHitCollisionCollector<JPH::CollideShapeBodyCollector> hits;
    JoltVulpine::jPhysicsSystem->GetBroadPhaseQuery().CollideAABox(bounds, hits, JPH::BroadPhaseLayerFilter(), JPH::ObjectLayerFilter());

    NavGeometry geo;
    for (const JPH::BodyID &id : hits.mHits)
    {
        INFO_MESSAGE("building shape, ", id.GetIndex());
        JPH::BodyLockRead lock(JoltVulpine::jPhysicsSystem->GetBodyLockInterface(), id);
        if (lock.Succeeded() && lock.GetBody().IsStatic() && !lock.GetBody().IsSensor())
        {
            const JPH::Body &body = lock.GetBody();
            
            const JPH::AABox& bounds = body.GetWorldSpaceBounds();
            
            const JPH::TransformedShape parentShape = body.GetTransformedShape();
            JPH::AllHitCollisionCollector<JPH::TransformedShapeCollector> collector;
            JPH::ShapeFilter filter;
            parentShape.CollectTransformedShapes(bounds, collector, filter);

            for (int leafIdx = 0; leafIdx < collector.mHits.size(); leafIdx++)
            {
                JPH::TransformedShape leafShape = collector.mHits[leafIdx];
                JPH::Shape::GetTrianglesContext ctx;

                leafShape.GetTrianglesStart(ctx, bounds, JPH::RVec3::sZero());
                constexpr int N = 128;
                JPH::Float3 v[N * 3];
                size_t count = 0;
                while (int n = leafShape.GetTrianglesNext(ctx, N, v))
                {
                    for (int i = 0; i < n; ++i)
                    {
                        int base = geo.verts.size();
                        for (int k = 0; k < 3; ++k)
                            geo.verts.insert(geo.verts.end(), vec3( v[i*3+k].x, v[i*3+k].y, v[i*3+k].z ));
                        geo.tris.insert(geo.tris.end(), ivec3( base, base + 1, base + 2 ));
                        geo.userAreas.push_back(0);
                    }
                    count += n;

                    if (count > 1'000'000) break; // bad
                }

                INFO_MESSAGE(PRINTVAR(count), PRINTVAR(geo.tris.size()), PRINTVAR(geo.verts.size()))

                if (count > 1'000'000)
                {
                    INFO_MESSAGE("count: ", count);
                    exit(1);
                }
            }
        }
    }

    if (geo.tris.size() == 0)
    {
        WARNING_MESSAGE("geo build empty!");
        return;
    }

    INFO_MESSAGE("geo build complete");

    const float agentHeight = 2.0f;
    const float agentMaxClimb = 0.7f;
    const float agentRadius = 0.3f;

    size_t numVerts = geo.verts.size();
    size_t numTris = geo.tris.size();

    float* verts = (float*)geo.verts.data();
    int* tris = (int*)geo.tris.data();


    rcConfig config{};
    config.cs = 0.3f;
    config.ch = 0.2f;
    config.walkableSlopeAngle = 45.f;
    config.walkableHeight = (int)ceilf(agentHeight / config.ch);
    config.walkableClimb  = (int)floorf(agentMaxClimb / config.ch);
    config.walkableRadius = (int)ceilf(agentRadius / config.cs);
    config.maxEdgeLen = (int)(12.f / config.cs);
    config.maxSimplificationError = 1.3f;
    config.minRegionArea = rcSqr(8);  
    config.mergeRegionArea = rcSqr(20);
    config.maxVertsPerPoly = 6;
    config.detailSampleDist = config.cs * 6;
    config.detailSampleMaxError = config.ch;
    rcCalcBounds(verts, numVerts, config.bmin, config.bmax);
    rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

    unsigned char* triAreas = nullptr;
    heightfield = nullptr;
    compactHeightfield = nullptr;
    contourSet = nullptr;
    polyMesh = nullptr;
    detailMesh = nullptr;

    BuildContext* buildContext = new BuildContext();

    buildContext->resetTimers();
	buildContext->startTimer(RC_TIMER_TOTAL);

    // from recast sample
    heightfield = rcAllocHeightfield();
    if (!heightfield)
    {
        buildContext->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'heightfield'.");
        return;
    }
    if (!rcCreateHeightfield(
            buildContext,
            *heightfield,
            config.width,
            config.height,
            config.bmin,
            config.bmax,
            config.cs,
            config.ch))
    {
        buildContext->log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
        return;
    }

    triAreas = new unsigned char[numVerts];
    if (!triAreas)
    {
        buildContext->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'triAreas' (%d).", numTris);
        return;
    }
    memset(triAreas, 0, numVerts * sizeof(unsigned char));

    rcMarkWalkableTriangles(buildContext, config.walkableSlopeAngle, verts, numVerts, tris, numTris, triAreas);

    if (!rcRasterizeTriangles(buildContext, verts, numVerts, tris, triAreas, numTris, *heightfield, config.walkableClimb))
    {
        buildContext->log(RC_LOG_ERROR, "buildNavigation: Could not rasterize triangles.");
        return;
    }

    rcFilterLowHangingWalkableObstacles(buildContext, config.walkableClimb, *heightfield);
    rcFilterLedgeSpans(buildContext, config.walkableHeight, config.walkableClimb, *heightfield);
    rcFilterWalkableLowHeightSpans(buildContext, config.walkableHeight, *heightfield);

    compactHeightfield = rcAllocCompactHeightfield();
    if (!compactHeightfield)
    {
        buildContext->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
        return;
    }

    if (!rcBuildCompactHeightfield(
            buildContext,
            config.walkableHeight,
            config.walkableClimb,
            *heightfield,
            *compactHeightfield))
    {
        buildContext->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
        return;
    }

    if (!rcErodeWalkableArea(buildContext, config.walkableRadius, *compactHeightfield))
    {
        buildContext->log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
        return;
    }

    // layer partitioning, seemed like a good compromise
    if (!rcBuildLayerRegions(buildContext, *compactHeightfield, 0, config.minRegionArea))
    {
        buildContext->log(RC_LOG_ERROR, "buildNavigation: Could not build layer regions.");
        return;
    }

    contourSet = rcAllocContourSet();
    if (!contourSet)
    {
        buildContext->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'cset'.");
        return;
    }
    if (!rcBuildContours(buildContext, *compactHeightfield, config.maxSimplificationError, config.maxEdgeLen, *contourSet))
    {
        buildContext->log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
        return;
    }

    polyMesh = rcAllocPolyMesh();
    if (!polyMesh)
    {
        buildContext->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
        return;
    }
    if (!rcBuildPolyMesh(buildContext, *contourSet, config.maxVertsPerPoly, *polyMesh))
    {
        buildContext->log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
        return;
    }

    detailMesh = rcAllocPolyMeshDetail();
    if (!detailMesh)
    {
        buildContext->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmdtl'.");
        return;
    }
    if (!rcBuildPolyMeshDetail(
            buildContext,
            *polyMesh,
            *compactHeightfield,
            config.detailSampleDist,
            config.detailSampleMaxError,
            *detailMesh))
    {
        buildContext->log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
        return;
    }

    // done with recast :)

    if (config.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
	{
		unsigned char* navData = 0;
		int navDataSize = 0;

        triAreas = new unsigned char[numTris];
        memset(triAreas, 0, numTris * sizeof(unsigned char));

        for (int i = 0; i < polyMesh->npolys; ++i)
        {
            if (polyMesh->areas[i] == RC_WALKABLE_AREA)
                polyMesh->flags[i] = 1;
        }

		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = polyMesh->verts;
		params.vertCount = polyMesh->nverts;
		params.polys = polyMesh->polys;
		params.polyAreas = polyMesh->areas;
		params.polyFlags = polyMesh->flags;
		params.polyCount = polyMesh->npolys;
		params.nvp = polyMesh->nvp;
		params.detailMeshes = detailMesh->meshes;
		params.detailVerts = detailMesh->verts;
		params.detailVertsCount = detailMesh->nverts;
		params.detailTris = detailMesh->tris;
		params.detailTriCount = detailMesh->ntris;
		params.offMeshConVerts = nullptr;
		params.offMeshConRad = nullptr;
		params.offMeshConDir = nullptr;
		params.offMeshConAreas = nullptr;
		params.offMeshConFlags = nullptr;
		params.offMeshConUserID = nullptr;
		params.offMeshConCount = 0;
		params.walkableHeight = agentHeight;
		params.walkableRadius = agentRadius;
		params.walkableClimb = agentMaxClimb;
		rcVcopy(params.bmin, polyMesh->bmin);
		rcVcopy(params.bmax, polyMesh->bmax);
		params.cs = config.cs;
		params.ch = config.ch;
		params.buildBvTree = true;

        constexpr size_t tileSize = 5;

        constexpr size_t maxTiles = (radius / tileSize) * (radius / tileSize);
        constexpr size_t maxPolys = 1000;

        // bits for max tiles and max polys must fit on 22 bits
        static_assert((ceillog2(maxTiles) + ceillog2(maxPolys)) <= 22);

        dtNavMeshParams meshParams;
        meshParams.tileHeight = tileSize;
        meshParams.tileWidth = tileSize;
        meshParams.orig[0] = origin.x;
        meshParams.orig[1] = origin.y;
        meshParams.orig[2] = origin.z;
        meshParams.maxTiles = maxTiles;
        meshParams.maxPolys = maxPolys;


		if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
		{
			buildContext->log(RC_LOG_ERROR, "Could not build Detour navmesh.");
			return;
		}

		navMesh = dtAllocNavMesh();
		if (!navMesh)
		{
			dtFree(navData);
			buildContext->log(RC_LOG_ERROR, "Could not create Detour navmesh");
			return;
		}

        navQuery = dtAllocNavMeshQuery();
		dtStatus status = navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
		if (dtStatusFailed(status))
		{
			dtFree(navData);
			buildContext->log(RC_LOG_ERROR, "Could not init Detour navmesh");
			return;
		}

		status = navQuery->init(navMesh, 2048);
		if (dtStatusFailed(status))
		{
			buildContext->log(RC_LOG_ERROR, "Could not init Detour navmesh query");
			return;
		}


	}

    buildContext->stopTimer(RC_TIMER_TOTAL);
    auto totalTime = buildContext->getAccumulatedTime(RC_TIMER_TOTAL);
    float totalBuildTimeSec = static_cast<float>(totalTime) / 1000.0;
    NOTIF_MESSAGE(PRINTVAR(totalBuildTimeSec));
}

*/

void TestNavMeshUpdate()
{
    VulpineNav::NavMesh::DebugDrawTileInstanced(256.0f, 0.0f, true, false);
}


struct TestObject
{
    EntityRef entity;
    vec2 velocity;
};

constexpr int N_OBJECT = 1000;

std::array<TestObject, N_OBJECT> testArray;

EntityRef testParent = nullptr;

EntityRef myAwesomeEntity = nullptr;
void Apps::LunaTesting::init()
{
    const vec3 origin(-675, 37, -624);

    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot", state3D(true));

        App::setController(&playerControl);

        GG::sun->shadowCameraSize = vec2(2048);

        HierarchyState3D s;
        s.position = origin + vec3(0, 8, 0);
        GG::playerEntity = spawnEntityToParent("Jolt Player", *appRoot, s);
    }

    testParent = newEntity();
    ComponentModularity::addChild(*appRoot, testParent);

    std::random_device rd;
    std::mt19937 rng(rd());

    constexpr float rangeWorld = 100.0f;
    std::uniform_real_distribution<float> distWorld(-rangeWorld, rangeWorld);
    std::uniform_real_distribution<float> distAngle(0.0f, PI2);
    std::uniform_real_distribution<float> distVelocity(5.0f, 80.0f);
    
    for (int i = 0; i < N_OBJECT; i++)
    {
        TestObject obj;
        obj.entity = newEntity();

        // ComponentModularity::addChild(*testParent, obj.entity);

        vec2 spawnOffset = vec2(
            distWorld(rng),
            distWorld(rng));

        vec3 spawnPos = vec3(
            origin.x + spawnOffset.x,
            0.0f,
            origin.z + spawnOffset.y);

        obj.entity->set<State3D>({.position = spawnPos});

        // float angle = distAngle(rng);
        // obj.velocity = vec2(cos(angle), sin(angle)) * distVelocity(rng);

        obj.velocity = normalize(spawnOffset) * distVelocity(rng);

        obj.entity->set<AgentNavigation>({});

        testArray[i] = obj;
    }
    
    for(int x = 0; x < 4; x++)
        for(int y = 0; y < 4; y++) 
        {
            ComponentModularity::addChild(*appRoot, 
                spawnEntity("Jolt Test Ball", origin + vec3(0, 8 + x, y) + (rand()%8)/8.f)
            );

            ComponentModularity::addChild(*appRoot, 
                spawnEntity("Jolt Test Suzanne", origin + vec3(4 + rand()%2, 8 + x, y) + (rand()%8)/8.f)
            );

            // ComponentModularity::addChild(*appRoot, 
            //     spawnEntity("Jolt Player Alt", origin + vec3(4 + rand()%2, 8 + x, y) + (rand()%8)/8.f)
            // );
        }
        
    HierarchyState3D s;
    s.position = origin;
    spawnEntityToParent("Jolt Test Scene", *appRoot, s);
    
    ComponentModularity::addChild(*appRoot, Blueprint::SpawnMainGameTerrain());

    
    // JoltVulpine::jPhysicsSystem->OptimizeBroadPhase();

    JoltVulpine::enablePhysics = true;
    globals.simulationTime.resume();

    // if (Loader<ScriptInstance>::loadingInfos.find("test_ent") != Loader<ScriptInstance>::loadingInfos.end())
    // {
    //     Loader<ScriptInstance>::get("test_ent").run();
    // }


    // appRoot->set<Script>(Script(
    //     "test", ScriptHook::ON_INIT, 
    //     "test_step", ScriptHook::ON_UPDATE
    // ));

    // VulpineTextOutputRef out(new VulpineTextOutput());
    // EntityRef e = appRoot;
    // DataLoader<EntityRef>::write(e, out);
    // out->saveAs("data/test_ent.vEntity");

    // flags["test"] = "Hello World!";
    // flags["test2"] = 1354;
    // flags["test3"] = 13.54f;
    // flags["test4"] = true;
    // flags["test5"] = Flag::MakeFlagFromScript<int>("return_int");

    for(auto &i : Loader<Flag>::loadingInfos)
        std::cout << i.first << "\n";

    std::cout << Loader<Flag>::get("test4")->as_string() << "\n";


    // auto flags = Loader<Flags>::get("ModTest");
    // flags["CPP_FLAG"] = 1e6f;


    /*
        Exemple de code pour faire une sauvegarde.

        A la place de Flags, il faudra utiliser une struct qui ne charge qu'un certain type de flag
    */
    // std::string saveFolder = "data/saves/";
    // for(auto &i : Loader<Flags>::loadedAssets)
    // {
    //     VulpineTextOutputRef out2(new VulpineTextOutput(4096));
    //     i.second.writeAsSaveFileMode = true;
    //     DataLoader<Flags>::write(i.second, out2);
    //     i.second.writeAsSaveFileMode = false;

    //     std::string filename = saveFolder + i.first + ".vSavedFlags";
    //     out2->saveAs(filename.c_str());

    //     // NOTIF_MESSAGE(filename)
    // }

    // VulpineTextBuffRef in(new VulpineTextBuff("data/flags.vFlags"));
    // flags = DataLoader<Flags>::read(in);

    
    // std::cout << "Value of flag test5: " << flags["test5"]->as_string() << std::endl << std::endl;
    // if (flags["test"])
    // {
    //     std::cout << "Flag test is true!" << std::endl;
    // }
    // else {
    //     std::cout << "Flag test is false!" << std::endl;
    // }

    // std::cout << "Value of flag test2: " << flags["test2"]->as_int() << std::endl;

    // VulpineTextOutputRef out2(new VulpineTextOutput(4096));
    
    // DataLoader<Flags>::write(flags, out2);
    // out2->saveAs(Loader<Flags>::loadingInfos["ModTest"]->buff->getSource().c_str());

    // LogicBlock::registerFunction(
    //     LogicBlock::Function(
    //         "print_something",
    //         Flag::STRING,
    //         {Flag::STRING, Flag::STRING},
    //         [](const std::vector<FlagDataPtr>& args, Flags& flags) -> FlagDataPtr {
    //             std::cout << args[0]->as_string() << args[1]->as_string() << std::endl;
    //             return Flag::MakeFlag("done");
    //         }
    //     )
    // );
    LogicBlock::registerFunction(
        LogicBlock::Function(
            "pow",
            FlagData::FLOAT,
            {FlagData::FLOAT, FlagData::FLOAT},
            [](const std::vector<FlagDataPtr>& args) -> FlagDataPtr {
                float base = args[0]->as_float();
                float exponent = args[1]->as_float();
                return FlagData::MakeFlag(std::pow(base, exponent));
            }
        )
    );

    // LogicBlock::registerFunction(
    //     LogicBlock::Function(
    //         "getCompute",
    //         Flag::INT,
    //         {},
    //         [](const std::vector<FlagDataPtr>& args, Flags& flags) -> FlagDataPtr {
    //             return Flag::MakeFlag(flags["test2"]->as_int() * 2);
    //         }
    //     )
    // );

    // // std::string testStr = "Logic Test 1 Result: $(${test2} > ${test3} && (${test} == \"Hello World!\"))";
    // // std::string testStr = "Logic Test 2 Result: $(${test2} > 10)";
    // // std::string testStr = " 1 + 1 is: $(1 + 1)\n 5 - 3 is: $(5 - 3)\n 4 * 2 is: $(4 * 2)\n 8 / 4 is: $(8 / 4)";
    // // std::string testStr = "Logic Test 3 Result: $(!${test4} || !(${test2} > ${test3}))";
    // // std::string testStr = "String Concat Test Result: $(${test} + ' ' + ${test2})";
    // std::string testStr = "If then else Test Result: $(if (!${test4}) then (1) else (0)) $(if (false) then ('you shouldn't be seeing this'))";
    // // std::string testStr = "Inline if Test Result: $(${test4} && if (${test2} > 5) then (true) else (false))";
    // // std::string testStr = "Function Call Test Result: $(print_something('Hello from ', if (true) then ('function call!') else ('second option!)))";
    // // std::string testStr = "Power Function Test Result: $(pow(2, 2 * 4))";
    // // std::string testStr = "Operator Precedence Test Result: $(1 + 2 * 3)";
    
    // std::string testStr = "Operator Precedence Test Result: $(1 + 2 * 3) abcdefg";
    // std::string testStr = "if else: $(if (1 > 2) then (\"Hello\") else (\"Hi\")) abcdefg";
    // std::string testStr = "if else: $(if (1 > 2) then (\"Hello\") else (if (3 > 1) then (\"Heyyyyyyy\") else (\"Hiiiii\"))) abcdefg";
    // std::string testStr = "$(if (1 > 2) then (\"Hello\") else (\"World\")) $(1 + 1 1) $(1 12354656) $(if (1 + 1 + 1) then (1 + 1) else (false) 1)";
    // std::string testStr = "okay wait does this break??: $(1 + 1 1) $(1 + 1 1)";
    // std::string testStr = "nothing: $( () )";
    std::string testStr = "$(${test7})";

    size_t len = testStr.length();
    char *input = new char[len+1];
    strcpy(input, testStr.c_str());

    std::cout << len << "\t" << input << "\n";
    LogicBlock::parse_string_cstr(&input, len, len+1);
    std::cout << len << "\t" << input << "\n";

    // myAwesomeEntity->set<AudioPlayer>({});
    // AudioPlayer& p = myAwesomeEntity->comp<AudioPlayer>();

    // AudioClip& c = p.Play({.clipName = "FootstepsStoneDirt1Mono", .position = vec3(2, 0, 0), .positionInEntityReferential = true, .loop = true});

    // myAwesomeEntity->set<state3D>(state3D(origin));
    // myAwesomeEntity->set<EntityModel>(EntityModel{Loader<ObjectGroup>::get("chaise").copy()});

    /*
    myAwesomeEntity->set<AudioScatterer>({
        .clips = {
            "zapsplat_animals_budgies_x2_chirping_happy_001_75540",
            "zapsplat_animals_bird_ringneck_parakeet_kisses_x5_109602",
            "zapsplat_animals_bird_peewee_call_australia_003_11999",
            "glitched_tones_urban_farm_jumbo_quail_and_background_birds_01_366",
            // "audio_hero_BirdBlackThroatedDiver_DIGIC10-11"
        }, 
        .meanTime = 3.0f,
        .posOffset = vec3(0, 5.5, 0), 
        .halfExtents = vec3(30, 1, 30), 
    });

    GG::playerEntity->set<AudioPlayer>({});
    GG::playerEntity->set<FootstepsManager>({});
    */

    // constexpr int N = 1e6;
    // BenchTimer timer("Logic Block Parsing Benchmark");
    // int acctmp = 0;
    // for (int i = 0; i < N; i++)
    // {
    //     std::string testStr2 = testStr;

    //     timer.start();
    //     LogicBlock::parse_string(testStr2);
    //     timer.stop();

    //     acctmp += testStr2.length();
    // }

    // std::cout << timer << acctmp << "\n";

    // timer.reset();

    // acctmp = 0;
    // for (int i = 0; i < N; i++)
    // {
    //     size_t len = testStr.length();
    //     char *input = new char[len+1];
    //     strcpy(input, testStr.c_str());
        
    //     timer.start();
    //     LogicBlock::parse_string_cstr(&input, len, len+1);
    //     timer.stop();

    //     acctmp += len;
    // }

    // std::cout << timer << acctmp << "\n";

    // std::cout << timer << std::endl;
    // std::cout << "omg" << acctmp << std::endl;    
    // // std::cout << testStr << std::endl;
    

    // testRecast();
    // TestNavMeshInit();

    GG::playerEntity->set(AgentNavigation({.navmeshBuildPriority = 10}));


    glLineWidth(3.0f);
}

/*

vec3 ghostPos = vec3(0, 0, 0);

namespace NavPath
{
    constexpr int MAX_POLYS = 256;
    constexpr int MAX_STRAIGHT = 256;

    static std::vector<vec3> path;
    static size_t pathIdx = 0;
    static float repathTimer = 0.f;

    bool computePath(const vec3 &start, const vec3 &end, std::vector<vec3> &outPath)
    {
        outPath.clear();
        if (agentSlot == -1) return false;

        dtNavMesh* dtnavMesh;
        dtNavMeshQuery* navQuery;

        dtnavMesh = navMesh.GetNavMesh(agentSlot);
        navQuery = navMesh.GetNavMeshQuery(agentSlot);

        if (!dtnavMesh || !navQuery)
            return false;

        dtQueryFilter filter;
        filter.setIncludeFlags(0xffff);
        filter.setExcludeFlags(0);

        const float extents[3] = {2.f, 4.f, 2.f};

        dtPolyRef startRef = 0, endRef = 0;
        vec3 startNearest, endNearest;

        navQuery->findNearestPoly(&start.x, extents, &filter, &startRef, &startNearest.x);
        navQuery->findNearestPoly(&end.x,   extents, &filter, &endRef,   &endNearest.x);

        if (!startRef || !endRef)
            return false;

        dtPolyRef polys[MAX_POLYS];
        int npolys = 0;
        dtStatus st = navQuery->findPath(startRef, endRef,
                                         &startNearest.x, &endNearest.x,
                                         &filter, polys, &npolys, MAX_POLYS);
        if (dtStatusFailed(st) || npolys == 0)
            return false;

        vec3 realEnd = endNearest;
        if (polys[npolys - 1] != endRef)
            navQuery->closestPointOnPoly(polys[npolys - 1], &endNearest.x, &realEnd.x, nullptr);

        float straight[MAX_STRAIGHT * 3];
        unsigned char flags[MAX_STRAIGHT];
        dtPolyRef refs[MAX_STRAIGHT];
        int nStraight = 0;

        st = navQuery->findStraightPath(&startNearest.x, &realEnd.x,
                                        polys, npolys,
                                        straight, flags, refs,
                                        &nStraight, MAX_STRAIGHT);
        if (dtStatusFailed(st) || nStraight == 0)
            return false;

        outPath.reserve(nStraight);
        for (int i = 0; i < nStraight; ++i)
            outPath.emplace_back(straight[i * 3 + 0], straight[i * 3 + 1], straight[i * 3 + 2]);

        return true;
    }

    bool repathToPlayer()
    {
        const vec3 playerPos = GG::playerEntity->comp<State3D>().position;

        std::vector<vec3> newPath;
        if (!computePath(ghostPos, playerPos, newPath))
            return false;

        path = std::move(newPath);
        pathIdx = path.size() > 1 ? 1 : 0;
        return true;
    }

    void updateGhost(float dt, float speed = 4.0f, float repathInterval = 0.5f)
    {
        if (agentSlot == -1) return;

        dtNavMesh* dtnavMesh;
        dtNavMeshQuery* navQuery;

        dtnavMesh = navMesh.GetNavMesh(agentSlot);
        navQuery = navMesh.GetNavMeshQuery(agentSlot);

        if (!dtnavMesh || !navQuery || !GG::playerEntity)
            return;

        repathTimer -= dt;
        if (repathTimer <= 0.f || path.empty())
        {
            repathTimer = repathInterval;
            repathToPlayer();
        }

        if (path.empty() || pathIdx >= path.size())
            return;

        float remaining = speed * dt;
        while (remaining > 0.f && pathIdx < path.size())
        {
            const vec3 &target = path[pathIdx];
            const vec3 delta = target - ghostPos;
            const float dist = glm::length(delta);

            if (dist <= remaining)
            {
                ghostPos = target;
                remaining -= dist;
                pathIdx++;
            }
            else
            {
                ghostPos += delta / dist * remaining;
                remaining = 0.f;
            }
        }
    }

    void resetGhost(const vec3 &pos)
    {
        ghostPos = pos;
        path.clear();
        pathIdx = 0;
        repathTimer = 0.f;

        navMesh.UnloadLoader(prevGhostPos);
        navMesh.CreateLoader(ghostPos, 0);
        prevGhostPos = ghostPos;
    }
}
*/

void Apps::LunaTesting::update()
{
    // ComponentModularity::synchronizeChildren(appRoot);


    // NavPath::updateGhost(globals.simulationTime.getDelta(), 0.0f, 0.05f);

    // GG::draw->drawBoxFromHalfExtents(ghostPos + vec3(0, 1.0, 0), vec3(0.5, 1.0, 0.5), 0.0f, ModelState3D(), "#00ff00"_rgb);
    
    // if (NavPath::path.size() > 0)
    // {
    //     for (int i = 0; i < NavPath::path.size() - 1; i++)
    //     GG::draw->drawLine(NavPath::path[i] + vec3(0, 1, 0), NavPath::path[i + 1] + vec3(0, 1, 0), 0.0f, ModelState3D(), "#0000ff"_rgb);
    // }

    // float t = globals.appTime.getElapsedTime() * glm::two_pi<float>() * 0.05f;
    // myAwesomeEntity->comp<state3D>().position.x = -700 + sin(t) * 2.0f;
    // myAwesomeEntity->comp<state3D>().lookDirection = vec3(sin(t), 0, cos(t));
    
    // if (globals.appTime.getUpdateCounter() % 60)
    // {
        // AudioPlayer& p = myAwesomeEntity->comp<AudioPlayer>();

        // p.Play({.clipName = "FootstepsStoneDirt1", .loop = true});
    // }   

    // bool b = InputManager::getGamepadButtonValue(VULPINE_GAMEPAD_BUTTON_A);
    // std::cout << "button A is: " << (b ? "Pressed" : "Not Pressed") << std::endl;

    TestNavMeshUpdate();

    static std::vector<DrawBoxes::Box> boxes(N_OBJECT);
    for (int i = 0; i < N_OBJECT; i++)
    {
        const TestObject& testObject = testArray[i];
        vec2 translation = testObject.velocity * globals.simulationTime.getDelta();
        vec3& position = testObject.entity->comp<State3D>().position;
        position.x += translation.x;
        position.z += translation.y;

        boxes[i] = {position + vec3(-2) + vec3(0, 256, 0), position + vec3(2) + vec3(0, 256, 0)};
    }

    GG::draw->drawBoxes(boxes, 0.0f, ModelState3D(), "#ff00ff"_rgb);

    if (globals.getController() == &playerControl && GG::playerEntity)
        prevPlayerPosition = GG::playerEntity->comp<State3D>().position;
}


void Apps::LunaTesting::clean()
{
    globals.simulationTime.pause();

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));

    for (int i = 0; i < testArray.size(); i++)
    {
        testArray[i] = TestObject();
    }

    appRoot = EntityRef();
    GameGlobals::playerEntity = EntityRef();

    testParent = EntityRef();
    
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);
}

