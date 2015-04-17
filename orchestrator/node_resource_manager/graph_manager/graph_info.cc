#include "graph_info.h"

GraphInfo::GraphInfo() :
	controller(NULL), lsi(NULL), nfsManager(NULL)//, graph(NULL)
{

}

GraphInfo::~GraphInfo()
{
}

void GraphInfo::setController(Controller *controller)
{
	this->controller = controller;
}

void GraphInfo::setLSI(LSI *lsi)
{
	this->lsi = lsi;
}

void GraphInfo::setNFsManager(NFsManager *nfsManager)
{
	this->nfsManager = nfsManager;
}

void GraphInfo::setGraph(highlevel::Graph *graph)
{
	this->graph = graph;
}

Controller *GraphInfo::getController()
{
	return controller;
}

LSI *GraphInfo::getLSI()
{
	return lsi;
}

highlevel::Graph *GraphInfo::getGraph()
{
	return graph;
}

NFsManager *GraphInfo::getNFsManager()
{
	return nfsManager;
}

