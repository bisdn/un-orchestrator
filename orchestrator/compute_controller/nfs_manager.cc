#include "nfs_manager.h"

void NFsManager::setImplementation(Implementation *implementation)
{
	this->implementation = implementation;
}

nf_t NFsManager::getNFType()
{
	return implementation->getType();
}

string NFsManager::getCores()
{
	return implementation->getCores();
}
