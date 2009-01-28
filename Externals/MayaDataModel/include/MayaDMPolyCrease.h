/*
    Copyright (c) 2008 NetAllied Systems GmbH

    This file is part of MayaDataModel.

    Licensed under the MIT Open Source License,
    for details please see LICENSE file or the website
    http://www.opensource.org/licenses/mit-license.php
*/
#ifndef __MayaDM_POLYCREASE_H__
#define __MayaDM_POLYCREASE_H__
#include "MayaDMTypes.h"
#include "MayaDMConnectables.h"
#include "MayaDMPolyModifierWorld.h"
namespace MayaDM
{
class PolyCrease : public PolyModifierWorld
{
public:
public:
	PolyCrease(FILE* file,const std::string& name,const std::string& parent=""):PolyModifierWorld(file, name, parent, "polyCrease"){}
	virtual ~PolyCrease(){}
	void setCrease(size_t cr_i,float cr)
	{
		if(cr == -1.0) return;
		fprintf(mFile,"setAttr \".cr[%i]\" %f;\n", cr_i,cr);

	}
	void setCrease(size_t cr_start,size_t cr_end,float* cr)
	{
		fprintf(mFile,"setAttr \".cr[%i:%i]\" ", cr_start,cr_end);
		size_t size = (cr_end-cr_start)*1+1;
		for(size_t i=0;i<size;++i)
		{
			fprintf(mFile,"%f",cr[i]);
			if(i+1<size) fprintf(mFile," ");
		}
		fprintf(mFile,";\n");

	}
	void startCrease(size_t cr_start,size_t cr_end)
	{
		fprintf(mFile,"setAttr \".cr[%i:%i]\"",cr_start,cr_end);

	}
	void appendCrease(float cr)
	{
		fprintf(mFile," %f",cr);

	}
	void endCrease()
	{
		fprintf(mFile,";\n");

	}
	void setInputVertexComponents(const componentList& ivc)
	{
		fprintf(mFile,"setAttr \".ivc\" -type \"componentList\" ");
		ivc.write(mFile);
		fprintf(mFile,";\n");

	}
	void setCreaseVertex(size_t crv_i,float crv)
	{
		if(crv == -1.0) return;
		fprintf(mFile,"setAttr \".crv[%i]\" %f;\n", crv_i,crv);

	}
	void setCreaseVertex(size_t crv_start,size_t crv_end,float* crv)
	{
		fprintf(mFile,"setAttr \".crv[%i:%i]\" ", crv_start,crv_end);
		size_t size = (crv_end-crv_start)*1+1;
		for(size_t i=0;i<size;++i)
		{
			fprintf(mFile,"%f",crv[i]);
			if(i+1<size) fprintf(mFile," ");
		}
		fprintf(mFile,";\n");

	}
	void startCreaseVertex(size_t crv_start,size_t crv_end)
	{
		fprintf(mFile,"setAttr \".crv[%i:%i]\"",crv_start,crv_end);

	}
	void appendCreaseVertex(float crv)
	{
		fprintf(mFile," %f",crv);

	}
	void endCreaseVertex()
	{
		fprintf(mFile,";\n");

	}
	void getCrease(size_t cr_i)
	{
		fprintf(mFile,"\"%s.cr[%i]\"",mName.c_str(),cr_i);

	}
	void getInputVertexComponents()
	{
		fprintf(mFile,"\"%s.ivc\"",mName.c_str());

	}
	void getCreaseVertex(size_t crv_i)
	{
		fprintf(mFile,"\"%s.crv[%i]\"",mName.c_str(),crv_i);

	}
protected:
	PolyCrease(FILE* file,const std::string& name,const std::string& parent,const std::string& nodeType):PolyModifierWorld(file, name, parent, nodeType) {}

};
}//namespace MayaDM
#endif//__MayaDM_POLYCREASE_H__