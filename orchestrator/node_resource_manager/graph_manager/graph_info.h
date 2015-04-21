#ifndef GRAPH_INFO_H_
#define GRAPH_INFO_H_ 1

#pragma once

#include "lsi.h"
#include "../../network_controller/openflow_controller/controller.h"
#include "../../compute_controller/nfs_manager.h"
#include "../graph/high_level_graph/high_level_graph.h"

class Controller;

class GraphInfo
{
private:
	Controller *controller;
	LSI *lsi;
	NFsManager *nfsManager;
	highlevel::Graph *graph;

	//FIXME: PUT the following methods protected, and the GraphCreator as a friend?
public:
	GraphInfo();
	~GraphInfo();
	
	void setController(Controller *controller);
	void setLSI(LSI *lsi);
	void setNFsManager(NFsManager *nfsManager);
	void setGraph(highlevel::Graph *graph);
	
	NFsManager *getNFsManager();
	LSI *getLSI();
	Controller *getController();
	highlevel::Graph *getGraph();
};

#endif //GRAPH_INFO_H_
