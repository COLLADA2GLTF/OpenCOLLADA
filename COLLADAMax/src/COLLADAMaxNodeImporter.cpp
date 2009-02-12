/*
Copyright (c) 2008 NetAllied Systems GmbH

This file is part of COLLADAMax.

Portions of the code are:
Copyright (c) 2005-2007 Feeling Software Inc.
Copyright (c) 2005-2007 Sony Computer Entertainment America

Based on the 3dsMax COLLADASW Tools:
Copyright (c) 2005-2006 Autodesk Media Entertainment

Licensed under the MIT Open Source License, 
for details please see LICENSE file or the website
http://www.opensource.org/licenses/mit-license.php
*/

#include "COLLADAMaxStableHeaders.h"
#include "COLLADAMaxNodeImporter.h"

#include "dummy.h"

namespace COLLADAMax
{

	NodeImporter::NodeImporter( DocumentImporter* documentImporter )
		:	ImporterBase(documentImporter)
	{}

	//------------------------------
	NodeImporter::~NodeImporter()
	{
	}


	//---------------------------------------------------------------
	void NodeImporter::Matrix4ToMaxMatrix3 ( Matrix3 & copy,  const COLLADABU::Math::Matrix4& original )
	{
		Point4 column;
		column[ 0 ] = (float)original.getElement(0,0);
		column[ 1 ] = (float)original.getElement(0,1);
		column[ 2 ] = (float)original.getElement(0,2);
		column[ 3 ] = (float)original.getElement(0,3);
		copy.SetColumn(0, column);

		column[ 0 ] = (float)original.getElement(1,0);
		column[ 1 ] = (float)original.getElement(1,1);
		column[ 2 ] = (float)original.getElement(1,2);
		column[ 3 ] = (float)original.getElement(1,3);
		copy.SetColumn(1, column);

		column[ 0 ] = (float)original.getElement(2,0);
		column[ 1 ] = (float)original.getElement(2,1);
		column[ 2 ] = (float)original.getElement(2,2);
		column[ 3 ] = (float)original.getElement(2,3);
		copy.SetColumn(2, column);
	}

	//------------------------------
	void NodeImporter::setNodeProperties( const COLLADAFW::Node* node, ImpNode* importNode)
	{
		const String& newNodeName = node->getName();
		if ( !newNodeName.empty() )
			importNode->SetName(newNodeName.c_str());

		// set transform
		COLLADABU::Math::Matrix4 transformationMatrix;
		node->getTransformationMatrix(transformationMatrix);
		Matrix3 maxTransformationMatrix;
		Matrix4ToMaxMatrix3(maxTransformationMatrix, transformationMatrix);
		importNode->SetTransform(0, maxTransformationMatrix);
	}

	//------------------------------
	ImpNode* NodeImporter::importNode( const COLLADAFW::Node* node, ImpNode* parentImportNode )
	{
		bool singleInstance = (node->getInstanceGeometries().getCount() +
			                           node->getInstanceCameras().getCount() ) == 1;
		ImpNode* newImportNode = 0;
		const COLLADAFW::UniqueId& nodeUniqueId = node->getUniqueId();

		if ( !singleInstance )
		{
			newImportNode = getMaxImportInterface()->CreateNode();

			setNodeProperties(node, newImportNode);
			getMaxImportInterface()->AddNodeToScene(newImportNode);

			RefResult res = newImportNode->Reference(getDummyObject());

			importInstanceGeometries(node->getInstanceGeometries(), newImportNode);
			importInstanceCameras(node->getInstanceCameras(), newImportNode);
			importNodes(node->getChildNodes(), newImportNode);
		}
		else
		{
			newImportNode = importInstanceGeometry( node, parentImportNode );
			if ( !newImportNode )
				newImportNode = importInstanceCamera( node, parentImportNode );

			assert(newImportNode);
			importNodes(node->getChildNodes(), newImportNode);
		}

		// Append all nodes that are referenced by this node.
		importInstanceNodes(node->getInstanceNodes(), newImportNode);

		/** Store the unique id of the created node, to resolve references, when ever necessary.*/ 
		addUniqueIdINodePair( nodeUniqueId, newImportNode->GetINode());

		INode* childNode = newImportNode->GetINode();
		INode* parentNode = parentImportNode->GetINode();
		parentNode->AttachChild(childNode, FALSE);

		/* if there are nodes that reference the just created node, clone this node
		and append it to the referencing node.*/
		ImpNode* referencingImpNode = 0;
		while ( referencingImpNode = getReferencingImpNodesByUniqueId(nodeUniqueId) )
		{
			removeUniqueIdReferencingImpNodePair( nodeUniqueId, referencingImpNode);
			recursivelyCloneINode( referencingImpNode, newImportNode->GetINode() );
		}

		return newImportNode;
	}

	//------------------------------
	bool NodeImporter::importNodes( const COLLADAFW::NodePointerArray& nodeArray, ImpNode* parentImportNode )
	{
		for ( size_t i = 0, count = nodeArray.getCount(); i < count; ++i)
		{
			ImpNode* newNode = importNode(nodeArray[i], parentImportNode);
		}

		return true;
	}

	//------------------------------
	template<class Instance, 
		     void (NodeImporter::*postProcess)( INode*, Instance* )>
	bool NodeImporter::importInstances( const COLLADAFW::PointerArray<Instance>& instanceArray, ImpNode* parentImportNode )
	{
		for ( size_t i = 0, count = instanceArray.getCount(); i < count; ++i)
		{
			Instance* instance = instanceArray[i];

			ImpNode* newImportNode = getMaxImportInterface()->CreateNode();
			INode* newNode = newImportNode->GetINode();
			const COLLADAFW::UniqueId& uniqueId = instance->getInstanciatedObjectId();

			Object* object = getObjectByUniqueId(uniqueId);
			if ( object )
			{
				newImportNode->Reference(object);
			}
			else
			{
				newImportNode->Reference( getDummyObject() );
			}
			const COLLADAFW::UniqueId& instanceUniqueId = instance->getInstanciatedObjectId();
			// Store mapping between unique ids and nodes referencing the corresponding object.
			// Used to clone nodes
			addObjectINodeUniqueIdPair(newNode, instanceUniqueId);
			// Used to resolve instancing of objects
			addUniqueIdObjectINodePair(instanceUniqueId, newNode);

			INode* parentNode = parentImportNode->GetINode();
			parentNode->AttachChild(newNode, FALSE);

			// post process the creation
			if ( postProcess )
				(this->*postProcess)(newNode, instance);
		}

		return true;
	}


	//------------------------------
	bool NodeImporter::importInstanceGeometries( const COLLADAFW::InstanceGeometryPointerArray& instanceGeometryArray, ImpNode* parentImportNode )
	{
		return importInstances<COLLADAFW::InstanceGeometry, &NodeImporter::storeMaterialBindings>(instanceGeometryArray, parentImportNode);
	}


	//------------------------------
	bool NodeImporter::importInstanceCameras( const COLLADAFW::InstanceCameraPointerArray& instanceCameraArray, ImpNode* parentImportNode )
	{
		return importInstances<COLLADAFW::InstanceCamera, 0>(instanceCameraArray, parentImportNode);
	}

	//------------------------------
	template<class Instance, 
		     const COLLADAFW::PointerArray<Instance>& (COLLADAFW::Node::*getInstances)()const,
			 void (NodeImporter::*postProcess)( INode*, Instance* )>
	ImpNode* NodeImporter::importInstance( const COLLADAFW::Node* node, ImpNode* parentImportNode )
	{
		const COLLADAFW::PointerArray<Instance>& instances = (node->*getInstances)();
		if ( instances.getCount() != 1 )
			return 0;

		ImpNode* newImportNode = getMaxImportInterface()->CreateNode();
		setNodeProperties(node, newImportNode);
		INode* newNode = newImportNode->GetINode();

		Instance* instanceGeometry = instances[0];
		const COLLADAFW::UniqueId& uniqueId = instanceGeometry->getInstanciatedObjectId();

		Object* object = getObjectByUniqueId(uniqueId);
		if ( object )
		{
			newImportNode->Reference(object);
			const String& objectName = getObjectNameByObject(object);

			if ( node->getName().empty() && !objectName.empty() )
				newImportNode->SetName( objectName.c_str() );
		}
		else
		{
			newImportNode->Reference( getDummyObject() );
		}

		const COLLADAFW::UniqueId& instanceGeometryUniqueId = instanceGeometry->getInstanciatedObjectId();
		// Store mapping between unique ids and nodes referencing the corresponding object.
		// Used to clone nodes
		addObjectINodeUniqueIdPair(newNode, instanceGeometryUniqueId);
		// Used to resolve instancing of objects
		addUniqueIdObjectINodePair(instanceGeometryUniqueId, newNode);
		INode* parentNode = parentImportNode->GetINode();
		parentNode->AttachChild(newNode, FALSE);

		// post process the creation
		if ( postProcess )
			(this->*postProcess)(newNode, instanceGeometry);

		return newImportNode;
	}

	//------------------------------
	ImpNode* NodeImporter::importInstanceGeometry( const COLLADAFW::Node* node, ImpNode* parentImportNode )
	{
		return importInstance<COLLADAFW::InstanceGeometry, &COLLADAFW::Node::getInstanceGeometries, &NodeImporter::storeMaterialBindings>(node, parentImportNode);
	}


	//------------------------------
	ImpNode* NodeImporter::importInstanceCamera( const COLLADAFW::Node* node, ImpNode* parentImportNode )
	{
		return importInstance<COLLADAFW::InstanceCamera, &COLLADAFW::Node::getInstanceCameras, 0>(node, parentImportNode);
	}

	//------------------------------
	bool NodeImporter::importInstanceNodes( const COLLADAFW::InstanceNodePointerArray& instanceNodeArray, ImpNode* parentImportNode )
	{
		for ( size_t i = 0, count = instanceNodeArray.getCount(); i < count; ++i)
		{
			COLLADAFW::InstanceNode* instanceNode = instanceNodeArray[i];

			const COLLADAFW::UniqueId& uniqueId = instanceNode->getInstanciatedObjectId();

			INode* instanciatedINode = getINodeByUniqueId(uniqueId);
			// check if the referenced node is already in the max scene graph
			if ( instanciatedINode )
			{
				if ( !recursivelyCloneINode(parentImportNode, instanciatedINode) )
					return false;;
			}
			else
			{
				const COLLADAFW::Node* instanciatedFWNode = getFWNodeByUniqueId( uniqueId );
				// check if the referenced node is in one of the already received library nodes
				if ( instanciatedFWNode )
				{
					importNode( instanciatedFWNode, parentImportNode );
				}
				else
				{
					// If the referenced node has not been imported, store which node is referenced
					// to clone the nodes as the referenced nodes gets imported
					addUniqueIdReferencingImpNodePair(instanceNode->getInstanciatedObjectId(), parentImportNode );
				}
			}
		}

		return true;
	}

	//------------------------------
	bool NodeImporter::recursivelyCloneINode( ImpNode* parentImportNode, INode* nodeToClone )
	{
		ImpNode* newImportNode = getMaxImportInterface()->CreateNode();
		getMaxImportInterface()->AddNodeToScene(newImportNode);

		INode* newNode = newImportNode->GetINode();

		Object* object = nodeToClone->GetObjectRef();
		newImportNode->Reference(object);
		newNode->SetTMController(nodeToClone->GetTMController());
		newImportNode->SetName(nodeToClone->GetName());
		addClonedINodeOriginalINodePair(newNode, nodeToClone);

		INode* parentNode = parentImportNode->GetINode();
		parentNode->AttachChild(newNode, TRUE);

		/* If the node to clone references an object, the cloned one must references the same object.*/
		COLLADAFW::UniqueId id = getUniqueIdByObjectINode(nodeToClone);
		if ( id.isValid() )
		{
			addUniqueIdObjectINodePair(id, newNode);
			addObjectINodeUniqueIdPair(newNode, id);
		}

		// Clone the children 
		for ( int i = 0, count = nodeToClone->NumberOfChildren(); i < count; ++i)
			recursivelyCloneINode(newImportNode, nodeToClone->GetChildNode(i));

		return true;
	}

	//------------------------------
	void NodeImporter::storeMaterialBindings( INode* node, COLLADAFW::InstanceGeometry* instanceGeometry )
	{
		COLLADAFW::InstanceGeometry::MaterialBindingArray& materialBindings = instanceGeometry->getMaterialBindings();
		if ( !materialBindings.empty() )
		{
			DocumentImporter::NodeMaterialBindingsPair& materialBindingsPair = createAndAddNodeMaterialBindingsPair(node);
			size_t bindingsCount = materialBindings.getCount();
			materialBindingsPair.materialBindings.reserve(bindingsCount);
			for ( size_t i = 0; i < bindingsCount; ++i)
				materialBindingsPair.materialBindings.push_back(materialBindings[i]);
		}
	}


} // namespace COLLADAMax