//
// Created by Plutex on 2026-03-07.
//

#include "PluEngine/Physics/PhysicsWorld.h"
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyManager.h>
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <glad/glad.h>

#include "EngineAssets.h"
#include "PluEngine/Log.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "PluEngine/PluUtils.h"
#include "PluEngine/BasicEngineClasses/Components/PhysicsBodyComponent.h"
#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Managers/ShadersManager.h"
#include "PluEngine/Physics/PhysicsCompoundShape.h"
#include "PluEngine/Physics/PluPhysicsMaterial.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "PluEngine/Shaders/ShaderProgram.h"
#include "PluEngine/Timer.h"

#include <algorithm>
#include <cmath>

using namespace Plu;

PhysicsWorld::PhysicsWorld() {
	mAllocator = CreateOwning<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
	mJobSystem = CreateOwning<JPH::JobSystemThreadPool>(
		JPH::cMaxPhysicsJobs,
		JPH::cMaxPhysicsBarriers,
		std::thread::hardware_concurrency() - 1
	);

	mBPLayerInterface = CreateOwning<BPLayerInterfaceImpl>();
	mObjVsBPFilter    = CreateOwning<ObjectVsBroadPhaseLayerFilterImpl>();
	mObjVsObjFilter   = CreateOwning<ObjectLayerPairFilterImpl>();

	mPhysicsSystem = CreateOwning<JPH::PhysicsSystem>();
	mPhysicsSystem->Init(
		1024, 0, 1024, 1024,
		*mBPLayerInterface,
		*mObjVsBPFilter,
		*mObjVsObjFilter
	);

	mContactListener = CreateOwning<OverlapContactListener>(this);
	mPhysicsSystem->SetContactListener(mContactListener.GetRaw());

	// UE-style collision channels are read live from ActiveCollisionConfig() (the project's
	// config). Filtering itself lives in the contact listener (OnContactValidate / OnContactAdded).

	PLU_CORE_INFO("Physics World Created!");
}

PhysicsWorld::~PhysicsWorld()
{
	mObjectsNeedShape.Clear();
}

void PhysicsWorld::Update(float DeltaTime) {
	mPhysicsSystem->Update(
		DeltaTime,
		cCollisionSteps,
		mAllocator.GetRaw(),
		mJobSystem.GetRaw()
	);

	{
		DynamicArray<PendingOverlapEvent> toProcess;
		{
			std::lock_guard<std::mutex> lock(mOverlapMutex);
			toProcess = std::move(mPendingOverlapEvents);
		}

		for (const auto& ev : toProcess)
		{
			UInt64* uuidA = mBodiesPerObject.Find(ev.BodyIdA);
			UInt64* uuidB = mBodiesPerObject.Find(ev.BodyIdB);
			if (!uuidA || !uuidB) continue;

			TUsePointer<GameObject> objA = mSceneWorld->GetGameObjectByUUID(*uuidA);
			TUsePointer<GameObject> objB = mSceneWorld->GetGameObjectByUUID(*uuidB);
			if (!objA || !objB) continue;

			auto resolveComp = [&](const TUsePointer<GameObject>& obj, JPH::SubShapeID subShapeID) -> Plu::GameObjectComponent*
			{
				auto* comps = obj->GetObjectWorldComponents();

				// Jolt collapses single-child StaticCompoundShape to the raw child shape.
				// In that case there is exactly one physics component — return it directly.
				if (obj->mCompoundShape)
				{
					const JPH::Shape* rawShape = obj->mCompoundShape->GetCompoundShape().GetPtr();
					if (rawShape && rawShape->GetType() == JPH::EShapeType::Compound)
					{
						const auto* compound = static_cast<const JPH::CompoundShape*>(rawShape);
						JPH::SubShapeID remainder;
						UInt32 childIdx = compound->GetSubShapeIndexFromID(subShapeID, remainder);
						if (childIdx < compound->GetNumSubShapes())
						{
							UInt32 physIdx = 0;
							for (const auto& c : *comps)
							{
								if (c->GetClass()->IsDerivedOf(PhysicsBodyComponent::GetStaticClass()))
								{
									if (physIdx == childIdx) return c.GetRaw();
									physIdx++;
								}
							}
						}
					}
				}

				// Fallback: single-body object or SubShapeID decode failed → first physics component
				for (const auto& c : *comps)
					if (c->GetClass()->IsDerivedOf(PhysicsBodyComponent::GetStaticClass()))
						return c.GetRaw();
				return nullptr;
			};

			// SubShapeA = pod-kształt objA, SubShapeB = pod-kształt objB
			// Dla objA przekazujemy komponent objA który był trafiony (SubShapeA)
			// Dla objB przekazujemy komponent objB który był trafiony (SubShapeB)
			Plu::GameObjectComponent* compA = resolveComp(objA, ev.SubShapeA);
			Plu::GameObjectComponent* compB = resolveComp(objB, ev.SubShapeB);
			if (ev.IsBegin)
			{
				objA->OnOverlapBegin(compB);
				objB->OnOverlapBegin(compA);
			}
			else
			{
				objA->OnOverlapEnd(compB);
				objB->OnOverlapEnd(compA);
			}
		}
	}

	for (const auto& entry : mBodiesPerObject) {
		JPH::BodyID bodyID(entry.first);
		if (GetBodyInterface().GetMotionType(bodyID) == JPH::EMotionType::Static) continue;

		// GetPosition() returns shape-local-origin (not COM) — that's exactly the game object's world position
		JPH::RVec3 objPos = GetBodyInterface().GetPosition(bodyID);
		JPH::Quat  jphRot = GetBodyInterface().GetRotation(bodyID);

		glm::quat glmRot(jphRot.GetW(), jphRot.GetX(), jphRot.GetY(), jphRot.GetZ());
		Vec3 eulerDeg = glm::degrees(glm::eulerAngles(glmRot));

		TUsePointer<GameObject> obj = mSceneWorld->GetGameObjectByUUID(entry.second);
		if (obj) {
			obj->SyncFromPhysicsBody(ToGLM(objPos), eulerDeg);
		}
	}
}

void PhysicsWorld::Init(TUsePointer<SceneWorld> sceneWorld, TUsePointer<EngineObjectManager> engineObjectManager)
{
	mSceneWorld = sceneWorld;
	mEngineObjectManager = engineObjectManager;
}

void PhysicsWorld::NewPhysicsComponent(TUsePointer<PhysicsBodyComponent> component)
{
	mObjectsNeedShape.Insert(component->GetParentGameObject()->GetObjectUUID(), component->GetParentGameObject());
}

void PhysicsWorld::FlushPendingBodies()
{
	if (mObjectsNeedShape.IsEmpty()) return;
	PLU_PROFILE_SCOPE("Physics Flush Pending Bodies");
	for (const auto& object : mObjectsNeedShape) {
		if (!object.second) continue; // destroyed before we got to build its body
		RebuildGameObjectBody(object.second.GetRaw());
	}
	mObjectsNeedShape.Clear();
}

void PhysicsWorld::MarkGameObjectShapeDirty(GameObject* gameObject)
{
	if (!gameObject) return;
	// Only objects that already have a body — otherwise we would create one outside of play.
	if (!mEngineObjectManager->IsValid(gameObject->mPhysicsBodyHandle)) return;
	mObjectsNeedShape.Insert(gameObject->GetObjectUUID(), mEngineObjectManager->GetObjectAsUser<GameObject>(*gameObject->GetEngineObjectHandle()));
}

void PhysicsWorld::RebuildGameObjectBody(GameObject* gameObject)
{
	PLU_PROFILE_SCOPE("Physics Rebuild Body");
	if (!gameObject) return;

	// Drop the previous compound shape (if any).
	if (gameObject->mCompoundShape) {
		mEngineObjectManager->DestroyObject(*gameObject->mCompoundShape->GetEngineObjectHandle());
		gameObject->mCompoundShape = nullptr;
	}

	// Collect every component that contributes collision geometry.
	const auto* components = gameObject->GetObjectWorldComponents();
	DynamicArray<TUsePointer<PhysicsBodyComponent>> physicsBodiesComponents;
	DynamicArray<TUsePointer<StaticMeshComponent>> meshComponents;
	for (const auto& comp : *components) {
		if (comp->GetClass()->IsDerivedOf(PhysicsBodyComponent::GetStaticClass())) {
			physicsBodiesComponents.PushBack(comp);
		} else if (comp->GetClass()->IsDerivedOfOrSame(StaticMeshComponent::GetStaticClass())) {
			auto* smc = static_cast<StaticMeshComponent*>(comp.GetRaw());
			if (smc->StaticMeshToDisplay && !smc->StaticMeshToDisplay->CollisionShapes.IsEmpty())
				meshComponents.PushBack(comp);
		}
	}

	// Build the compound shape using the object's *current* scale.
	TUsePointer<PhysicsCompoundShape> compoundShape = mEngineObjectManager->CreateObject(PhysicsCompoundShape::GetStaticClass());
	compoundShape->Init(physicsBodiesComponents, meshComponents, gameObject->GetObjectScale());
	gameObject->mCompoundShape = mEngineObjectManager->GetObjectAsOwner<PhysicsCompoundShape>(compoundShape->GetObjectHandle());

	// Drop the previous body (if any) and unregister it. Its motion is carried over to the new
	// body: a rebuild is triggered by collider edits (scale, friction, a component transform), and
	// those must not bring a moving object to a dead stop.
	JPH::Vec3 previousLinearVelocity  = JPH::Vec3::sZero();
	JPH::Vec3 previousAngularVelocity = JPH::Vec3::sZero();
	bool hasPreviousVelocity = false;
	if (mEngineObjectManager->IsValid(gameObject->mPhysicsBodyHandle)) {
		if (TUsePointer<PhysicsBody> oldBody = mEngineObjectManager->GetObjectAsUser<PhysicsBody>(gameObject->mPhysicsBodyHandle)) {
			previousLinearVelocity  = oldBody->GetLinearVelocity();
			previousAngularVelocity = oldBody->GetAngularVelocity();
			hasPreviousVelocity = true;
			mBodiesPerObject.Remove(oldBody->GetID().GetIndexAndSequenceNumber());
		}
		mEngineObjectManager->DestroyObject(gameObject->mPhysicsBodyHandle);
	}

	JPH::ShapeRefC shape = gameObject->mCompoundShape->GetCompoundShape();
	if (!shape) return;

	Vec3 rotDeg = gameObject->GetObjectRotation();
	JPH::Quat bodyRot = JPH::Quat::sEulerAngles(JPH::Vec3(
		JPH::DegreesToRadians(rotDeg.x),
		JPH::DegreesToRadians(rotDeg.y),
		JPH::DegreesToRadians(rotDeg.z)
	));
	JPH::RVec3 bodyPos = ToJPH(gameObject->GetObjectLocation());

	// Motion type is a whole-body property (a JPH::Body has one), so it comes from the game object.
	const bool isDynamic = gameObject->ActiveBody;

	// Friction/restitution/channel are now per-sub-shape, carried by each leaf shape's
	// PluPhysicsMaterial and combined per-contact in the contact listener. The body-level values
	// here are only a FALLBACK for sub-shapes that have no material (e.g. mesh collision).
	const float friction = 0.2f;
	const float restitution = 0.0f;
	// Mesh-only objects (no PhysicsBodyComponent) default to the static-world preset.
	const String collisionProfile = physicsBodiesComponents.IsEmpty() ? String("WorldStatic") : String("Default");
	const UInt32 collisionProfileIndex = ResolveCollisionProfileIndex(collisionProfile);

	gameObject->mPhysicsBodyHandle = mEngineObjectManager->CreateObject<PhysicsBody>(
		GetBodyInterface(),
		shape,
		bodyPos,
		bodyRot,
		isDynamic ? BodyType::Dynamic : BodyType::Static,
		friction,
		restitution,
		collisionProfileIndex
	);
	TUsePointer<PhysicsBody> newBody = mEngineObjectManager->GetObjectAsUser<PhysicsBody>(gameObject->mPhysicsBodyHandle);
	if (hasPreviousVelocity && isDynamic) {
		newBody->SetLinearVelocity(previousLinearVelocity);
		newBody->SetAngularVelocity(previousAngularVelocity);
	}
	mBodiesPerObject.Insert(newBody->GetID().GetIndexAndSequenceNumber(), gameObject->GetObjectUUID());
}

void PhysicsWorld::Play()
{
	// Add objects that only have StaticMeshComponent collision (no PhysicsBodyComponents)
	auto allGameObjects = mSceneWorld->GetAllGameObjects();
	for (const auto& obj : allGameObjects)
	{
		if (mObjectsNeedShape.Contains(obj->GetObjectUUID())) continue;
		const auto* comps = obj->GetObjectWorldComponents();
		for (const auto& comp : *comps)
		{
			if (comp->GetClass()->IsDerivedOfOrSame(StaticMeshComponent::GetStaticClass()))
			{
				auto* smc = static_cast<StaticMeshComponent*>(comp.GetRaw());
				if (smc->StaticMeshToDisplay && !smc->StaticMeshToDisplay->CollisionShapes.IsEmpty())
				{
					mObjectsNeedShape.Insert(obj->GetObjectUUID(), obj);
					break;
				}
			}
		}
	}

	FlushPendingBodies();
}

void PhysicsWorld::RemoveGameObjectBodies(GameObject* gameObject)
{
	mObjectsNeedShape.Remove(gameObject->GetObjectUUID());
	if (!mEngineObjectManager->IsValid(gameObject->mPhysicsBodyHandle)) return;
	TUsePointer<PhysicsBody> physBody = mEngineObjectManager->GetObjectAsUser<PhysicsBody>(gameObject->mPhysicsBodyHandle);
	if (!physBody) return;
	mBodiesPerObject.Remove(physBody->GetID().GetIndexAndSequenceNumber());
}

void PhysicsWorld::Shutdown()
{
	mObjectsNeedShape.Clear();
	mSceneWorld = nullptr;
}

void PhysicsWorld::CollectDebugRaycasts(float deltaTime, DynamicArray<float>& outLineVerts)
{
	// MAIN-only: dekrementuje timery i pakuje segmenty (pos(3)+color(3) interleaved) do
	// wspólnego bufora linii snapshotu. GL (upload+draw) robi wątek renderu w Rendererze.
	if (mRaycastsToDraw.IsEmpty()) return;

	DynamicArray<int> indiciesToRemove;
	for (int i = 0; i < mRaycastsToDraw.Size(); i++)
	{
		if (mRaycastsToDraw[i].first < 0.0f) {
			indiciesToRemove.PushBack(i);
		}
		mRaycastsToDraw[i].first -= deltaTime;
		const Line& l = mRaycastsToDraw[i].second;
		if (mRaycastsToDraw[i].second.hit) {
			outLineVerts.PushBack(l.A.x); outLineVerts.PushBack(l.A.y); outLineVerts.PushBack(l.A.z);
			outLineVerts.PushBack(1); outLineVerts.PushBack(0); outLineVerts.PushBack(0);
			outLineVerts.PushBack(l.B.x); outLineVerts.PushBack(l.B.y); outLineVerts.PushBack(l.B.z);
			outLineVerts.PushBack(1); outLineVerts.PushBack(0); outLineVerts.PushBack(0);

			outLineVerts.PushBack(l.B.x); outLineVerts.PushBack(l.B.y); outLineVerts.PushBack(l.B.z);
			outLineVerts.PushBack(0); outLineVerts.PushBack(1); outLineVerts.PushBack(0);
			outLineVerts.PushBack(l.AfterHit.x); outLineVerts.PushBack(l.AfterHit.y); outLineVerts.PushBack(l.AfterHit.z);
			outLineVerts.PushBack(0); outLineVerts.PushBack(1); outLineVerts.PushBack(0);
		} else {
			outLineVerts.PushBack(l.A.x); outLineVerts.PushBack(l.A.y); outLineVerts.PushBack(l.A.z);
			outLineVerts.PushBack(1); outLineVerts.PushBack(0); outLineVerts.PushBack(0);
			outLineVerts.PushBack(l.B.x); outLineVerts.PushBack(l.B.y); outLineVerts.PushBack(l.B.z);
			outLineVerts.PushBack(1); outLineVerts.PushBack(0); outLineVerts.PushBack(0);
		}
	}

	for (auto idx : indiciesToRemove) {
		mRaycastsToDraw.RemoveAt(idx);
	}
}

RaycastHit PhysicsWorld::Raycast(const Vec3 &Origin, const Vec3 &Direction, float MaxDistance,
                                 RaycastDebugSettings DebugDrawSettings,
                                 const DynamicArray<GameObject*>& IgnoredObjects)
{
	RaycastHit HitResult;

	JPH::RRayCast    Ray    { ToJPH(Origin), ToJPH(Direction) * MaxDistance };
	JPH::RayCastResult Result;

	JPH::IgnoreMultipleBodiesFilter IgnoreFilter;
	IgnoreFilter.Reserve(static_cast<UInt32>(IgnoredObjects.Size()));
	for (GameObject* Ignored : IgnoredObjects) {
		if (!Ignored) continue;
		if (TUsePointer<PhysicsBody> Body = Ignored->GetPhysicsBody())
			IgnoreFilter.IgnoreBody(Body->GetID());
	}

	HitResult.Hit = mPhysicsSystem->GetNarrowPhaseQuery().CastRay(Ray, Result, {}, {}, IgnoreFilter);
	HitResult.Fraction = 0;
	HitResult.HitLocation = {0,0,0};

	if (HitResult.Hit) {
		HitResult.HitLocation = ToGLM(Ray.GetPointOnRay(Result.mFraction));
		HitResult.Fraction = Result.mFraction;
		HitResult.PhysicsBodyHit = Result.mBodyID;
		HitResult.HitObject = mSceneWorld->GetGameObjectByUUID(mBodiesPerObject[Result.mBodyID.GetIndexAndSequenceNumber()]).GetRaw();
	}

	if (DebugDrawSettings.DrawDebug) {
		Vec3 end = Direction * MaxDistance;
		if (HitResult.Hit) {
			mRaycastsToDraw.PushBack({DebugDrawSettings.DrawTime, {Origin, HitResult.HitLocation, true, end}});
		} else {
			mRaycastsToDraw.PushBack({DebugDrawSettings.DrawTime, {Origin, end}});
		}
	}
	return HitResult;
}

// Reads our per-sub-shape material out of the leaf shape's user data. Returns false when the
// sub-shape has none (e.g. mesh collision, whose shapes carry the default user data 0).
static bool GetSubShapeMaterial(const JPH::Body& body, JPH::SubShapeID subShapeID, PhysicsMaterialData& out)
{
	const JPH::Shape* shape = body.GetShape();
	if (!shape) return false;
	return TryUnpackPhysicsMaterial(shape->GetSubShapeUserData(subShapeID), out);
}

UInt32 OverlapContactListener::ResolveSubShapeProfile(const JPH::Body& body, JPH::SubShapeID subShapeID) const
{
	PhysicsMaterialData mat;
	if (GetSubShapeMaterial(body, subShapeID, mat))
		return mat.CollisionProfileIndex;
	return body.GetCollisionGroup().GetGroupID(); // fallback for material-less sub-shapes (mesh)
}

void OverlapContactListener::ApplyCombinedMaterial(const JPH::Body& inBody1, const JPH::Body& inBody2,
                                                   JPH::SubShapeID sub1, JPH::SubShapeID sub2,
                                                   JPH::ContactSettings& ioSettings) const
{
	PhysicsMaterialData matA, matB;
	const bool hasA = GetSubShapeMaterial(inBody1, sub1, matA);
	const bool hasB = GetSubShapeMaterial(inBody2, sub2, matB);

	const float fA = hasA ? matA.Friction    : inBody1.GetFriction();
	const float fB = hasB ? matB.Friction    : inBody2.GetFriction();
	const float rA = hasA ? matA.Restitution : inBody1.GetRestitution();
	const float rB = hasB ? matB.Restitution : inBody2.GetRestitution();

	// Jolt's default combine rules: geometric mean for friction, max for restitution.
	ioSettings.mCombinedFriction    = std::sqrt(fA * fB);
	ioSettings.mCombinedRestitution = std::max(rA, rB);
}

JPH::ValidateResult OverlapContactListener::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2,
                                                              JPH::RVec3Arg inBaseOffset,
                                                              const JPH::CollideShapeResult& inCollisionResult)
{
	// UE-style channel filter: Ignore pairs produce no contact at all. Channels are resolved
	// per sub-shape (CollideShapeResult carries the hit sub-shapes).
	const CollisionResponse resp = ResolvePairResponse(
		ActiveCollisionConfig(),
		ResolveSubShapeProfile(inBody1, inCollisionResult.mSubShapeID1),
		ResolveSubShapeProfile(inBody2, inCollisionResult.mSubShapeID2));

	return resp == CollisionResponse::Ignore
		? JPH::ValidateResult::RejectAllContactsForThisBodyPair
		: JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

bool OverlapContactListener::ResolveOverlap(const JPH::Body& inBody1, const JPH::Body& inBody2,
                                            JPH::SubShapeID sub1, JPH::SubShapeID sub2,
                                            JPH::ContactSettings& ioSettings) const
{
	const CollisionResponse resp = ResolvePairResponse(
		ActiveCollisionConfig(),
		ResolveSubShapeProfile(inBody1, sub1),
		ResolveSubShapeProfile(inBody2, sub2));

	if (resp == CollisionResponse::Overlap)
	{
		// Per-pair overlap: suppress the physical response but keep the contact for events.
		ioSettings.mIsSensor = true;
		return true;
	}
	return false; // Block (Ignore pairs never reach the contact listener).
}

void OverlapContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2,
                                            const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
	// Per-sub-shape friction/restitution (applies to block contacts; harmless for sensors).
	ApplyCombinedMaterial(inBody1, inBody2, inManifold.mSubShapeID1, inManifold.mSubShapeID2, ioSettings);

	if (!ResolveOverlap(inBody1, inBody2, inManifold.mSubShapeID1, inManifold.mSubShapeID2, ioSettings)) return;

	// Jolt gwarantuje: body1.ID < body2.ID
	UInt32 idA = inBody1.GetID().GetIndexAndSequenceNumber();
	UInt32 idB = inBody2.GetID().GetIndexAndSequenceNumber();
	UInt64 key = (static_cast<UInt64>(idA) << 32) | static_cast<UInt64>(idB);

	std::lock_guard<std::mutex> lock(mWorld->mOverlapMutex);
	if (!mWorld->mActiveSensorPairs.Contains(key))
	{
		mWorld->mActiveSensorPairs.Insert(key, { inManifold.mSubShapeID1, inManifold.mSubShapeID2 });
		mWorld->mPendingOverlapEvents.PushBack({idA, idB, inManifold.mSubShapeID1, inManifold.mSubShapeID2, true});
	}
}

void OverlapContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2,
                                                const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
	// ContactSettings are per-call: friction/restitution and a per-pair Overlap's mIsSensor must be
	// re-asserted every step. The begin event was already queued in OnContactAdded; nothing to queue here.
	ApplyCombinedMaterial(inBody1, inBody2, inManifold.mSubShapeID1, inManifold.mSubShapeID2, ioSettings);
	ResolveOverlap(inBody1, inBody2, inManifold.mSubShapeID1, inManifold.mSubShapeID2, ioSettings);
}

void OverlapContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
{
	UInt32 idA = inSubShapePair.GetBody1ID().GetIndexAndSequenceNumber();
	UInt32 idB = inSubShapePair.GetBody2ID().GetIndexAndSequenceNumber();
	UInt64 key = (static_cast<UInt64>(idA) << 32) | static_cast<UInt64>(idB);

	std::lock_guard<std::mutex> lock(mWorld->mOverlapMutex);
	auto* stored = mWorld->mActiveSensorPairs.Find(key);
	if (stored)
	{
		mWorld->mPendingOverlapEvents.PushBack({idA, idB, stored->first, stored->second, false});
		mWorld->mActiveSensorPairs.Remove(key);
	}
}

#ifdef PLU_ENGINE_EDITOR_BUILD
void PhysicsWorld::DrawEditModeShapes(JoltWireframeRenderer* wireframe, JoltPointRenderer* points,
                                      Vec3 wireColor, Vec3 pointColor)
{
	for (const auto& entry : mObjectsNeedShape)
	{
		TUsePointer<GameObject> gameObject = entry.second;
		if (!gameObject) continue;

		Matrix4 objectWorldMat = gameObject->GetObjectWorldMatrix();

		const auto* components = gameObject->GetObjectWorldComponents();
		if (!components) continue;

		for (const auto& comp : *components)
		{
			if (!comp->GetClass()->IsDerivedOf(PhysicsBodyComponent::GetStaticClass())) continue;

			auto* physComp = static_cast<PhysicsBodyComponent*>(comp.GetRaw());
			JPH::ShapeRefC shape = physComp->GetShape();
			if (!shape) continue;

			Vec3 relLoc = physComp->GetRelativeLocation();
			Vec3 relRotDeg = physComp->GetRelativeRotation();
			glm::mat4 relMat = glm::translate(glm::mat4(1.0f), relLoc) *
			                   glm::mat4_cast(glm::quat(glm::radians(relRotDeg)));
			glm::mat4 worldMat = objectWorldMat * relMat;

			if (wireframe) wireframe->AddShape(shape, worldMat, wireColor);
			if (points)    points->AddShape(shape, worldMat, pointColor);
		}
	}

	// Uwaga: nie wymagamy mSceneWorld->Info — scena overlay edytora (np. StaticMeshViewport)
	// ma Info == nullptr, a i tak chcemy z niej ekstrahować kształty kolizji StaticMeshComponentów.
	if (!mSceneWorld || (!wireframe && !points)) return;

	DynamicArray<TUsePointer<GameObject>> allObjects = mSceneWorld->GetAllGameObjects();
	for (const auto& gameObject : allObjects)
	{
		if (!gameObject) continue;
		const auto* components = gameObject->GetObjectWorldComponents();
		if (!components) continue;

		for (const auto& comp : *components)
		{
			if (!comp->GetClass()->IsDerivedOfOrSame(StaticMeshComponent::GetStaticClass())) continue;

			auto* smc = static_cast<StaticMeshComponent*>(comp.GetRaw());
			StaticMesh* mesh = smc->StaticMeshToDisplay.GetRaw();
			if (!mesh || !mesh->IsLoaded || mesh->CollisionShapes.IsEmpty()) continue;

			DynamicArray<MeshCollisionShapeEntry>* cached = mEditModeCollisionCache.Find(mesh);
			if (!cached)
			{
				DynamicArray<MeshCollisionShapeEntry> built = BuildCollisionShapesForMesh(mesh);
				mEditModeCollisionCache.Insert(mesh, std::move(built));
				cached = mEditModeCollisionCache.Find(mesh);
			}
			if (!cached) continue;

			glm::mat4 componentWorldMat = smc->GetWorldMatrix();
			for (const auto& shapeEntry : *cached)
			{
				if (!shapeEntry.Shape) continue;
				// Triangles come out in shape-local (COM-centred) space, so we replicate what
				// Jolt does for a compound sub-shape: LocalOffset + shape's own COM.
				Vec3 drawOffset = shapeEntry.LocalOffset + Plu::ToGLMFromVec3(shapeEntry.Shape->GetCenterOfMass());
				glm::mat4 shapeMat = componentWorldMat *
				                     glm::translate(glm::mat4(1.0f), drawOffset);
				if (wireframe) wireframe->AddShape(shapeEntry.Shape, shapeMat, wireColor);
				if (points)    points->AddShape(shapeEntry.Shape, shapeMat, pointColor);
			}
		}
	}
}

void PhysicsWorld::InvalidateMeshCollisionCache(StaticMesh* mesh)
{
	if (mesh)
		mEditModeCollisionCache.Remove(mesh);
	else
		mEditModeCollisionCache.Clear();
}
#endif
