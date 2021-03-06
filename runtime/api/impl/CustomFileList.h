#ifndef _CUSTOM_FILE_LIST_H_
#define _CUSTOM_FILE_LIST_H_

#include "runtime/api/IFileSystem.h"

#if _MSC_VER == 1900
#define C_CONST const
#else
#define C_CONST
#endif

#if !DISABLE_FILE_AND_UI

//............................................................................................
class FileArchive : public Ogre::Archive
{
public:
	FileArchive(const Ogre::String& name, const Ogre::String& archType);
	~FileArchive();

public:
	virtual bool isCaseSensitive(void) const { return false; }
	virtual void load();
	virtual void unload();
	virtual Ogre::DataStreamPtr open(const String& filename, bool readOnly = true) const;
	virtual Ogre::DataStreamPtr create(const String& filename) const;
	virtual void remove(const String& filename) const;
	
	virtual Ogre::StringVectorPtr list(bool recursive = true, bool dirs = false) C_CONST;
	virtual Ogre::FileInfoListPtr listFileInfo(bool recursive = true, bool dirs = false) C_CONST;
	virtual Ogre::StringVectorPtr find(const String& pattern, bool recursive = true, bool dirs = false) C_CONST;
	virtual Ogre::FileInfoListPtr findFileInfo(const String& pattern, bool recursive = true, bool dirs = false) C_CONST;
	virtual bool exists(const String& filename) C_CONST;
	virtual time_t getModifiedTime(const String& filename) C_CONST;

	void AddFileToSizeMap(const char* name, unsigned int val)
	{
		m_FileToSizeMap[name] = val;
	}
public:
	typedef Ogre::map<String, unsigned int>::type	FileToSizeMap;
	FileToSizeMap m_FileToSizeMap;
	OGRE_AUTO_MUTEX;
};
//............................................................................................
//............................................................................................
class FileArchiveFactory : public Ogre::ArchiveFactory
{
public:
	FileArchiveFactory();
	virtual ~FileArchiveFactory();

	const String& getType(void) const;
	Ogre::Archive *createInstance(const String& name);
	Ogre::Archive *createInstance(const String& name, bool readOnly);
	void destroyInstance(Ogre::Archive* arch);
};
//............................................................................................
//............................................................................................
class _OgrePrivate FileDataStream : public Ogre::DataStream
{
public:
	FileDataStream(IReadFile* pReadFile);
	FileDataStream(const String& name, IReadFile* pReadFile);
	virtual ~FileDataStream();

	// overwrite method
public:
	virtual size_t read(void* buf, size_t count);
	virtual size_t write(void* buf, size_t count);
	virtual void skip(long count);
	virtual void seek( size_t pos );
	virtual size_t tell(void) const;
	virtual bool canClone(void) const;
	virtual bool eof(void) const;
	virtual void close(void);
	virtual Ogre::uchar* getPtr(void);

protected:
	IReadFile* m_pReadFile;
};
//............................................................................................
//............................................................................................
class SGOgreImageFile : public IReadFile
{
public:
	SGOgreImageFile();
	virtual ~SGOgreImageFile();

	BOOL Init(Ogre::Image* pImage);

	// overwrite method
public:
	virtual bool IsInPackage();
	virtual bool IsBufferWrite();
	virtual bool IsPreparing();
	virtual bool IsPrepareSuccess();
	virtual bool IsPrepareFailed();

	virtual unsigned int GetVersion();
	virtual int GetPrepareSize();
	virtual int GetSize();
	virtual void* GetBuffer();

	virtual bool Read(void* pBuffer, int nSize);
	virtual bool Seek(int nOffset, int nOrigin);
	virtual int Tell();
	virtual void Close();

public:
	Ogre::Image* m_pImage;
	unsigned char m_uchBuffer[12];
	int m_nCurrentReadPosition;
};

#endif

#endif
