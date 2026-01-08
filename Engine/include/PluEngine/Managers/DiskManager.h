//
// Created by Plutex on 1/6/26.
//

#ifndef PLUENGINE_DISKMANAGER_H
#define PLUENGINE_DISKMANAGER_H
#include "PluEngine/Objects/EngineObject.h"
#include "nlohmann/json.hpp"
#include "DiskManager.generated.h"

namespace Plu
{
	PLU_CLASS()
	class PLU_API DiskManager : public EngineObject
	{
		REFLECTION_BODY_DISKMANAGER()
	public:
		static bool SaveJson(const StringW& path, const nlohmann::json& json);
		static std::optional<nlohmann::json> LoadJson(const StringW& path);


		DiskManager();
		~DiskManager() override;
	};
}

#endif //PLUENGINE_DISKMANAGER_H
