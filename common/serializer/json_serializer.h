#ifndef _json_serializer_h_
#define _json_serializer_h_
#pragma once

#include "common/type.h"
#include "common/struct.h"

namespace egal
{
	namespace FS 
	{
		struct IFile;
	}

	class ArchivePath;
	struct IAllocator;

	class JsonSerializer
	{
		friend class ErrorProxy;
	public:
		enum AccessMode
		{
			READ,
			WRITE
		};

	public:
		JsonSerializer(FS::IFile& file, AccessMode access_mode, const ArchivePath& path, IAllocator& allocator);
		~JsonSerializer();

		// serialize
		void serialize(const e_char* label, GameObject value);
		void serialize(const e_char* label, ComponentHandle value);
		void serialize(const e_char* label, e_uint32 value);
		void serialize(const e_char* label, e_uint16 value);
		void serialize(const e_char* label, e_float value);
		void serialize(const e_char* label, e_int32 value);
		void serialize(const e_char* label, const e_char* value);
		void serialize(const e_char* label, const ArchivePath& value);
		void serialize(const e_char* label, e_bool value);
		void beginObject();
		void beginObject(const e_char* label);
		void endObject();
		void beginArray(const e_char* label);
		void endArray();
		void serializeArrayItem(GameObject value);
		void serializeArrayItem(ComponentHandle value);
		void serializeArrayItem(e_uint32 value);
		void serializeArrayItem(e_int32 value);
		void serializeArrayItem(e_int64 value);
		void serializeArrayItem(e_float value);
		void serializeArrayItem(e_bool value);
		void serializeArrayItem(const e_char* value);

		// deserialize
		void deserialize(const e_char* label, GameObject& value, GameObject default_value);
		void deserialize(const e_char* label, ComponentHandle& value, ComponentHandle default_value);
		void deserialize(const e_char* label, e_uint32& value, e_uint32 default_value);
		void deserialize(const e_char* label, e_uint16& value, e_uint16 default_value);
		void deserialize(const e_char* label, e_float& value, e_float default_value);
		void deserialize(const e_char* label, e_int32& value, e_int32 default_value);
		void deserialize(const e_char* label, e_char* value, e_int32 max_length, const e_char* default_value);
		void deserialize(const e_char* label, ArchivePath& value, const ArchivePath& default_value);
		void deserialize(const e_char* label, e_bool& value, e_bool default_value);
		void deserialize(e_char* value, e_int32 max_length, const e_char* default_value);
		void deserialize(ArchivePath& path, const ArchivePath& default_value);
		void deserialize(e_bool& value, e_bool default_value);
		void deserialize(e_float& value, e_float default_value);
		void deserialize(e_int32& value, e_int32 default_value);
		void deserializeArrayBegin(const e_char* label);
		void deserializeArrayBegin();
		void deserializeArrayEnd();
		e_bool isArrayEnd();
		void deserializeArrayItem(GameObject& value, GameObject default_value);
		void deserializeArrayItem(ComponentHandle& value, ComponentHandle default_value);
		void deserializeArrayItem(e_uint32& value, e_uint32 default_value);
		void deserializeArrayItem(e_int32& value, e_int32 default_value);
		void deserializeArrayItem(e_int64& value, e_int64 default_value);
		void deserializeArrayItem(e_float& value, e_float default_value);
		void deserializeArrayItem(e_bool& value, e_bool default_value);
		void deserializeArrayItem(e_char* value, e_int32 max_length, const e_char* default_value);
		void deserializeObjectBegin();
		void deserializeObjectEnd();
		void deserializeLabel(e_char* label, e_int32 max_length);
		void deserializeRawString(e_char* buffer, e_int32 max_length);
		void nextArrayItem();
		e_bool isNextBoolean() const;
		e_bool isObjectEnd();

		e_bool isError() const { return m_is_error; }

	private:
		void deserializeLabel(const e_char* label);
		void deserializeToken();
		void deserializeArrayComma();
		e_float tokenToFloat();
		void expectToken(e_char expected_token);
		void writeString(const e_char* str);
		void writeBlockComma();

	private:
		void operator=(const JsonSerializer&);
		JsonSerializer(const JsonSerializer&);

	private:
		AccessMode m_access_mode;
		e_bool m_is_first_in_block;
		FS::IFile& m_file;
		const e_char* m_token;
		e_int32 m_token_size;
		e_bool m_is_string_token;
		e_char m_path[MAX_PATH_LENGTH];
		IAllocator& m_allocator;

		const e_char* m_data;
		e_int32 m_data_size;
		e_bool m_own_data;
		e_bool m_is_error;
	};
}

#endif