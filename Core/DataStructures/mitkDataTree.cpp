#include "DataTree.h"


//##ModelId=3E38F46A0190

mitk::DataTree::DataTree() : 

    DataTreeBase( )

{

	addTreeChangeListener( this );
}





//##ModelId=3E38F46A01AE

mitk::DataTree::~DataTree()

{

}



/**

 *

 */

//##ModelId=3E3FE0430148

TreeIterator<mitk::DataTreeNode::Pointer>* mitk::DataTree::GetNext( const char* propertyKey, const mitk::BaseProperty* property,  TreeIterator<mitk::DataTreeNode::Pointer>* startPosition ){



	if ( startPosition == NULL )

		startPosition = preorderIterator();



	TreeIterator<mitk::DataTreeNode::Pointer>* pos = startPosition->clone();

	mitk::DataTreeNode::Pointer dtn;

	



	while ( pos->hasNext() ) {

	

		dtn = pos->next();

		mitk::PropertyList::Pointer propertyList = dtn->GetPropertyList();



		if ( property == propertyList->GetProperty( propertyKey ) )

			return pos;			

	}

	

	delete pos;

	return NULL;

}



/**

 *

 */

//##ModelId=3EA6ADB7029F

void mitk::DataTree::treeChanged( TreeIterator<DataTreeNode::Pointer>& changedTreePosition ) {



	Modified();

}

#if (_MSC_VER > 1200) || !defined(_MSC_VER)
MBI_STD::istream& mitk::operator>>( MBI_STD::istream& i, mitk::DataTreeNode::Pointer& dtn ) {

	dtn = mitk::DataTreeNode::New();
   //i >> av.get();
   return i;
}
#endif

#if ((defined(_MSC_VER)) && (_MSC_VER <= 1200))
MBI_STD::istream& operator>>( MBI_STD::istream& i, mitk::DataTreeNode::Pointer& dtn ) 
{

	dtn = mitk::DataTreeNode::New();
   //i >> av.get();
   return i;
}
#endif

