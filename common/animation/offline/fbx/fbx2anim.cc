
#include "common/animation/offline/tools/convert2anim.h"

#include "common/animation/offline/fbx/fbx.h"

// fbx2anim is a command line tool that converts an animation imported from a
// fbx document to egal runtime format.
//
// fbx2anim extracts animated joints from a fbx document. Only the animated
// joints whose names match those of the egal runtime skeleton given as argument
// are selected. Keyframes are then optimized, based on command line settings,
// and serialized as a runtime animation to an egal binary archive.
//
// Use fbx2anim integrated help command (fbx2anim --help) for more details
// about available arguments.

class FbxAnimationConverter
	: public egal::animation::offline::AnimationConverter
{
private:
	// Implement SkeletonConverter::Import function.
	virtual bool Import(const char* _filename,
		const egal::animation::Skeleton& _skeleton,
		float _sampling_rate, Animations* _animations)
	{
		return egal::animation::offline::fbx::ImportFromFile(
			_filename, _skeleton, _sampling_rate, _animations);
	}
};

int main(int _argc, const char** _argv)
{
	FbxAnimationConverter converter;
	return converter(_argc, _argv);
}
