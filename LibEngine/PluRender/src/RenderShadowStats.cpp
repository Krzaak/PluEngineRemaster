//
// Created by Plutex on 8/12/26.
//

#include "PluEngine/Render/RenderUsageStats.h"

#include <atomic>

#include "PluEngine/Render/RenderUtils.h"

namespace
{
	// Per-cascade caster counts. Fixed-size array (bounded by Plu::kMaxShadowCascades) so the
	// mirror stays lock-free like the counters above.
	std::atomic<UInt32> gStatShadowCascadeCount{0};
	std::atomic<UInt32> gStatShadowCascadeCasters[Plu::kMaxShadowCascades] = {};

	// Same mirrors for the spot lights (see SetSpotLightStats).
	std::atomic<UInt32> gStatVisibleSpotLights{0};
	std::atomic<UInt32> gStatSpotShadowSlots{0};
	std::atomic<UInt32> gStatSpotShadowCasters[Plu::kMaxSpotShadowSlots] = {};
}

void Plu::SetShadowCascadeStats(const UInt32* casterCounts, UInt32 cascadeCount)
{
	const UInt32 count = cascadeCount < static_cast<UInt32>(kMaxShadowCascades)
	                   ? cascadeCount
	                   : static_cast<UInt32>(kMaxShadowCascades);
	for (UInt32 c = 0; c < count; c++) {
		gStatShadowCascadeCasters[c].store(casterCounts[c], std::memory_order_relaxed);
	}
	gStatShadowCascadeCount.store(count, std::memory_order_relaxed);
}

UInt32 Plu::GetStatShadowCascadeCount()
{
	return gStatShadowCascadeCount.load(std::memory_order_relaxed);
}

UInt32 Plu::GetStatShadowCascadeCasters(UInt32 cascadeIndex)
{
	if (cascadeIndex >= static_cast<UInt32>(kMaxShadowCascades)) return 0;
	return gStatShadowCascadeCasters[cascadeIndex].load(std::memory_order_relaxed);
}

void Plu::SetSpotLightStats(const UInt32* casterCounts, UInt32 slotCount, UInt32 visibleLights)
{
	const UInt32 count = slotCount < static_cast<UInt32>(kMaxSpotShadowSlots)
	                   ? slotCount
	                   : static_cast<UInt32>(kMaxSpotShadowSlots);
	for (UInt32 s = 0; s < count; s++) {
		gStatSpotShadowCasters[s].store(casterCounts[s], std::memory_order_relaxed);
	}
	gStatSpotShadowSlots.store(count, std::memory_order_relaxed);
	gStatVisibleSpotLights.store(visibleLights, std::memory_order_relaxed);
}

UInt32 Plu::GetStatVisibleSpotLights()
{
	return gStatVisibleSpotLights.load(std::memory_order_relaxed);
}

UInt32 Plu::GetStatSpotShadowSlots()
{
	return gStatSpotShadowSlots.load(std::memory_order_relaxed);
}

UInt32 Plu::GetStatSpotShadowCasters(UInt32 slotIndex)
{
	if (slotIndex >= static_cast<UInt32>(kMaxSpotShadowSlots)) return 0;
	return gStatSpotShadowCasters[slotIndex].load(std::memory_order_relaxed);
}
