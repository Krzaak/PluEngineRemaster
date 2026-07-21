//
// Created by Plutex on 7/21/26.
//

#include "PluEngine/NodeGraph/GraphNode.h"
#include "PluEngine/Reflection/ReflectionBase.h"

namespace Plu
{
	// Reflected property types that map cleanly onto a Data pin. TypeId of the pin is exactly the
	// property's reflected type name, so connection validation is a plain string compare.
	static bool IsPinnableDataType(const String& typeName)
	{
		return typeName == "float" || typeName == "double"
			|| typeName == "bool"
			|| typeName == "int"    || typeName == "Int32"  || typeName == "UInt32"
			|| typeName == "Int64"  || typeName == "UInt64"
			|| typeName == "Vec2"   || typeName == "Vec3"   || typeName == "Vec4";
	}

	String GraphNode::GetDisplayName()
	{
		return GetClass()->TypeName;
	}

	void GraphNode::AddPin(const String& name, EPinDirection direction, EPinCategory category, const String& typeId)
	{
		NodePin pin(name, direction, category, typeId);
		if (direction == EPinDirection::Input) {
			InputPins.PushBack(pin);
		} else {
			OutputPins.PushBack(pin);
		}
	}

	void GraphNode::BuildDataPinsFromReflection()
	{
		// Walk the whole base chain so inherited PLU_PROPERTY fields also become pins.
		for (TypeInfo* type = GetClass(); type != nullptr; type = type->BaseType) {
			for (PropertyInfo* prop : type->Properties) {
				if (!prop) continue;
				if (prop->PropertyName == "Uuid") continue; // identity, not a data input
				if (!IsPinnableDataType(prop->PropertyTypeName)) continue;
				AddPin(prop->PropertyName, EPinDirection::Input, EPinCategory::Data, prop->PropertyTypeName);
			}
		}
	}

	NodePin* GraphNode::FindPin(const String& name, EPinDirection direction)
	{
		DynamicArray<NodePin>& pins = (direction == EPinDirection::Input) ? InputPins : OutputPins;
		for (NodePin& pin : pins) {
			if (pin.Name == name) return &pin;
		}
		return nullptr;
	}
}
