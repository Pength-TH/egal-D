#include "common/animation/offline/fbx/fbx_skeleton.h"
#include "common/animation/offline/raw_skeleton.h"
#include "common/utils/logger.h"
namespace egal
{
	namespace animation
	{
		namespace offline
		{
			namespace fbx
			{
				namespace
				{
					enum RecurseReturn
					{
						kError, kSkeletonFound, kNoSkeleton
					};

					RecurseReturn RecurseNode(FbxNode* _node, FbxSystemConverter* _converter,
						RawSkeleton* _skeleton, RawSkeleton::Joint* _parent,
						int _depth)
					{
						bool skeleton_found = false;
						RawSkeleton::Joint* this_joint = NULL;

						bool process_node = false;

						// Push this node if it's below a skeleton root (aka has a parent).
						process_node |= _parent != NULL;

						// Push this node as a new joint if it has a joint compatible attribute.
						FbxNodeAttribute* node_attribute = _node->GetNodeAttribute();
						process_node |= node_attribute && node_attribute->GetAttributeType() ==
							FbxNodeAttribute::eSkeleton;

						// Process node if required.
						if (process_node)
						{
							skeleton_found = true;

							RawSkeleton::Joint::Children* sibling = NULL;
							if (_parent)
							{
								sibling = &_parent->children;
							}
							else
							{
								sibling = &_skeleton->roots;
							}

							// Adds a new child.
							sibling->resize(sibling->size() + 1);
							this_joint = &sibling->back();  // Will not be resized inside recursion.
							this_joint->name = _node->GetName();

							log_info(this_joint->name.c_str());

							// Extract bind pose.
							const FbxAMatrix matrix = _parent ? _node->EvaluateLocalTransform()
								: _node->EvaluateGlobalTransform();
							if (!_converter->ConvertTransform(matrix, &this_joint->transform))
							{
								log_error("Failed to extract skeleton transform for joint %s.", this_joint->name);
								return kError;
							}

							// One level deeper in the hierarchy.
							_depth++;
						}

						// Iterate node's children.
						for (int i = 0; i < _node->GetChildCount(); i++)
						{
							FbxNode* child = _node->GetChild(i);
							const RecurseReturn ret =
								RecurseNode(child, _converter, _skeleton, this_joint, _depth);
							if (ret == kError)
							{
								return ret;
							}
							skeleton_found |= (ret == kSkeletonFound);
						}

						return skeleton_found ? kSkeletonFound : kNoSkeleton;
					}
				}  // namespace

				bool ExtractSkeleton(FbxSceneLoader& _loader, RawSkeleton* _skeleton)
				{
					RecurseReturn ret = RecurseNode(_loader.scene()->GetRootNode(),
						_loader.converter(), _skeleton, NULL, 0);
					if (ret == kNoSkeleton)
					{
						log_info("No skeleton found in Fbx scene.");
						return false;
					}
					else if (ret == kError)
					{
						log_error("Failed to extract skeleton.");
						return false;
					}
					return true;
				}
			}  // namespace fbx
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
