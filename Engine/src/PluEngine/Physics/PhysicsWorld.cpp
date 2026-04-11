//
// Created by Plutex on 2026-03-07.
//

#include "PluEngine/Physics/PhysicsWorld.h"
#include <Jolt/Physics/Body/BodyManager.h>
#include <glad/glad.h>

#include "glm/gtc/type_ptr.hpp"
#include "PluEngine/PluUtils.h"
#include "PluEngine/BasicEngineClasses/Components/PhysicsBodyComponent.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Physics/PhysicsCompoundShape.h"

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
		compoundShape->Init(physicsBodiesComponents);
		component->GetParentGameObject()->mCompoundShape = compoundShape;
		mBodiesPerObject.Remove(mEngineObjectManager->GetObjectAsUser<PhysicsBody>(component->GetParentGameObject()->mPhysicsBodyHandle)->GetID().GetIndexAndSequenceNumber());
		mEngineObjectManager->DestroyObject(component->GetParentGameObject()->mPhysicsBodyHandle);
		component->GetParentGameObject()->mPhysicsBodyHandle = mEngineObjectManager->CreateObject<PhysicsBody>(
			GetBodyInterface(),
			compoundShape->GetCompoundShape(),
			ToJPH(component->GetParentGameObject()->GetObjectLocation())
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
		compoundShape->Init(physicsBodiesComponents);
		mEngineObjectManager->DestroyObject(object.second->mPhysicsBodyHandle);
		object.second->mPhysicsBodyHandle = mEngineObjectManager->CreateObject<PhysicsBody>(
			GetBodyInterface(),
			compoundShape->GetCompoundShape(),
			ToJPH(object.second->GetObjectLocation())
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

void PhysicsWorld::DrawDebugRaycasts(float deltaTime, Matrix4 viewProj)
{
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

	glUseProgram(mShader);
	glUniformMatrix4fv(glGetUniformLocation(mShader, "uViewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));

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
		HitResult.HitObject = mSceneWorld->GetGameObjectByUUID(mBodiesPerObject[Result.mBodyID.GetIndexAndSequenceNumber()]);
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
	mShader = BuildShader();

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
	glDeleteProgram(mShader);
}

GLuint PhysicsWorld::BuildShader()
{
	const char* vert = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        uniform mat4 uViewProj;
        out vec3 vColor;
        void main()
        {
            vColor = aColor;
            gl_Position = uViewProj * vec4(aPos, 1.0);
        }
    )glsl";

	const char* frag = R"glsl(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() { FragColor = vec4(vColor, 1.0); }
    )glsl";

	auto compile = [](GLenum type, const char* src) {
		GLuint s = glCreateShader(type);
		glShaderSource(s, 1, &src, nullptr);
		glCompileShader(s);
		return s;
	};

	GLuint vs = compile(GL_VERTEX_SHADER, vert);
	GLuint fs = compile(GL_FRAGMENT_SHADER, frag);
	GLuint prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);
	glDeleteShader(vs);
	glDeleteShader(fs);
	return prog;
}
