#include "nfs_manager.h"

void NFsManager::setDescription(Description *description)
{
	this->description = description;
}

nf_t NFsManager::getNFType()
{
	return description->getType();
}

string NFsManager::getCores()
{
	return description->getCores();
}
