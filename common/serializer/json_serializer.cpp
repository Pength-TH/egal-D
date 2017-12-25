#include "common/serializer/json_serializer.h"
#include "common/egal-d.h"
#include "common/resource/resource_public.h"

#include <cstdlib>

namespace egal
{
	class ErrorProxy
	{
	public:
		explicit ErrorProxy(JsonSerializer& serializer, const e_char* message)
		{
			serializer.m_is_error = true;
			const e_char* c = serializer.m_data;
			e_int32 line = 0;
			e_int32 column = 0;
			while (c < serializer.m_token)
			{
				if (*c == '\n')
				{
					++line;
					column = 0;
				}
				++column;
				++c;
			}

			log_error("%s line %s,column %s:%s.", serializer.m_path, (line + 1), column, message);
		}
	};

	JsonSerializer::JsonSerializer(FS::IFile& file,
		AccessMode access_mode,
		const ArchivePath& path,
		IAllocator& allocator)
		: m_file(file)
		, m_access_mode(access_mode)
		, m_allocator(allocator)
	{
		m_is_error = false;
		StringUnitl::copyString(m_path, path.c_str());
		m_is_first_in_block = true;
		m_data = nullptr;
		m_is_string_token = false;
		if (m_access_mode == READ)
		{
			m_data_size = (e_int32)file.size();
			if (file.getBuffer() != nullptr)
			{
				m_data = (const e_char*)file.getBuffer();
				m_own_data = false;
			}
			else
			{
				e_int32 size = (e_int32)m_file.size();
				e_char* data = (e_char*)m_allocator.allocate(size);
				m_own_data = true;
				file.read(data, m_data_size);
				m_data = data;
			}
			m_token = m_data;
			m_token_size = 0;
			deserializeToken();
		}
	}

	JsonSerializer::~JsonSerializer()
	{
		if (m_access_mode == READ && m_own_data)
		{
			m_allocator.deallocate((void*)m_data);
		}
	}

#pragma region serialization
	void JsonSerializer::serialize(const e_char* label, GameObject value)
	{
		serialize(label, value.index);
	}

	void JsonSerializer::serialize(const e_char* label, ComponentHandle value)
	{
		serialize(label, value.index);
	}

	void JsonSerializer::serialize(const e_char* label, e_uint32 value)
	{
		writeBlockComma();
		e_char tmp[20];
		writeString(label);
		StringUnitl::toCString(value, tmp, 20);
		m_file.write(" : ", StringUnitl::stringLength(" : "));
		m_file.write(tmp, StringUnitl::stringLength(tmp));
		m_is_first_in_block = false;
	}


	void JsonSerializer::serialize(const e_char* label, e_uint16 value)
	{
		writeBlockComma();
		e_char tmp[20];
		writeString(label);
		StringUnitl::toCString(value, tmp, 20);
		m_file.write(" : ", StringUnitl::stringLength(" : "));
		m_file.write(tmp, StringUnitl::stringLength(tmp));
		m_is_first_in_block = false;
	}


	void JsonSerializer::serialize(const e_char* label, e_float value)
	{
		writeBlockComma();
		e_char tmp[20];
		writeString(label);
		StringUnitl::toCString(value, tmp, 20, 8);
		m_file.write(" : ", StringUnitl::stringLength(" : "));
		m_file.write(tmp, StringUnitl::stringLength(tmp));
		m_is_first_in_block = false;
	}


	void JsonSerializer::serialize(const e_char* label, e_int32 value)
	{
		writeBlockComma();
		e_char tmp[20];
		writeString(label);
		StringUnitl::toCString(value, tmp, 20);
		m_file.write(" : ", StringUnitl::stringLength(" : "));
		m_file.write(tmp, StringUnitl::stringLength(tmp));
		m_is_first_in_block = false;
	}


	void JsonSerializer::serialize(const e_char* label, const ArchivePath& value)
	{
		writeBlockComma();
		writeString(label);
		m_file.write(" : \"", 4);
		m_file.write(value.c_str(), value.length());
		m_file.write("\"", 1);
		m_is_first_in_block = false;
	}


	void JsonSerializer::serialize(const e_char* label, const e_char* value)
	{
		writeBlockComma();
		writeString(label);
		m_file.write(" : \"", 4);
		if (value == nullptr)
		{
			m_file.write("", 1);
		}
		else
		{
			m_file.write(value, StringUnitl::stringLength(value));
		}
		m_file.write("\"", 1);
		m_is_first_in_block = false;
	}


	void JsonSerializer::serialize(const e_char* label, e_bool value)
	{
		writeBlockComma();
		writeString(label);
		m_file.write(value ? " : true" : " : false", value ? 7 : 8);
		m_is_first_in_block = false;
	}


	void JsonSerializer::beginObject()
	{
		writeBlockComma();
		m_file.write("{", 1);
		m_is_first_in_block = true;
	}


	void JsonSerializer::beginObject(const e_char* label)
	{
		writeBlockComma();
		writeString(label);
		m_file.write(" : {", 4);
		m_is_first_in_block = true;
	}

	void JsonSerializer::endObject()
	{
		m_file.write("}", 1);
		m_is_first_in_block = false;
	}


	void JsonSerializer::beginArray(const e_char* label)
	{
		writeBlockComma();
		writeString(label);
		m_file.write(" : [", 4);
		m_is_first_in_block = true;
	}


	void JsonSerializer::endArray()
	{
		m_file.write("]", 1);
		m_is_first_in_block = false;
	}

	void JsonSerializer::serializeArrayItem(const e_char* value)
	{
		writeBlockComma();
		writeString(value);
		m_is_first_in_block = false;
	}


	/*void JsonSerializer::serializeArrayItem(string& value)
	{
		writeBlockComma();
		writeString(value.c_str() ? value.c_str() : "");
		m_is_first_in_block = false;
	}*/


	void JsonSerializer::serializeArrayItem(e_uint32 value)
	{
		writeBlockComma();
		e_char tmp[20];
		StringUnitl::toCString(value, tmp, 20);
		m_file.write(tmp, StringUnitl::stringLength(tmp));
		m_is_first_in_block = false;
	}


	void JsonSerializer::serializeArrayItem(GameObject value)
	{
		serializeArrayItem(value.index);
	}


	void JsonSerializer::serializeArrayItem(ComponentHandle value)
	{
		serializeArrayItem(value.index);
	}


	void JsonSerializer::serializeArrayItem(e_int32 value)
	{
		writeBlockComma();
		e_char tmp[20];
		StringUnitl::toCString(value, tmp, 20);
		m_file.write(tmp, StringUnitl::stringLength(tmp));
		m_is_first_in_block = false;
	}


	void JsonSerializer::serializeArrayItem(e_int64 value)
	{
		writeBlockComma();
		e_char tmp[30];
		StringUnitl::toCString(value, tmp, 30);
		m_file.write(tmp, StringUnitl::stringLength(tmp));
		m_is_first_in_block = false;
	}


	void JsonSerializer::serializeArrayItem(e_float value)
	{
		writeBlockComma();
		e_char tmp[20];
		StringUnitl::toCString(value, tmp, 20, 8);
		m_file.write(tmp, StringUnitl::stringLength(tmp));
		m_is_first_in_block = false;
	}


	void JsonSerializer::serializeArrayItem(e_bool value)
	{
		writeBlockComma();
		m_file.write(value ? "true" : "false", value ? 4 : 5);
		m_is_first_in_block = false;
	}

#pragma endregion


#pragma region deserialization


	e_bool JsonSerializer::isNextBoolean() const
	{
		if (m_is_string_token) return false;
		if (m_token_size == 4 && StringUnitl::compareStringN(m_token, "true", 4) == 0) return true;
		if (m_token_size == 5 && StringUnitl::compareStringN(m_token, "false", 5) == 0) return true;
		return false;
	}


	void JsonSerializer::deserialize(const e_char* label, GameObject& value, GameObject default_value)
	{
		deserialize(label, value.index, default_value.index);
	}


	void JsonSerializer::deserialize(const e_char* label, ComponentHandle& value, ComponentHandle default_value)
	{
		deserialize(label, value.index, default_value.index);
	}


	void JsonSerializer::deserialize(e_bool& value, e_bool default_value)
	{
		value = !m_is_string_token ? m_token_size == 4 && (StringUnitl::compareStringN(m_token, "true", 4) == 0)
			: default_value;
		deserializeToken();
	}


	void JsonSerializer::deserialize(e_float& value, e_float default_value)
	{
		if (!m_is_string_token)
		{
			value = tokenToFloat();
		}
		else
		{
			value = default_value;
		}
		deserializeToken();
	}


	void JsonSerializer::deserialize(e_int32& value, e_int32 default_value)
	{
		if (m_is_string_token || !StringUnitl::fromCString(m_token, m_token_size, &value))
		{
			value = default_value;
		}
		deserializeToken();
	}


	void JsonSerializer::deserialize(const e_char* label, ArchivePath& value, const ArchivePath& default_value)
	{
		deserializeLabel(label);
		if (!m_is_string_token)
		{
			value = default_value;
		}
		else
		{
			e_char tmp[MAX_PATH_LENGTH];
			e_int32 size = Math::minimum(TlengthOf(tmp) - 1, m_token_size);
			StringUnitl::copyMemory(tmp, m_token, size);
			tmp[size] = '\0';
			value = tmp;
			deserializeToken();
		}
	}


	void JsonSerializer::deserialize(ArchivePath& value, const ArchivePath& default_value)
	{
		if (!m_is_string_token)
		{
			value = default_value;
		}
		else
		{
			e_char tmp[MAX_PATH_LENGTH];
			e_int32 size = Math::minimum(TlengthOf(tmp) - 1, m_token_size);
			StringUnitl::copyMemory(tmp, m_token, size);
			tmp[size] = '\0';
			value = tmp;
			deserializeToken();
		}
	}


	void JsonSerializer::deserialize(e_char* value, e_int32 max_length, const e_char* default_value)
	{
		if (!m_is_string_token)
		{
			StringUnitl::copyString(value, max_length, default_value);
		}
		else
		{
			e_int32 size = Math::minimum(max_length - 1, m_token_size);
			StringUnitl::copyMemory(value, m_token, size);
			value[size] = '\0';
			deserializeToken();
		}
	}


	void JsonSerializer::deserialize(const e_char* label, e_float& value, e_float default_value)
	{
		deserializeLabel(label);
		if (!m_is_string_token)
		{
			value = tokenToFloat();
			deserializeToken();
		}
		else
		{
			value = default_value;
		}
	}


	void JsonSerializer::deserialize(const e_char* label, e_uint32& value, e_uint32 default_value)
	{
		deserializeLabel(label);
		if (m_is_string_token || !StringUnitl::fromCString(m_token, m_token_size, &value))
		{
			value = default_value;
		}
		else
		{
			deserializeToken();
		}
	}


	void JsonSerializer::deserialize(const e_char* label, e_uint16& value, e_uint16 default_value)
	{
		deserializeLabel(label);
		if (m_is_string_token || !StringUnitl::fromCString(m_token, m_token_size, &value))
		{
			value = default_value;
		}
		else
		{
			deserializeToken();
		}
	}


	e_bool JsonSerializer::isObjectEnd()
	{
		if (m_token == m_data + m_data_size)
		{
			ErrorProxy(*this, "Unexpected end of file while looking for the end of an object.");
			return true;
		}

		return (!m_is_string_token && m_token_size == 1 && m_token[0] == '}');
	}


	void JsonSerializer::deserialize(const e_char* label, e_int32& value, e_int32 default_value)
	{
		deserializeLabel(label);
		if (m_is_string_token || !StringUnitl::fromCString(m_token, m_token_size, &value))
		{
			value = default_value;
		}
		deserializeToken();
	}


	void JsonSerializer::deserialize(const e_char* label,
		e_char* value,
		e_int32 max_length,
		const e_char* default_value)
	{
		deserializeLabel(label);
		if (!m_is_string_token)
		{
			StringUnitl::copyString(value, max_length, default_value);
		}
		else
		{
			e_int32 size = Math::minimum(max_length - 1, m_token_size);
			StringUnitl::copyMemory(value, m_token, size);
			value[size] = '\0';
			deserializeToken();
		}
	}


	void JsonSerializer::deserializeArrayBegin(const e_char* label)
	{
		deserializeLabel(label);
		expectToken('[');
		m_is_first_in_block = true;
		deserializeToken();
	}


	void JsonSerializer::expectToken(e_char expected_token)
	{
		if (m_is_string_token || m_token_size != 1 || m_token[0] != expected_token)
		{
			e_char tmp[2];
			tmp[0] = expected_token;
			tmp[1] = 0;
			ErrorProxy(*this, "Unexpected token + ");
			log_error(" %s, expected.", String(m_token, m_token_size, m_allocator));
			deserializeToken();
		}
	}


	void JsonSerializer::deserializeArrayBegin()
	{
		expectToken('[');
		m_is_first_in_block = true;
		deserializeToken();
	}


	void JsonSerializer::deserializeRawString(e_char* buffer, e_int32 max_length)
	{
		e_int32 size = Math::minimum(max_length - 1, m_token_size);
		StringUnitl::copyMemory(buffer, m_token, size);
		buffer[size] = '\0';
		deserializeToken();
	}


	void JsonSerializer::nextArrayItem()
	{
		if (!m_is_first_in_block)
		{
			expectToken(',');
			deserializeToken();
		}
	}


	e_bool JsonSerializer::isArrayEnd()
	{
		if (m_token == m_data + m_data_size)
		{
			ErrorProxy(*this, "Unexpected end of file while looking for the end of an array.");
			return true;
		}

		return (!m_is_string_token && m_token_size == 1 && m_token[0] == ']');
	}


	void JsonSerializer::deserializeArrayEnd()
	{
		expectToken(']');
		m_is_first_in_block = false;
		deserializeToken();
	}


	void JsonSerializer::deserializeArrayItem(e_char* value, e_int32 max_length, const e_char* default_value)
	{
		deserializeArrayComma();
		if (m_is_string_token)
		{
			e_int32 size = Math::minimum(max_length - 1, m_token_size);
			StringUnitl::copyMemory(value, m_token, size);
			value[size] = '\0';
			deserializeToken();
		}
		else
		{
			ErrorProxy(*this, "Unexpected token + ");
			log_error("%s,expected string.", String(m_token, m_token_size, m_allocator));
			deserializeToken();
			StringUnitl::copyString(value, max_length, default_value);
		}
	}


	void JsonSerializer::deserializeArrayItem(GameObject& value, GameObject default_value)
	{
		deserializeArrayItem(value.index, default_value.index);
	}


	void JsonSerializer::deserializeArrayItem(ComponentHandle& value, ComponentHandle default_value)
	{
		deserializeArrayItem(value.index, default_value.index);
	}


	void JsonSerializer::deserializeArrayItem(e_uint32& value, e_uint32 default_value)
	{
		deserializeArrayComma();
		if (m_is_string_token || !StringUnitl::fromCString(m_token, m_token_size, &value))
		{
			value = default_value;
		}
		deserializeToken();
	}


	void JsonSerializer::deserializeArrayItem(e_int32& value, e_int32 default_value)
	{
		deserializeArrayComma();
		if (m_is_string_token || !StringUnitl::fromCString(m_token, m_token_size, &value))
		{
			value = default_value;
		}
		deserializeToken();
	}


	void JsonSerializer::deserializeArrayItem(e_int64& value, e_int64 default_value)
	{
		deserializeArrayComma();
		if (m_is_string_token || !StringUnitl::fromCString(m_token, m_token_size, &value))
		{
			value = default_value;
		}
		deserializeToken();
	}


	void JsonSerializer::deserializeArrayItem(e_float& value, e_float default_value)
	{
		deserializeArrayComma();
		if (m_is_string_token)
		{
			value = default_value;
		}
		else
		{
			value = tokenToFloat();
		}
		deserializeToken();
	}


	void JsonSerializer::deserializeArrayItem(e_bool& value, e_bool default_value)
	{
		deserializeArrayComma();
		if (m_is_string_token)
		{
			value = default_value;
		}
		else
		{
			value = m_token_size == 4 && StringUnitl::compareStringN("true", m_token, m_token_size) == 0;
		}
		deserializeToken();
	}


	void JsonSerializer::deserialize(const e_char* label, e_bool& value, e_bool default_value)
	{
		deserializeLabel(label);
		if (!m_is_string_token)
		{
			value = m_token_size == 4 && StringUnitl::compareStringN("true", m_token, 4) == 0;
		}
		else
		{
			value = default_value;
		}
		deserializeToken();
	}


	static e_bool isDelimiter(e_char c)
	{
		return c == '\t' || c == '\n' || c == ' ' || c == '\r';
	}


	void JsonSerializer::deserializeArrayComma()
	{
		if (m_is_first_in_block)
		{
			m_is_first_in_block = false;
		}
		else
		{

			expectToken(',');
			deserializeToken();
		}
	}


	static e_bool isSingleCharToken(e_char c)
	{
		return c == ',' || c == '[' || c == ']' || c == '{' || c == '}' || c == ':';
	}


	void JsonSerializer::deserializeToken()
	{
		m_token += m_token_size;
		if (m_is_string_token)
		{
			++m_token;
		}

		while (m_token < m_data + m_data_size && isDelimiter(*m_token))
		{
			++m_token;
		}
		if (*m_token == '/' && m_token < m_data + m_data_size - 1 && m_token[1] == '/')
		{
			m_token_size = e_int32((m_data + m_data_size) - m_token);
			m_is_string_token = false;
		}
		else if (*m_token == '"')
		{
			++m_token;
			m_is_string_token = true;
			const e_char* token_end = m_token;
			while (token_end < m_data + m_data_size && *token_end != '"')
			{
				++token_end;
			}
			if (token_end == m_data + m_data_size)
			{
				ErrorProxy(*this, "Unexpected end of file while looking for.");
				m_token_size = 0;
			}
			else
			{
				m_token_size = e_int32(token_end - m_token);
			}
		}
		else if (isSingleCharToken(*m_token))
		{
			m_is_string_token = false;
			m_token_size = 1;
		}
		else
		{
			m_is_string_token = false;
			const e_char* token_end = m_token;
			while (token_end < m_data + m_data_size && !isDelimiter(*token_end) &&
				!isSingleCharToken(*token_end))
			{
				++token_end;
			}
			m_token_size = e_int32(token_end - m_token);
		}
	}


	void JsonSerializer::deserializeObjectBegin()
	{
		m_is_first_in_block = true;
		expectToken('{');
		deserializeToken();
	}

	void JsonSerializer::deserializeObjectEnd()
	{
		expectToken('}');
		m_is_first_in_block = false;
		deserializeToken();
	}


	void JsonSerializer::deserializeLabel(e_char* label, e_int32 max_length)
	{
		if (!m_is_first_in_block)
		{
			expectToken(',');
			deserializeToken();
		}
		else
		{
			m_is_first_in_block = false;
		}
		if (!m_is_string_token)
		{
			ErrorProxy(*this, "Unexpected token + ");
			log_error("%s, expected string.", String(m_token, m_token_size, m_allocator));
			deserializeToken();
		}
		StringUnitl::copyNString(label, max_length, m_token, m_token_size);
		deserializeToken();
		expectToken(':');
		deserializeToken();
	}


	void JsonSerializer::writeString(const e_char* str)
	{
		m_file.write("\"", 1);
		if (str)
		{
			m_file.write(str, StringUnitl::stringLength(str));
		}
		m_file.write("\"", 1);
	}


	void JsonSerializer::writeBlockComma()
	{
		if (!m_is_first_in_block)
		{
			m_file.write(",\n", 2);
		}
	}


	void JsonSerializer::deserializeLabel(const e_char* label)
	{
		if (!m_is_first_in_block)
		{
			expectToken(',');
			deserializeToken();
		}
		else
		{
			m_is_first_in_block = false;
		}
		if (!m_is_string_token)
		{
			ErrorProxy(*this, "Unexpected label + ");
			log_error("%s , expected.", String(m_token, m_token_size, m_allocator));
			deserializeToken();
		}
		if (StringUnitl::compareStringN(label, m_token, m_token_size) != 0)
		{
			ErrorProxy(*this, "Unexpected label + ");
			log_error("%s , expected %s", String(m_token, m_token_size, m_allocator), label);
			deserializeToken();
		}
		deserializeToken();
		if (m_is_string_token || m_token_size != 1 || m_token[0] != ':')
		{
			ErrorProxy(*this, "Unexpected label + ");
			log_error("%s , expected %s", String(m_token, m_token_size, m_allocator), label);
			deserializeToken();
		}
		deserializeToken();
	}
#pragma endregion

	e_float JsonSerializer::tokenToFloat()
	{
		e_char tmp[64];
		e_int32 size = Math::minimum((e_int32)sizeof(tmp) - 1, m_token_size);
		StringUnitl::copyMemory(tmp, m_token, size);
		tmp[size] = '\0';
		return (e_float)atof(tmp);
	}
}
