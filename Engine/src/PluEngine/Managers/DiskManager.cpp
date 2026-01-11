//
// Created by Plutex on 1/6/26.
//

#include "PluEngine/Managers/DiskManager.h"
#include <fstream>
#include <string>
#include <locale>


namespace Plu
{
#ifdef PLU_PLATFORM_LINUX
	bool SaveJsonInternal(const String &path, const nlohmann::json &json)
	{
		std::ofstream out(path.CStr());
		try {
			out << json.dump(4);
		} catch (...) {
			PLU_CORE_ERROR("Error saving JSON!");
			return false;
		}
		try {
			out.close();
		} catch (...) {
			PLU_CORE_ERROR("Error closing file after JSON saving!");
			return false;
		}
		return true;
	}
#elif defined(PLU_PLATFORM_WINDOWS)
	bool SaveJsonInternal(const StringW &path, const nlohmann::json &json)
	{
		std::ofstream out(path.CStr());
		try
		{
			out << json.dump(4);
		} catch (...)
		{
			PLU_CORE_ERROR("Error saving JSON!");
			return false;
		}
		try
		{
			out.close();
		} catch (...)
		{
			PLU_CORE_ERROR("Error closing file after JSON saving!");
			return false;
		}
		return true;
	}
#endif

	bool DiskManager::SaveJson(const StringW &path, const nlohmann::json &json)
	{
#ifdef PLU_PLATFORM_LINUX
		const String narrow = String::FromWide(path.CStr());
		return SaveJsonInternal(narrow, json);
#elif defined(PLU_PLATFORM_WINDOWS)
		return SaveJsonInternal(path, json);
#endif
	}

	std::optional<nlohmann::json> DiskManager::LoadJson(const PathW &path)
	{
		try {
			nlohmann::json json;
#ifdef PLU_PLATFORM_LINUX
			std::ifstream in(String::FromWide(path.CStr()).CStr());
			json = nlohmann::json::parse(in);
#elif defined(PLU_PLATFORM_WINDOWS)
			std::ifstream in(path.CStr());
			json = nlohmann::json::parse(in);
#endif
			return json;
		} catch (...) {
			PLU_CORE_ERROR("Error loading JSON at: {}", String::FromWide(path.CStr()).CStr());
			return std::nullopt;
		}
	}

	DiskManager::DiskManager()
	{
	}

	DiskManager::~DiskManager()
	{
	}
}
