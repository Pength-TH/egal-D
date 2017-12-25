#include "common/serializer/serializer.h"
#include "common/egal-d.h"

namespace egal
{
	static e_uint32 asUint32(float v)
	{
		return *(e_uint32*)&v;
	}

	static float asFloat(e_uint32 v)
	{
		return *(float*)&v;
	}

	void TextSerializer::write(const e_char* label, GameObject entity)
	{
		GameObjectGUID guid = entity_map.get(entity);
		blob << "#" << label << "\n\t" << guid.value << "\n";
	}

	void TextSerializer::write(const e_char* label, ComponentHandle value)
	{
		blob << "#" << label << "\n\t" << value.index << "\n";
	}

	void TextSerializer::write(const e_char* label, const RigidTransform& value)
	{
		blob << "#" << label << " (" << value.pos.x << ", " << value.pos.y << ", " << value.pos.z << ") "
			<< " (" << value.rot.x << ", " << value.rot.y << ", " << value.rot.z << ", " << value.rot.w << ")\n\t"
			<< asUint32(value.pos.x) << "\n\t" << asUint32(value.pos.y) << "\n\t" << asUint32(value.pos.z) << "\n\t"
			<< asUint32(value.rot.x) << "\n\t" << asUint32(value.rot.y) << "\n\t" << asUint32(value.rot.z) << "\n\t"
			<< asUint32(value.rot.w) << "\n";
	}

	void TextSerializer::write(const e_char* label, const float3& value)
	{
		blob << "#" << label << " (" << value.x << ", " << value.y << ", " << value.z << ")\n\t" << asUint32(value.x) << "\n\t"
			<< asUint32(value.y) << "\n\t" << asUint32(value.z) << "\n";
	}

	void TextSerializer::write(const e_char* label, const float4& value)
	{
		blob << "#" << label << " (" << value.x << ", " << value.y << ", " << value.z << ", " << value.w << ")\n\t"
			<< asUint32(value.x) << "\n\t" << asUint32(value.y) << "\n\t" << asUint32(value.z) << "\n\t" << asUint32(value.w) << "\n";
	}

	void TextSerializer::write(const e_char* label, const Quaternion& value)
	{
		blob << "#" << label << " (" << value.x << ", " << value.y << ", " << value.z << ", " << value.w << ")\n\t"
			<< asUint32(value.x) << "\n\t" << asUint32(value.y) << "\n\t" << asUint32(value.z) << "\n\t" << asUint32(value.w) << "\n";
	}

	void TextSerializer::write(const e_char* label, float value)
	{
		blob << "#" << label << " " << value << "\n\t" << asUint32(value) << "\n";
	}

	void TextSerializer::write(const e_char* label, e_bool value)
	{
		blob << "#" << label << "\n\t" << (e_uint32)value << "\n";
	}

	void TextSerializer::write(const e_char* label, const e_char* value)
	{
		blob << "#" << label << "\n\t\"" << value << "\"\n";
	}

	void TextSerializer::write(const e_char* label, e_uint32 value)
	{
		blob << "#" << label << "\n\t" << value << "\n";
	}

	void TextSerializer::write(const e_char* label, e_int64 value)
	{
		blob << "#" << label << "\n\t" << value << "\n";
	}

	void TextSerializer::write(const e_char* label, e_uint64 value)
	{
		blob << "#" << label << "\n\t" << value << "\n";
	}

	void TextSerializer::write(const e_char* label, e_int32 value)
	{
		blob << "#" << label << "\n\t" << value << "\n";
	}

	void TextSerializer::write(const e_char* label, e_int8 value)
	{
		blob << "#" << label << "\n\t" << value << "\n";
	}

	void TextSerializer::write(const e_char* label, e_uint8 value)
	{
		blob << "#" << label << "\n\t" << value << "\n";
	}


	GameObject TextDeserializer::getGameObject(GameObjectGUID guid)
	{
		return entity_map.get(guid);
	}


	void TextDeserializer::read(GameObject* entity)
	{
		GameObjectGUID guid;
		read(&guid.value);
		*entity = entity_map.get(guid);
	}


	void TextDeserializer::read(RigidTransform* value)
	{
		skip();
		value->pos.x = asFloat(readU32());
		skip();
		value->pos.y = asFloat(readU32());
		skip();
		value->pos.z = asFloat(readU32());
		skip();
		value->rot.x = asFloat(readU32());
		skip();
		value->rot.y = asFloat(readU32());
		skip();
		value->rot.z = asFloat(readU32());
		skip();
		value->rot.w = asFloat(readU32());
	}


	void TextDeserializer::read(float3* value)
	{
		skip();
		value->x = asFloat(readU32());
		skip();
		value->y = asFloat(readU32());
		skip();
		value->z = asFloat(readU32());
	}


	void TextDeserializer::read(float4* value)
	{
		skip();
		value->x = asFloat(readU32());
		skip();
		value->y = asFloat(readU32());
		skip();
		value->z = asFloat(readU32());
		skip();
		value->w = asFloat(readU32());
	}


	void TextDeserializer::read(Quaternion* value)
	{
		skip();
		value->x = asFloat(readU32());
		skip();
		value->y = asFloat(readU32());
		skip();
		value->z = asFloat(readU32());
		skip();
		value->w = asFloat(readU32());
	}

	void TextDeserializer::read(ComponentHandle* value)
	{
		skip();
		value->index = readU32();
	}


	void TextDeserializer::read(float* value)
	{
		skip();
		*value = asFloat(readU32());
	}


	void TextDeserializer::read(e_bool* value)
	{
		skip();
		*value = readU32() != 0;
	}


	void TextDeserializer::read(e_uint32* value)
	{
		skip();
		*value = readU32();
	}


	void TextDeserializer::read(e_uint64* value)
	{
		skip();
		e_char tmp[40];
		e_char* c = tmp;
		*c = blob.readChar();
		while (*c >= '0' && *c <= '9' && (c - tmp) < TlengthOf(tmp))
		{
			++c;
			*c = blob.readChar();
		}
		*c = 0;
		StringUnitl::fromCString(tmp, TlengthOf(tmp), value);
	}


	void TextDeserializer::read(e_int64* value)
	{
		skip();
		e_char tmp[40];
		e_char* c = tmp;
		*c = blob.readChar();
		if (*c == '-')
		{
			++c;
			*c = blob.readChar();
		}
		while (*c >= '0' && *c <= '9' && (c - tmp) < TlengthOf(tmp))
		{
			++c;
			*c = blob.readChar();
		}
		*c = 0;
		StringUnitl::fromCString(tmp, TlengthOf(tmp), value);
	}


	void TextDeserializer::read(e_int32* value)
	{
		skip();
		e_char tmp[20];
		e_char* c = tmp;
		*c = blob.readChar();
		if (*c == '-')
		{
			++c;
			*c = blob.readChar();
		}
		while (*c >= '0' && *c <= '9' && (c - tmp) < TlengthOf(tmp))
		{
			++c;
			*c = blob.readChar();
		}
		*c = 0;
		StringUnitl::fromCString(tmp, TlengthOf(tmp), value);
	}


	void TextDeserializer::read(e_uint8* value)
	{
		skip();
		*value = (e_uint8)readU32();
	}


	void TextDeserializer::read(e_int8* value)
	{
		skip();
		*value = (e_int8)readU32();
	}


	void TextDeserializer::read(e_char* value, int max_size)
	{
		skip();
		e_uint8 c = blob.readChar();
		ASSERT(c == '"');
		e_char* out = value;
		*out = blob.readChar();
		while (*out != '"' && out - value < max_size - 1)
		{
			++out;
			*out = blob.readChar();
		}
		ASSERT(*out == '"');
		*out = 0;
	}

	e_uint32 TextDeserializer::readU32()
	{
		e_char tmp[20];
		e_char* c = tmp;
		*c = blob.readChar();
		while (*c >= '0' && *c <= '9' && (c - tmp) < TlengthOf(tmp))
		{
			++c;
			*c = blob.readChar();
		}
		*c = 0;
		e_uint32 v;
		StringUnitl::fromCString(tmp, TlengthOf(tmp), &v);
		return v;
	}

	void TextDeserializer::skip()
	{
		e_uint8 c = blob.readChar();
		if (c == '#')
			while (blob.readChar() != '\n')
				;
		if (c == '\t') return;
		while (blob.readChar() != '\t')
			;
	}

}
