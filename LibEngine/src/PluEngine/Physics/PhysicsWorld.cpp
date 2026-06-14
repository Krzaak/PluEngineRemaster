//
// Created by Plutex on 2026-03-07.
//

#include "PluEngine/Physics/PhysicsWorld.h"
#include <Jolt/Physics/Body/BodyManager.h>
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <glad/glad.h>

#include "EngineAssets.h"
#include "PluEngine/Log.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "PluEngine/PluUtils.h"
#include "PluEngine/BasicEngineClasses/Components/PhysicsBodyComponent.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Managers/ShadersManager.h"
#include "PluEngine/Physics/PhysicsCompoundShape.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "PluEngine/Shaders/ShaderProgram.h"

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

	Init();

	PLU_CORE_INFO("Physics World Created!");
}

PhysicsWorld::~PhysicsWorld()
{
	Cleanup();
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

void PhysicsWorld::NewPhysicsComponent(TUsePointer<PhysicsBodyComponent> component, bool isPlaying)
{
	if (isPlaying) {
		PLU_CORE_TRACE("ReGenerating Compound Shape for {}", component->GetParentGameObject()->GetDisplayName().CStr());
		TOwningPointer<PhysicsCompoundShape> compoundShape = component->GetParentGameObject()->mCompoundShape;
		if (compoundShape) {
			mEngineObjectManager->DestroyObject(*compoundShape->GetEngineObjectHandle());
		}
		component->GetParentGameObject()->mCompoundShape = nullptr;
		TUsePointer<PhysicsCompoundShape> shapeUser = mEngineObjectManager->CreateObject(PhysicsCompoundShape::GetStaticClass());
		compoundShape = mEngineObjectManager->GetObjectAsOwner<PhysicsCompoundShape>(shapeUser->GetObjectHandle());
		const auto components = component->GetParentGameObject()->GetObjectWorldComponents();
		DynamicArray<TUsePointer<PhysicsBodyComponent>> physicsBodiesComponents;
		for (const auto& component : *components) {
			if (component->GetClass()->IsDerivedOf(PhysicsBodyComponent::GetStaticClass())) {
				physicsBodiesComponents.PushBack(component);
			}
		}
		compoundShape->Init(physicsBodiesComponents, component->GetParentGameObject()->GetObjectScale());
		component->GetParentGameObject()->mCompoundShape = compoundShape;
		mBodiesPerObject.Remove(mEngineObjectManager->GetObjectAsUser<PhysicsBody>(component->GetParentGameObject()->mPhysicsBodyHandle)->GetID().GetIndexAndSequenceNumber());
		mEngineObjectManager->DestroyObject(component->GetParentGameObject()->mPhysicsBodyHandle);
		JPH::ShapeRefC newShape = compoundShape->GetCompoundShape();
		Vec3 newRotDeg = component->GetParentGameObject()->GetObjectRotation();
		JPH::Quat newBodyRot = JPH::Quat::sEulerAngles(JPH::Vec3(
			JPH::DegreesToRadians(newRotDeg.x),
			JPH::DegreesToRadians(newRotDeg.y),
			JPH::DegreesToRadians(newRotDeg.z)
		));
		JPH::RVec3 newBodyPos = ToJPH(component->GetParentGameObject()->GetObjectLocation());
		const auto* newComponents = component->GetParentGameObject()->GetObjectWorldComponents();
		bool newIsDynamic = false;
		PhysicsBodyMode newMode = PhysicsBodyMode::Solid;
		for (const auto& comp : *newComponents)
		{
			if (!comp->GetClass()->IsDerivedOf(PhysicsBodyComponent::GetStaticClass())) continue;
			auto* physComp = static_cast<PhysicsBodyComponent*>(comp.GetRaw());
			if (physComp->ActiveBody) newIsDynamic = true;
			if (physComp->BodyMode == PhysicsBodyMode::Trigger) newMode = PhysicsBodyMode::Trigger;
		}
		component->GetParentGameObject()->mPhysicsBodyHandle = mEngineObjectManager->CreateObject<PhysicsBody>(
			GetBodyInterface(),
			newShape,
			newBodyPos,
			newBodyRot,
			newIsDynamic ? BodyType::Dynamic : BodyType::Static,
			newMode
		);
		mBodiesPerObject.Insert(mEngineObjectManager->GetObjectAsUser<PhysicsBody>(component->GetParentGameObject()->mPhysicsBodyHandle)->GetID().GetIndexAndSequenceNumber(), component->GetParentGameObject()->GetObjectUUID());
	} else {
		mObjectsNeedShape.Insert(component->GetParentGameObject()->GetObjectUUID(), component->GetParentGameObject());
	}
}

void PhysicsWorld::Play()
{
	for (const auto& object : mObjectsNeedShape) {
		PLU_CORE_TRACE("Generating Compound Shape for {}", object.second->GetDisplayName().CStr());
		const auto components = object.second->GetObjectWorldComponents();
		DynamicArray<TUsePointer<PhysicsBodyComponent>> physicsBodiesComponents;
		for (const auto& component : *components) {
			if (component->GetClass()->IsDerivedOf(PhysicsBodyComponent::GetStaticClass())) {
				physicsBodiesComponents.PushBack(component);
			}
		}
		const TUsePointer<PhysicsCompoundShape> compoundShape = mEngineObjectManager->CreateObject(PhysicsCompoundShape::GetStaticClass());
		compoundShape->Init(physicsBodiesComponents, object.second->GetObjectScale());
		object.second->mCompoundShape = mEngineObjectManager->GetObjectAsOwner<PhysicsCompoundShape>(compoundShape->GetObjectHandle());
		mEngineObjectManager->DestroyObject(object.second->mPhysicsBodyHandle);
		JPH::ShapeRefC shape = compoundShape->GetCompoundShape();
		Vec3 rotDeg = object.second->GetObjectRotation();
		JPH::Quat bodyRot = JPH::Quat::sEulerAngles(JPH::Vec3(
			JPH::DegreesToRadians(rotDeg.x),
			JPH::DegreesToRadians(rotDeg.y),
			JPH::DegreesToRadians(rotDeg.z)
		));
		JPH::RVec3 bodyPos = ToJPH(object.second->GetObjectLocation());
		bool isDynamic = false;
		PhysicsBodyMode mode = PhysicsBodyMode::Solid;
		for (const auto& comp : physicsBodiesComponents)
		{
			if (comp->ActiveBody) isDynamic = true;
			if (comp->BodyMode == PhysicsBodyMode::Trigger) mode = PhysicsBodyMode::Trigger;
		}
		object.second->mPhysicsBodyHandle = mEngineObjectManager->CreateObject<PhysicsBody>(
			GetBodyInterface(),
			shape,
			bodyPos,
			bodyRot,
			isDynamic ? BodyType::Dynamic : BodyType::Static,
			mode
		);
		mBodiesPerObject.Insert(mEngineObjectManager->GetObjectAsUser<PhysicsBody>(object.second->mPhysicsBodyHandle)->GetID().GetIndexAndSequenceNumber(), object.second->GetObjectUUID());
	}
	mObjectsNeedShape.Clear();
}

void PhysicsWorld::Shutdown()
{
	mObjectsNeedShape.Clear();
	mSceneWorld = nullptr;
}

void PhysicsWorld::DrawDebugRaycasts(float deltaTime, Matrix4 viewProj, const TUsePointer<IShaderManager> &shaderManager)
{
	if (!mShaderManager) mShaderManager = shaderManager;
	if (mRaycastsToDraw.IsEmpty()) return;

	// Spakuj do flat bufora: pos(3) + color(3) na wierzchołek
	DynamicArray<float> buf;
	buf.Reserve(mRaycastsToDraw.Size() * 2 * 6);

	DynamicArray<int> indiciesToRemove;
	int raycastsToDraw = 0;
	for (int i = 0; i < mRaycastsToDraw.Size(); i++)
	{
		if (mRaycastsToDraw[i].first < 0.0f) {
			indiciesToRemove.PushBack(i);
		}
		mRaycastsToDraw[i].first -= deltaTime;
		const Line& l = mRaycastsToDraw[i].second;
		if (mRaycastsToDraw[i].second.hit) {
			buf.PushBack(l.A.x); buf.PushBack(l.A.y); buf.PushBack(l.A.z);
			buf.PushBack(1); buf.PushBack(0); buf.PushBack(0);
			buf.PushBack(l.B.x); buf.PushBack(l.B.y); buf.PushBack(l.B.z);
			buf.PushBack(1); buf.PushBack(0); buf.PushBack(0);

			buf.PushBack(l.B.x); buf.PushBack(l.B.y); buf.PushBack(l.B.z);
			buf.PushBack(0); buf.PushBack(1); buf.PushBack(0);
			buf.PushBack(l.AfterHit.x); buf.PushBack(l.AfterHit.y); buf.PushBack(l.AfterHit.z);
			buf.PushBack(0); buf.PushBack(1); buf.PushBack(0);
			raycastsToDraw += 2;
		} else {
			buf.PushBack(l.A.x); buf.PushBack(l.A.y); buf.PushBack(l.A.z);
			buf.PushBack(1); buf.PushBack(0); buf.PushBack(0);
			buf.PushBack(l.B.x); buf.PushBack(l.B.y); buf.PushBack(l.B.z);
			buf.PushBack(1); buf.PushBack(0); buf.PushBack(0);
			raycastsToDraw++;
		}
	}

	if (buf.IsEmpty()) return;

	if (!mShader) {
		mShader = mShaderManager->GetShaderProgram(EngineAssets::DebugLine);
	}

	if (!mShader->IsLoaded()) {
		mShaderManager->LoadShader(mShader->Uuid);
	}

	mShader->SetMatrix4Uniform("uViewProj", viewProj);

	glBindVertexArray(mVao);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glBufferData(GL_ARRAY_BUFFER, buf.Size() * sizeof(float), buf.Data(), GL_DYNAMIC_DRAW);

	glDrawArrays(GL_LINES, 0, raycastsToDraw * 2);
	glBindVertexArray(0);

	for (auto idx : indiciesToRemove) {
		mRaycastsToDraw.RemoveAt(idx);
	}
}

RaycastHit PhysicsWorld::Raycast(const Vec3 &Origin, const Vec3 &Direction, float MaxDistance,
                                 RaycastDebugSettings DebugDrawSettings)
{
	RaycastHit HitResult;

	JPH::RRayCast    Ray    { ToJPH(Origin), ToJPH(Direction) * MaxDistance };
	JPH::RayCastResult Result;

	HitResult.Hit = mPhysicsSystem->GetNarrowPhaseQuery().CastRay(Ray, Result);
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

void PhysicsWorld::Init()
{
	glGenVertexArrays(1, &mVao);
	glGenBuffers(1, &mVbo);
	glBindVertexArray(mVao);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);

	// aPos
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	// aColor
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);
}

void PhysicsWorld::Cleanup()
{
	glDeleteVertexArrays(1, &mVao);
	glDeleteBuffers(1, &mVbo);
}

void OverlapContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2,
                                            const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
	if (!inBody1.IsSensor() && !inBody2.IsSensor()) return;

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
}
#endif
