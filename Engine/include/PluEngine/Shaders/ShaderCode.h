//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_SHADERCODE_H
#define PLUENGINE_SHADERCODE_H
#include "PluEngine/Objects/EngineObject.h"
#include "ShaderCode.generated.h"
#include "PluEngine/PluUUID.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
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
			nlohmann::json j = {};
			if (data->ArraySize == 0) {
				if (data->Type == "int") {
					j = TypeSerializer<int>::Serialize(&static_cast<ShaderUniform<int>*>(data)->Data);
				} else if (data->Type == "float") {
					j = TypeSerializer<float>::Serialize(&static_cast<ShaderUniform<float>*>(data)->Data);
				} else if (data->Type == "bool") {
					j = TypeSerializer<bool>::Serialize(&static_cast<ShaderUniform<bool>*>(data)->Data);
				} else if (data->Type == "vec2") {
					j = TypeSerializer<glm::vec2>::Serialize(&static_cast<ShaderUniform<glm::vec2>*>(data)->Data);
				} else if (data->Type == "vec3") {
					j = TypeSerializer<glm::vec3>::Serialize(&static_cast<ShaderUniform<glm::vec3>*>(data)->Data);
				} else if (data->Type == "vec4") {
					j = TypeSerializer<glm::vec4>::Serialize(&static_cast<ShaderUniform<glm::vec4>*>(data)->Data);
				}
				return {
					{"name", data->Name.CStr()},
					{"type", data->Type.CStr()},
					{"value", j}
				};
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
		virtual DynamicArray<TOwningPointer<IShaderUniform>>* GetCodeUniforms() = 0;
		virtual void RenewUniforms() = 0;
	};
}

#endif //PLUENGINE_SHADERCODE_H
