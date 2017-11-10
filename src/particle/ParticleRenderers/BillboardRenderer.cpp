/*
-----------------------------------------------------------------------------------------------
This source file is part of the Particle Universe product.

Copyright (c) 2012 Henry van Merode

Usage of this program is licensed under the terms of the Particle Universe Commercial License.
You can find a copy of the Commercial License in the Particle Universe package.
-----------------------------------------------------------------------------------------------
*/

#include "ParticleUniversePCH.h"

#ifndef PARTICLE_UNIVERSE_EXPORTS
#define PARTICLE_UNIVERSE_EXPORTS
#endif

#include "ParticleRenderers/ParticleUniverseBillboardRenderer.h"
#include "ParticleRenderers/ParticleUniverseBillboard.h"
#include "OgreNode.h"
#include "OgreSceneNode.h"
#include "OgreSceneManager.h"


#include "seed_memory_leak_check.h" //the last include head file,no include after this line
namespace ParticleUniverse
{
	// Constants
	const BillboardRenderer::BillboardType BillboardRenderer::DEFAULT_BILLBOARD_TYPE = BillboardRenderer::BBT_POINT;
	const bool BillboardRenderer::DEFAULT_ACCURATE_FACING = false;
	const Ogre::BillboardOrigin BillboardRenderer::DEFAULT_ORIGIN = Ogre::BBO_CENTER;
	const Ogre::BillboardRotationType BillboardRenderer::DEFAULT_ROTATION_TYPE = Ogre::BBR_TEXCOORD;
	const Vector3 BillboardRenderer::DEFAULT_COMMON_DIRECTION(0, 0, 1);
	const Vector3 BillboardRenderer::DEFAULT_COMMON_UP_VECTOR(0, 1, 0);
	const bool BillboardRenderer::DEFAULT_POINT_RENDERING = false;
	const bool BillboardRenderer::DEFAULT_COMMON_DIRECTION_LOCAL = true;
	const bool BillboardRenderer::DEFAULT_COMMON_UP_VECTOR_LOCAL = true;
	//-----------------------------------------------------------------------
	BillboardRenderer::BillboardRenderer(void) : 
		ParticleRenderer(),
		mBillboardType(DEFAULT_BILLBOARD_TYPE),
		mCommonDirection(DEFAULT_COMMON_DIRECTION),
		mCommonUpVector(DEFAULT_COMMON_UP_VECTOR),
		mCommonDirectionLocal(DEFAULT_COMMON_DIRECTION_LOCAL),
		mUpVectorLocal(DEFAULT_COMMON_UP_VECTOR_LOCAL)
	{
		mBillboardSet = PU_NEW Ogre::BillboardSet("", 0, true);
		mBillboardSet->setBillboardsInWorldSpace(true);
		autoRotate = false;
		m_pNode = NULL;
	}
	//-----------------------------------------------------------------------
	BillboardRenderer::~BillboardRenderer(void)
	{
		if(m_pNode)
		{
			Ogre::SceneNode* pNode = static_cast<Ogre::SceneNode*>(m_pNode);
			if(pNode && pNode->getCreator())
			{
				pNode->getCreator()->destroySceneNode(pNode);
				m_pNode = NULL;
			}
		}
		if (mBillboardSet)
		{
			PU_DELETE mBillboardSet;
			mBillboardSet = 0;
			m_pNode = NULL;
		}
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_prepare(ParticleTechnique* technique)
	{
		// Use the given technique, although it should be the same as mParentTechnique (must be set already)
		if (!technique || mRendererInitialised)
			return;

		_notifyParticleQuota(technique->getVisualParticleQuota());

		// Notify attached, but only if the parentnode exists
		if (technique->getParentSystem()->getParentNode())
		{		
			if(m_pNode)
			{
				technique->getParentSystem()->getParentNode()->addChild(m_pNode);
				m_pNode->setPosition(technique->getDerivedPosition());
				_notifyAttached(m_pNode, technique->getParentSystem()->isParentIsTagPoint());
			}
			else
			{
				m_pNode = technique->getParentSystem()->getParentNode()->createChild();
				if(m_pNode)
				{
					m_pNode->setPosition(technique->getDerivedPosition());
					_notifyAttached(m_pNode, technique->getParentSystem()->isParentIsTagPoint());
				}
				else
					_notifyAttached(technique->getParentSystem()->getParentNode(), technique->getParentSystem()->isParentIsTagPoint());
			}
		}

		_notifyDefaultDimensions(_mRendererScale.x * technique->getDefaultWidth(),
								_mRendererScale.y * technique->getDefaultHeight(),
								_mRendererScale.z * technique->getDefaultDepth());
		_setMaterialName(technique->getMaterialName());
		mBillboardSet->setRenderQueueGroup(mQueueId);
		
		// Set the texture coordinates (if used)
		if (mTextureCoordsSet)
		{
			mBillboardSet->setTextureCoords(*mUVList.begin(), static_cast<uint16>(mUVList.size()));
		}
		else if (mTextureCoordsRowsAndColumnsSet)
		{
			mBillboardSet->setTextureStacksAndSlices(mTextureCoordsRows, mTextureCoordsColumns);
		}
		
		mRendererInitialised = true;
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_unprepare(ParticleTechnique* technique)
	{
		_notifyAttached(0); // Bugfix V1.5: If detached from scenenode, do not use the pointer to it
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setBillboardType(BillboardType bbt)
	{
		mBillboardType = bbt;

		// Because BBT_ORIENTED_SHAPE is unknown to Ogre, this switch is needed
		switch (bbt)
		{
			case BBT_POINT:
				mBillboardSet->setBillboardType(Ogre::BBT_POINT);
				break;

			case BBT_ORIENTED_COMMON:
				mBillboardSet->setBillboardType(Ogre::BBT_ORIENTED_COMMON);
				break;

			case BBT_ORIENTED_SELF:
				mBillboardSet->setBillboardType(Ogre::BBT_ORIENTED_SELF);
				break;

			case BBT_ORIENTED_SHAPE:
				mBillboardSet->setBillboardType(Ogre::BBT_ORIENTED_SELF);
				break;

			case BBT_PERPENDICULAR_COMMON:
				mBillboardSet->setBillboardType(Ogre::BBT_PERPENDICULAR_COMMON);
				break;

			case BBT_PERPENDICULAR_SELF:
				mBillboardSet->setBillboardType(Ogre::BBT_PERPENDICULAR_SELF);
				break;
		}
	}
	//-----------------------------------------------------------------------
	BillboardRenderer::BillboardType BillboardRenderer::getBillboardType(void) const
	{
		return mBillboardType;
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setUseAccurateFacing(bool acc)
	{
		mBillboardSet->setUseAccurateFacing(acc);
	}
	//-----------------------------------------------------------------------
	bool BillboardRenderer::isUseAccurateFacing(void) const
	{
		return mBillboardSet->getUseAccurateFacing();
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setBillboardRotationType(Ogre::BillboardRotationType rotationType)
	{
		mBillboardSet->setBillboardRotationType(rotationType);
	}
	//-----------------------------------------------------------------------
	Ogre::BillboardRotationType BillboardRenderer::getBillboardRotationType(void) const
	{
		return mBillboardSet->getBillboardRotationType();
	}
    //-----------------------------------------------------------------------
	void BillboardRenderer::setCommonDirection(const Vector3& vec)
	{
		mCommonDirection = vec;
		mCommonDirection.normalise();
		mBillboardSet->setCommonDirection(mCommonDirection);
	}
	//-----------------------------------------------------------------------
	const Vector3& BillboardRenderer::getCommonDirection(void) const
	{
		return mBillboardSet->getCommonDirection();
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setCommonUpVector(const Vector3& vec)
	{
		mCommonUpVector = vec;
		mCommonUpVector.normalise();
		mBillboardSet->setCommonUpVector(mCommonUpVector);
	}
	//-----------------------------------------------------------------------
	const Vector3& BillboardRenderer::getCommonUpVector(void) const
	{
		return mCommonUpVector;//mBillboardSet->getCommonUpVector();
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setCommonDirectionLocal(bool bLocal)
	{
		mCommonDirectionLocal = bLocal;
	}
	//-----------------------------------------------------------------------
	const bool& BillboardRenderer::getCommonDirectionLocal(void) const
	{
		return mCommonDirectionLocal;
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setUpVectorLocal(bool bLocal)
	{
		mUpVectorLocal = bLocal;
	}
	//-----------------------------------------------------------------------
	const bool& BillboardRenderer::getUpVectorLocal(void) const
	{
		return mUpVectorLocal;
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setPointRenderingEnabled(bool enabled)
	{
		mBillboardSet->setPointRenderingEnabled(enabled);
	}
	//-----------------------------------------------------------------------
	bool BillboardRenderer::isPointRenderingEnabled(void) const
	{
		return mBillboardSet->isPointRenderingEnabled();
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_updateRenderQueue(Ogre::RenderQueue* queue, ParticlePool* pool)
	{
		// Always perform this one
		ParticleRenderer::_updateRenderQueue(queue, pool);

		if (!mVisible)
			return;

		// Fast check to determine whether there are visual particles
		if (pool->isEmpty(Particle::PT_VISUAL))
			return;

		if (mUpVectorLocal || mCommonDirectionLocal)
		{	
			mParentTechnique->setDoubleRender(true);
			Quaternion quaCurrent = getParentTechnique()->getParentSystem()->getDerivedOrientation();
			if (mCommonDirectionLocal)
				mBillboardSet->setCommonDirection(quaCurrent * mCommonDirection);
			if (mUpVectorLocal)
				mBillboardSet->setCommonUpVector(quaCurrent * mCommonUpVector);
		}

		mBillboardSet->setCullIndividually(mCullIndividual);

		mBillboardSet->beginBillboards(pool->getSize(Particle::PT_VISUAL));
		Billboard bb; // This is the Particle Universe Billboard and not the Ogre Billboard
		
		VisualParticle* particle = static_cast<VisualParticle*>(pool->getFirst(Particle::PT_VISUAL));
		while (!pool->end(Particle::PT_VISUAL))
		{
			if (particle)
			{
				bb.mPosition = particle->position;

				if (mBillboardType == BBT_ORIENTED_SELF || mBillboardType == BBT_PERPENDICULAR_SELF)
				{
					// Normalise direction vector
					bb.mDirection = particle->direction;
					bb.mDirection.normalise();
				}
				else if (mBillboardType == BBT_ORIENTED_SHAPE)
				{
					// Normalise orientation vector
					bb.mDirection = Vector3(particle->orientation.x, particle->orientation.y, particle->orientation.z);
					bb.mDirection.normalise();
				}
					
				bb.mColour = particle->colour;
				bb.mRotation = particle->zRotation; // Use rotation around the Z-axis (2D rotation)

				if (bb.mOwnDimensions = particle->ownDimensions)
				{
					bb.mOwnDimensions = true;
					bb.mWidth = particle->width;
					bb.mHeight = particle->height;
				}
				float texNum = particle->textureCoordsCurrent >= getNumTextureCoords() ? getNumTextureCoords() - 1 : particle->textureCoordsCurrent;
				// PU 1.4: No validation on max. texture coordinate because of performance reasons.
				bb.setTexcoordIndex(texNum);
					
				mBillboardSet->injectBillboard(bb);
			}
			
			particle = static_cast<VisualParticle*>(pool->getNext(Particle::PT_VISUAL));
		}

        mBillboardSet->endBillboards();

		// Update the queue
		mBillboardSet->_updateRenderQueue(queue);
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_notifyAttached(Ogre::Node* parent, bool isTagPoint)
	{
		if(!parent && m_pNode && m_pNode->getParent())
			m_pNode->getParent()->removeChild(m_pNode);
		
		if(parent && m_pNode && !m_pNode->getParent())
		{
			parent->addChild(m_pNode);
			mBillboardSet->_notifyAttached(m_pNode, isTagPoint);
		}
		else
			mBillboardSet->_notifyAttached(parent, isTagPoint);
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_setMaterialName(const String& materialName)
	{
		mBillboardSet->setMaterialName(materialName);
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_notifyCurrentCamera(Camera* cam)
	{
		mBillboardSet->_notifyCurrentCamera(cam);
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_notifyParticleQuota(size_t quota)
	{
		mBillboardSet->setPoolSize(static_cast<unsigned int>(quota));
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_notifyDefaultDimensions(Real width, Real height, Real depth)
	{
		// Ignore the depth, because the billboard is flat.
		mBillboardSet->setDefaultDimensions(width, height);
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_notifyParticleResized(void)
	{
		mBillboardSet->_notifyBillboardResized();
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_notifyParticleZRotated(void)
	{
		mBillboardSet->_notifyBillboardRotated();
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setRenderQueueGroup(uint8 queueId)
	{
		mQueueId = queueId;
		mBillboardSet->setRenderQueueGroup(mQueueId);
	}
	//-----------------------------------------------------------------------
	SortMode BillboardRenderer::_getSortMode(void) const
	{
		return mBillboardSet->_getSortMode();
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setVisible(bool visible)
	{
		ParticleRenderer::setVisible(visible);
		mBillboardSet->setVisible(visible);
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::copyAttributesTo (ParticleRenderer* renderer)
	{
		// First copy parent attributes
		ParticleRenderer::copyAttributesTo(renderer);

		// First cast to BillboardRenderer
		BillboardRenderer* billboardRenderer = static_cast<BillboardRenderer*>(renderer);

		// Copy attributes in case there is a billboardset (which should be available)
		if (!billboardRenderer->getBillboardSet())
			return;

		billboardRenderer->setBillboardType(getBillboardType());
		billboardRenderer->setBillboardOrigin(getBillboardOrigin());
		billboardRenderer->setBillboardRotationType(getBillboardRotationType());
		billboardRenderer->setCommonDirection(getCommonDirection());
		billboardRenderer->setCommonUpVector(getCommonUpVector());
		billboardRenderer->setPointRenderingEnabled(isPointRenderingEnabled());
		billboardRenderer->setUseAccurateFacing(isUseAccurateFacing());

		billboardRenderer->setCommonDirectionLocal(mCommonDirectionLocal);
		billboardRenderer->setUpVectorLocal(mUpVectorLocal);
	}

}
