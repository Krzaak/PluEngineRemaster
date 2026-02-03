//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_SHADERCODE_H
#define PLUENGINE_SHADERCODE_H
#include "PluEngine/Objects/EngineObject.h"
#include "ShaderCode.generated.h"
#include "PluEngine/PluUUID.h"
#include "glm/fwd.hpp"
#include "PluEngine/Reflection/TypeTraits.h"

namespace Plu
{
	PLU_STRUCT()
	struct PLU_API IShaderUniform
	{
		REFLECTION_BODY_ISHADERUNIFORM()

		PLU_PROPERTY()
		String Name;

		PLU_PROPERTY()
		String Type;

		PLU_PROPERTY()
		int ArraySize;
	};

	template<typename T>
	struct PLU_API ShaderUniform : IShaderUniform
	{
		T Data;
	};

	template<>
	struct TypeSerializer<IShaderUniform>
	{
		static nlohmann::json Serialize(void* dataToSerialize)
		{
			IShaderUniform* data = static_cast<IShaderUniform *>(dataToSerialize);
			if (data->ArraySize == 0) {
				if (data->Type == "int") {
					return TypeSerializer<int>::Serialize(&static_cast<ShaderUniform<int>*>(data)->Data);
					//TODO this mess of a serializer, add name type itp
				}
				if (data->Type == "float") {
					return TypeSerializer<float>::Serialize(dataToSerialize);
				}
				if (data->Type == "bool") {
					return TypeSerializer<bool>::Serialize(dataToSerialize);
				}
				if (data->Type == "vec2") {
					return TypeSerializer<glm::vec2>::Serialize(dataToSerialize);
				}
				if (data->Type == "vec3") {
					return TypeSerializer<glm::vec3>::Serialize(dataToSerialize);
				}
				if (data->Type == "vec4") {
					return TypeSerializer<glm::vec4>::Serialize(dataToSerialize);
				}
			}
			return {};
		}
		static void Deserialize(DeserializationContext* deserializationContext, const nlohmann::json& json, void* outValue)
		{

		}
		static void EditorControl(void* value, const String& name)
		{
		}
	};

	//Runtime and Editor specific shader code loader
	PLU_CLASS(Abstract, UUID=Uuid)
	class PLU_API IShaderCode : public EngineObject
	{
		REFLECTION_BODY_ISHADERCODE()
	public:
		PLU_PROPERTY()
		PluUUID Uuid;

		PLU_PROPERTY()
		String Name;

		virtual String GetCode() = 0;
		virtual DynamicArray<TUsePointer<IShaderUniform>>* GetCodeUniforms() = 0;
	};
}

#endif //PLUENGINE_SHADERCODE_H
