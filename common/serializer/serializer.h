#ifndef _serializer_h_
#define _serializer_h_

#include "common/type.h"
#include "common/struct.h"
#include "common/math/egal_math.h"

namespace egal
{
	class WriteBinary;
	class ReadBinary;
	struct GameObjectGUID
	{
		e_uint64 value;
		e_bool operator ==(const GameObjectGUID& rhs) const { return value == rhs.value; }
	};


	const GameObjectGUID INVALID_ENTITY_GUID = { 0xffffFFFFffffFFFF };
	inline e_bool isValid(GameObjectGUID guid) { return guid.value != INVALID_ENTITY_GUID.value; }


	struct ISaveGameObjectGUIDMap
	{
		virtual ~ISaveGameObjectGUIDMap() {}
		virtual GameObjectGUID get(GameObject entity) = 0;
	};


	struct ILoadGameObjectGUIDMap
	{
		virtual ~ILoadGameObjectGUIDMap() {}
		virtual GameObject get(GameObjectGUID guid) = 0;
	};

	//-----------------------------------------------------------------------------
	struct ISerializer
	{
		virtual ~ISerializer() {}
		virtual void write(const e_char* label, GameObject gamobj) = 0;
		virtual void write(const e_char* label, ComponentHandle value) = 0;
		virtual void write(const e_char* label, const RigidTransform& value) = 0;
		virtual void write(const e_char* label, const float2& value) = 0;
		virtual void write(const e_char* label, const float4& value) = 0;
		virtual void write(const e_char* label, const float3& value) = 0;
		virtual void write(const e_char* label, const Quaternion& value) = 0;
		virtual void write(const e_char* label, e_float value) = 0;
		virtual void write(const e_char* label, e_bool value) = 0;
		virtual void write(const e_char* label, e_int64 value) = 0;
		virtual void write(const e_char* label, e_uint64 value) = 0;
		virtual void write(const e_char* label, e_int32 value) = 0;
		virtual void write(const e_char* label, e_uint32 value) = 0;
		virtual void write(const e_char* label, e_int8 value) = 0;
		virtual void write(const e_char* label, e_uint8 value) = 0;
		virtual void write(const e_char* label, const e_char* value) = 0;
		virtual GameObjectGUID getGUID(GameObject entity) = 0;
	};

	struct IDeserializer
	{
		virtual ~IDeserializer() {}

		virtual void read(GameObject* entity) = 0;
		virtual void read(ComponentHandle* value) = 0;
		virtual void read(RigidTransform* value) = 0;
		virtual void read(float2* value) = 0;
		virtual void read(float4* value) = 0;
		virtual void read(float3* value) = 0;
		virtual void read(Quaternion* value) = 0;
		virtual void read(e_float* value) = 0;
		virtual void read(e_bool* value) = 0;
		virtual void read(e_uint64* value) = 0;
		virtual void read(e_int64* value) = 0;
		virtual void read(e_uint32* value) = 0;
		virtual void read(e_int32* value) = 0;
		virtual void read(e_uint8* value) = 0;
		virtual void read(e_int8* value) = 0;
		virtual void read(e_char* value, e_int32 max_size) = 0;
		virtual GameObject getGameObject(GameObjectGUID guid) = 0;
	};

	//-----------------------------------------------------------------------------
	struct TextSerializer : public ISerializer
	{
		TextSerializer(WriteBinary& _blob, ISaveGameObjectGUIDMap& _entity_map)
			: blob(_blob)
			, entity_map(_entity_map)
		{
		}

		void write(const e_char* label, GameObject entity) override;
		void write(const e_char* label, ComponentHandle value) override;
		void write(const char* label, const RigidTransform& value)  override;
		void write(const e_char* label, const float4& value) override;
		void write(const e_char* label, const float3& value)  override;
		void write(const e_char* label, const Quaternion& value) override;
		void write(const e_char* label, e_float value)  override;
		void write(const e_char* label, e_bool value)  override;
		void write(const e_char* label, e_int64 value)  override;
		void write(const e_char* label, e_uint64 value) override;
		void write(const e_char* label, e_int32 value)  override;
		void write(const e_char* label, e_uint32 value)  override;
		void write(const e_char* label, e_int8 value)  override;
		void write(const e_char* label, e_uint8 value)  override;
		void write(const e_char* label, const e_char* value)  override;
		GameObjectGUID getGUID(GameObject entity) override;

		WriteBinary& blob;
		ISaveGameObjectGUIDMap& entity_map;
	};


	struct TextDeserializer : public IDeserializer
	{
		TextDeserializer(ReadBinary& _blob, ILoadGameObjectGUIDMap& _entity_map)
			: blob(_blob)
			, entity_map(_entity_map)
		{
		}

		virtual void read(GameObject* entity)  override;
		virtual void read(ComponentHandle* value) override;
		void read(RigidTransform* value)  override;
		virtual void read(float4* value)  override;
		virtual void read(float3* value)  override;
		virtual void read(Quaternion* value)  override;
		virtual void read(e_float* value) override;
		virtual void read(e_bool* value)  override;
		virtual void read(e_uint64* value)  override;
		virtual void read(e_int64* value)  override;
		virtual void read(e_uint32* value)  override;
		virtual void read(e_int32* value)  override;
		virtual void read(e_uint8* value)  override;
		virtual void read(e_int8* value)  override;
		virtual void read(e_char* value, e_int32 max_size)  override;
		GameObject getGameObject(GameObjectGUID guid) override;

		void skip();
		e_uint32 readU32();

		ReadBinary& blob;
		ILoadGameObjectGUIDMap& entity_map;
	};


}
#endif