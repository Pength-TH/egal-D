#include "common/animation/offline/tools/convert2skel.h"

#include "common/animation/offline/fbx/fbx.h"

// fbx2skel is a command line tool that converts a skeleton imported from a
// Fbx document to egal runtime format.
//
// fbx2skel extracts the skeleton from the Fbx document. It then builds an
// egal runtime skeleton, from the Fbx skeleton, and serializes it to a egal
// binary archive.
//
// Use fbx2skel integrated help command (fbx2skel --help) for more details
// about available arguments.

class FbxSkeletonConverter : public egal::animation::offline::SkeletonConverter
{
private:
	// Implement SkeletonConverter::Import function.
	virtual bool Import(const char* _filename,
		egal::animation::offline::RawSkeleton* _skeleton)
	{
		return egal::animation::offline::fbx::ImportFromFile(_filename, _skeleton);
	}
};

int main(int _argc, const char** _argv)
{
	FbxSkeletonConverter converter;
	return converter(_argc, _argv);
}
