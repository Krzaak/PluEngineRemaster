//
// Created by Plutex on 1/6/26.
//

#include "PluEngine/Managers/DiskManager.h"
#include <fstream>
#include <string>
#include <locale>

#include "PluEngine/Log.h"

namespace Plu
{
	// 256 KB stdio buffer — keeps bulk sequential writes/reads disk-bound.
	static constexpr UInt64 kBinaryIOBufferSize = 256 * 1024;

	// =========================================================================
	// BinaryFileWriter
	// =========================================================================
	BinaryFileWriter::BinaryFileWriter(BinaryFileWriter&& other) noexcept
		: mFile(other.mFile), mError(other.mError)
	{
		other.mFile = nullptr;
		other.mError = false;
	}

	BinaryFileWriter& BinaryFileWriter::operator=(BinaryFileWriter&& other) noexcept
	{
		if (this != &other)
		{
			CloseFile();
			mFile = other.mFile;
			mError = other.mError;
			other.mFile = nullptr;
			other.mError = false;
		}
		return *this;
	}

	bool BinaryFileWriter::OpenInternal(std::FILE* file, const char* debugName) noexcept
	{
		if (mFile) CloseFile();
		if (!file)
		{
			PLU_CORE_ERROR("Failed to open file for writing: {}", debugName);
			mError = true;
			return false;
		}
		mFile = file;
		mError = false;
		std::setvbuf(mFile, nullptr, _IOFBF, kBinaryIOBufferSize);
		return true;
	}

	bool BinaryFileWriter::OpenFile(const Path& path) noexcept
	{
#ifdef _WIN32
		std::FILE* file = std::fopen(path.CStr(), "wb");
#else
		std::FILE* file = std::fopen(path.CStr(), "wb");
#endif
		return OpenInternal(file, path.CStr());
	}

	bool BinaryFileWriter::OpenFile(const PathW& path) noexcept
	{
#ifdef _WIN32
		std::FILE* file = nullptr;
		_wfopen_s(&file, path.CStr(), L"wb");
		const String narrow = String::FromWide(path.CStr());
		return OpenInternal(file, narrow.CStr());
#else
		const String narrow = String::FromWide(path.CStr());
		std::FILE* file = std::fopen(narrow.CStr(), "wb");
		return OpenInternal(file, narrow.CStr());
#endif
	}

	bool BinaryFileWriter::Write(const void* data, UInt64 size) noexcept
	{
		if (!mFile || mError) return false;
		if (size == 0) return true;
		if (std::fwrite(data, 1, size, mFile) != size)
		{
			PLU_CORE_ERROR("BinaryFileWriter: short write ({} bytes)", size);
			mError = true;
			return false;
		}
		return true;
	}

	bool BinaryFileWriter::WriteString(const String& str) noexcept
	{
		const UInt32 length = static_cast<UInt32>(str.Length());
		if (!Write(length)) return false;
		return Write(str.CStr(), length);
	}

	bool BinaryFileWriter::CloseFile() noexcept
	{
		if (!mFile) return false;
		const bool ok = std::fclose(mFile) == 0 && !mError;
		if (!ok && !mError) PLU_CORE_ERROR("Error closing file after binary write!");
		mFile = nullptr;
		return ok;
	}

	// =========================================================================
	// BinaryFileReader
	// =========================================================================
	BinaryFileReader::BinaryFileReader(BinaryFileReader&& other) noexcept
		: mFile(other.mFile), mError(other.mError)
	{
		other.mFile = nullptr;
		other.mError = false;
	}

	BinaryFileReader& BinaryFileReader::operator=(BinaryFileReader&& other) noexcept
	{
		if (this != &other)
		{
			CloseFile();
			mFile = other.mFile;
			mError = other.mError;
			other.mFile = nullptr;
			other.mError = false;
		}
		return *this;
	}

	bool BinaryFileReader::OpenInternal(std::FILE* file, const char* debugName) noexcept
	{
		if (mFile) CloseFile();
		if (!file)
		{
			PLU_CORE_ERROR("Failed to open file for reading: {}", debugName);
			mError = true;
			return false;
		}
		mFile = file;
		mError = false;
		std::setvbuf(mFile, nullptr, _IOFBF, kBinaryIOBufferSize);
		return true;
	}

	bool BinaryFileReader::OpenFile(const Path& path) noexcept
	{
		std::FILE* file = std::fopen(path.CStr(), "rb");
		return OpenInternal(file, path.CStr());
	}

	bool BinaryFileReader::OpenFile(const PathW& path) noexcept
	{
#ifdef _WIN32
		std::FILE* file = nullptr;
		_wfopen_s(&file, path.CStr(), L"rb");
		const String narrow = String::FromWide(path.CStr());
		return OpenInternal(file, narrow.CStr());
#else
		const String narrow = String::FromWide(path.CStr());
		std::FILE* file = std::fopen(narrow.CStr(), "rb");
		return OpenInternal(file, narrow.CStr());
#endif
	}

	bool BinaryFileReader::Read(void* data, UInt64 size) noexcept
	{
		if (!mFile || mError) return false;
		if (size == 0) return true;
		if (std::fread(data, 1, size, mFile) != size)
		{
			PLU_CORE_ERROR("BinaryFileReader: short read ({} bytes expected)", size);
			mError = true;
			return false;
		}
		return true;
	}

	bool BinaryFileReader::ReadString(String& outStr) noexcept
	{
		UInt32 length = 0;
		if (!Read(length)) return false;
		if (length == 0)
		{
			outStr = String();
			return true;
		}
		DynamicArray<char> buffer;
		buffer.Resize(length);
		if (!Read(buffer.Data(), length)) return false;
		outStr = String(buffer.Data(), length);
		return true;
	}

	bool BinaryFileReader::CloseFile() noexcept
	{
		if (!mFile) return false;
		const bool ok = std::fclose(mFile) == 0 && !mError;
		mFile = nullptr;
		return ok;
	}

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
