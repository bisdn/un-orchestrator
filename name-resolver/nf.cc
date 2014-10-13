#include "nf.h"


NF::NF(string name) :
	name(name)
{

}

void NF::addImplementation(Implementation *implementation)
{
	implementations.push_back(implementation);
}

string NF::getName()
{
	return name;
}

Object NF::toJSON()
{
	Object nf;	
	
	nf["name"]  = name;
	
	Array impl;
	for(list<Implementation*>::iterator i = implementations.begin(); i != implementations.end();i++)
	{
		impl.push_back((*i)->toJSON());
	}
	
	nf["implementations"] = impl;
	
	return nf;
}
