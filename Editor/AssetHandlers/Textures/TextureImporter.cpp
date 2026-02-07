//
// Created by Plutex on 2026-02-07.
//

#include "TextureImporter.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "PluEngine/Log.h"

bool Plu::TextureImport::ImportTexture(const PathW& origin, const PathW &outPath)
{
	int width = 0, height = 0, channels = 0;

	// Wczytaj piksele (RGBA wymuszone)
	unsigned char* data = stbi_load(origin.ToString().ToNarrow().CStr(),
									&width, &height, &channels, 4);

	if (!data) {
		PLU_ERROR("Error importing texture!");
		return false;
	}

	FILE* file = nullptr;

#ifdef _WIN32
	_wfopen_s(&file, outPath.CStr(), L"wb");
#else
	file = fopen(String::FromWide(outPath.CStr()).CStr(), "wb");
#endif

	if (!file)
	{
		PLU_ERROR("Failed to open file for writing: {}", String::FromWide(outPath.CStr()).CStr());
		stbi_image_free(data);
		return false;
	}

	// Magic number i wersja
	UInt32 magic = 0x41554C50;  // 'PLUA'
	UInt32 version = 1;
	fwrite(&magic, sizeof(UInt32), 1, file);
	fwrite(&version, sizeof(UInt32), 1, file);

	// Typ assetu
	const char* typeName = "TextureInfo";
	UInt32 typeLength = static_cast<UInt32>(strlen(typeName));
	fwrite(&typeLength, sizeof(UInt32), 1, file);
	fwrite(typeName, sizeof(char), typeLength, file);

	UInt64 uuid = PluUUID();
	fwrite(&uuid,sizeof(UInt64),1,file);

	UInt64 pixelCount = width * height * channels;
	fwrite(&width, sizeof(UInt32), 1, file);
	fwrite(&height, sizeof(UInt32), 1, file);
	fwrite(&channels, sizeof(UInt32), 1, file);

	fwrite(data, sizeof(unsigned char), pixelCount, file);
	fclose(file);
	stbi_image_free(data);
	return true;
}
