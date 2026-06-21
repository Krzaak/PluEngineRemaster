//
// Created by Plutex on 6/21/26.
//

#include "PluEngine/Renderer/RenderSnapshotBuilder.h"

#include "PluEngine/Renderer/RenderThreading.h"

Plu::RenderSnapshotBuilder::RenderSnapshotBuilder() : RenderSnapshotBuilder(nullptr, nullptr)
{
}

Plu::RenderSnapshotBuilder::RenderSnapshotBuilder(TripleBuffer<RenderSnapshot *> *tripleBuffer,
    ApplicationInfo *applicationInfo)
{
    this->mTripleBuffer = tripleBuffer;
    this->mAppInfo = applicationInfo;
    PLU_CORE_TRACE("Render Snapshot Builder Initialized");
}

Plu::RenderSnapshotBuilder::~RenderSnapshotBuilder()
{
}

void Plu::RenderSnapshotBuilder::BuildSnapshotAndPublish()
{
    if (!mTripleBuffer || !mAppInfo) return;

    RenderSnapshot*& snapshot = mTripleBuffer->GetWriteBuffer();
    if (snapshot == nullptr) {
        snapshot = new RenderSnapshot();
    } else {
        snapshot->Clear();
    }

    //Here will be building

    mTripleBuffer->Publish();
}
