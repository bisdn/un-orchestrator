#ifndef GRAPH_INFO_H_
#define GRAPH_INFO_H_ 1

#pragma once

#include "lsi.h"
#include "../../network_controller/openflow_controller/controller.h"
#include "../../compute_controller/compute_controller.h"
#include "../graph/high_level_graph/high_level_graph.h"

class Controller;

class GraphInfo
{
private:
	Controller *controller;
	LSI *lsi;
	ComputeController *computeController;
	highlevel::Graph *graph;

	//FIXME: PUT the following methods protected, and the GraphCreator as a friend?
public:
	GraphInfo();
	~GraphInfo();
	
	void setController(Controller *controller);
	void setLSI(LSI *lsi);
	void setComputeController(ComputeController *computeController);
	void setGraph(highlevel::Graph *graph);
	
	ComputeController *getComputeController();
	LSI *getLSI();
	Controller *getController();
	highlevel::Graph *getGraph();
};

#endif //GRAPH_INFO_H_
