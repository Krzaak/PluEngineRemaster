//
// Created by Plutex on 1/6/26.
//

#ifndef PLUENGINE_DISKMANAGER_H
#define PLUENGINE_DISKMANAGER_H
#include <cstdio>

#include "PluEngine/Core/Objects/EngineObject.h"
#include "nlohmann/json.hpp"
#include "DiskManager.generated.h"

namespace Plu
{
	// =============================================================================
	// BinaryFileWriter — scoped (RAII) wrapper for sequential binary writing.
	// Closes the file automatically when it goes out of scope; CloseFile() can be
	// called manually to flush/close earlier and check the result. Non-copyable,
	// movable. Uses a large stdio buffer so bulk writes stay disk-bound.
	// =============================================================================
	class PLUPLATFORM_API BinaryFileWriter
	{
	public:
		BinaryFileWriter() noexcept = default;
		explicit BinaryFileWriter(const Path& path) noexcept { OpenFile(path); }
		explicit BinaryFileWriter(const PathW& path) noexcept { OpenFile(path); }
		~BinaryFileWriter() { CloseFile(); }

		BinaryFileWriter(const BinaryFileWriter&) = delete;
		BinaryFileWriter& operator=(const BinaryFileWriter&) = delete;
		BinaryFileWriter(BinaryFileWriter&& other) noexcept;
		BinaryFileWriter& operator=(BinaryFileWriter&& other) noexcept;

		bool OpenFile(const Path& path) noexcept;
		bool OpenFile(const PathW& path) noexcept;
		// Flushes and closes. Returns false if no write succeeded or an error occurred.
		bool CloseFile() noexcept;

		[[nodiscard]] bool IsOpen() const noexcept { return mFile != nullptr; }
		[[nodiscard]] bool HasError() const noexcept { return mError; }
		// Human-readable description of the last failure (open/write/close). Empty if none.
		[[nodiscard]] const String& GetLastError() const noexcept { return mLastError; }

		// Raw bytes.
		bool Write(const void* data, UInt64 size) noexcept;

		// Single POD value (trivially copyable).
		template <typename T>
		bool Write(const T& value) noexcept
		{
			static_assert(std::is_trivially_copyable_v<T>, "BinaryFileWriter::Write<T> requires a trivially copyable type");
			return Write(&value, sizeof(T));
		}

		// Contiguous array of POD values.
		template <typename T>
		bool WriteArray(const T* data, UInt64 count) noexcept
		{
			static_assert(std::is_trivially_copyable_v<T>, "BinaryFileWriter::WriteArray<T> requires a trivially copyable type");
			return Write(data, sizeof(T) * count);
		}

		// Length-prefixed UTF-8 string (UInt32 length + raw bytes, no null terminator).
		bool WriteString(const String& str) noexcept;

	private:
		// openErrno is the value of errno captured immediately after the failed fopen.
		bool OpenInternal(std::FILE* file, const char* debugName, int openErrno) noexcept;

		std::FILE* mFile = nullptr;
		bool mError = false;
		String mLastError;
	};

	// =============================================================================
	// BinaryFileReader — scoped (RAII) counterpart to BinaryFileWriter.
	// =============================================================================
	class PLUPLATFORM_API BinaryFileReader
	{
	public:
		BinaryFileReader() noexcept = default;
		explicit BinaryFileReader(const Path& path) noexcept { OpenFile(path); }
		explicit BinaryFileReader(const PathW& path) noexcept { OpenFile(path); }
		~BinaryFileReader() { CloseFile(); }

		BinaryFileReader(const BinaryFileReader&) = delete;
		BinaryFileReader& operator=(const BinaryFileReader&) = delete;
		BinaryFileReader(BinaryFileReader&& other) noexcept;
		BinaryFileReader& operator=(BinaryFileReader&& other) noexcept;

		bool OpenFile(const Path& path) noexcept;
		bool OpenFile(const PathW& path) noexcept;
		bool CloseFile() noexcept;

		[[nodiscard]] bool IsOpen() const noexcept { return mFile != nullptr; }
		// True once any read failed (short read / EOF where data was expected).
		[[nodiscard]] bool HasError() const noexcept { return mError; }
		// Human-readable description of the last failure (open/read). Empty if none.
		[[nodiscard]] const String& GetLastError() const noexcept { return mLastError; }

		// Raw bytes. Returns false (and sets error) on a short read.
		bool Read(void* data, UInt64 size) noexcept;

		template <typename T>
		bool Read(T& value) noexcept
		{
			static_assert(std::is_trivially_copyable_v<T>, "BinaryFileReader::Read<T> requires a trivially copyable type");
			return Read(&value, sizeof(T));
		}

		template <typename T>
		bool ReadArray(T* data, UInt64 count) noexcept
		{
			static_assert(std::is_trivially_copyable_v<T>, "BinaryFileReader::ReadArray<T> requires a trivially copyable type");
			return Read(data, sizeof(T) * count);
		}

		// Reads a length-prefixed UTF-8 string written by WriteString.
		bool ReadString(String& outStr) noexcept;

	private:
		// openErrno is the value of errno captured immediately after the failed fopen.
		bool OpenInternal(std::FILE* file, const char* debugName, int openErrno) noexcept;

		std::FILE* mFile = nullptr;
		bool mError = false;
		String mLastError;
	};

	PLU_CLASS()
	class PLUPLATFORM_API DiskManager : public EngineObject
	{
		REFLECTION_BODY_DISKMANAGER()
	public:
		static bool SaveJson(const StringW& path, const nlohmann::json& json);
		static std::optional<nlohmann::json> LoadJson(const PathW &path);

		// Writes text verbatim, overwriting the file. For anything that is already a formatted
		// string (CSV dumps, reports) rather than a structure to serialise.
		static bool SaveText(const StringW& path, const String& text);


		DiskManager();
		~DiskManager() override;
	};
}

#endif //PLUENGINE_DISKMANAGER_H
