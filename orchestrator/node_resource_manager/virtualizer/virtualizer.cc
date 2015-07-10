#include "virtualizer.h"

map<string,string> Virtualizer::nameVirtualization;
map<string,pair<string,string> > Virtualizer::NFVirtualization;
unsigned int Virtualizer::currentID = 1;

bool Virtualizer::init()
{
	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL) 
    {  
		PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_INIT);
		if (pythonFunction && PyCallable_Check(pythonFunction)) 
        {
	    	PyObject *pythonRetVal;
	    	
	    	
	    	//Call the python function
	    	pythonRetVal = PyObject_CallObject(pythonFunction, NULL);
            long int retVal = PyInt_AsLong(pythonRetVal);
                
            logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Result of call: %d\n", retVal);
            
			Py_DECREF(pythonRetVal);
            Py_XDECREF(pythonFunction);
	    	Py_DECREF(pythonFile);
	    	
	    	if(retVal)
	    		return true;
	    	else
	    		return false;
	    }
	    else 
        {
            if (PyErr_Occurred())
                PyErr_Print();
		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_INIT);
		   	Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);
	        return false;			
		}
    }
    else
    {
       	PyErr_Print();
       	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
		return false;
    }

	return true;
}

void Virtualizer::terminate()
{	
	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL) 
    {  
		PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_TERMINATE);
		if (pythonFunction && PyCallable_Check(pythonFunction)) 
        {
	    	//Call the python function
	    	PyObject_CallObject(pythonFunction, NULL);
	    	
            Py_XDECREF(pythonFunction);
	    	Py_DECREF(pythonFile);
	    }
	    else 
        {
            if (PyErr_Occurred())
                PyErr_Print();
		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_INIT);
		   	Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);
		}
    }
    else
    {
       	PyErr_Print();
       	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
    }
}

bool Virtualizer::addResources(int cpu, int memory, char *memory_unit, int storage, char *storage_unit)
{
	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL) 
    {  
		PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_ADDRESOURCES);
		if (pythonFunction && PyCallable_Check(pythonFunction)) 
        {
	    	PyObject *pythonArgs = NULL, *pythonRetVal, *pythonValue;
	    		    	
	    	//Set the arguments to the python function
	    	//TODO: check that pythonValue is not NULL
			pythonArgs = PyTuple_New(5);	
	    	pythonValue = PyInt_FromLong(cpu);
            PyTuple_SetItem(pythonArgs, 0, pythonValue);
            pythonValue = PyInt_FromLong(memory);
            PyTuple_SetItem(pythonArgs, 1, pythonValue);
            pythonValue = PyString_FromString(memory_unit);
            PyTuple_SetItem(pythonArgs, 2, pythonValue);
            pythonValue = PyInt_FromLong(storage);
            PyTuple_SetItem(pythonArgs, 3, pythonValue);
            pythonValue = PyString_FromString(storage_unit);
            PyTuple_SetItem(pythonArgs, 4, pythonValue);
	    	
	    	//Call the python function
	    	pythonRetVal = PyObject_CallObject(pythonFunction, pythonArgs);
            Py_DECREF(pythonArgs);
            
			long int retVal = PyInt_AsLong(pythonRetVal);    
            logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Result of call: %d\n", retVal);
                			
			Py_DECREF(pythonRetVal);
            Py_XDECREF(pythonFunction);
	    	Py_DECREF(pythonFile);
	    	
	    	if(retVal)
	    		return true;
	    	else
	    		return false;
	    }
	    else 
        {
            if (PyErr_Occurred())
                PyErr_Print();
            
		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_HANDLE_REQ);
		   	Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);
			return false;	
		}
    }
    else
    {
       	PyErr_Print();
       	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
		return false;
    }
}

bool Virtualizer::addPort(char *physicalName, char *name, char *type)
{
	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL) 
    {  
		PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_ADDNODEPORT);
		if (pythonFunction && PyCallable_Check(pythonFunction)) 
        {
	    	PyObject *pythonArgs = NULL, *pythonRetVal, *pythonValue;
	    		    	
	    	//Set the arguments to the python function
	    	//TODO: check that pythonValue is not NULL
			pythonArgs = PyTuple_New(2);
	    	pythonValue = PyString_FromString(name);
            PyTuple_SetItem(pythonArgs, 0, pythonValue);
            pythonValue = PyString_FromString(type);
            PyTuple_SetItem(pythonArgs, 1, pythonValue);
	    	
	    	//Call the python function
	    	pythonRetVal = PyObject_CallObject(pythonFunction, pythonArgs);
            Py_DECREF(pythonArgs);
            
			long int retVal = PyInt_AsLong(pythonRetVal);    
            logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Result of call: %d\n", retVal);
                			
			Py_DECREF(pythonRetVal);
            Py_XDECREF(pythonFunction);
	    	Py_DECREF(pythonFile);
	    	
	    	if(retVal)
	    	{
	    		string pn(physicalName);
	    		string n(name);
	    		nameVirtualization[pn] = n;
	    		return true;
	    	}
	    	else
	    		return false;
	    }
	    else 
        {
            if (PyErr_Occurred())
                PyErr_Print();
            
		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_HANDLE_REQ);
		   	Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);
			return false;	
		}
    }
    else
    {
       	PyErr_Print();
       	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
		return false;
    }
}

bool Virtualizer::EditPortID(string physicalPortName, unsigned int ID)
{

	if(nameVirtualization.count(physicalPortName) == 0)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Virtualization unknown for physical port '%s'",physicalPortName.c_str());
		return false;
	}
	string portName = nameVirtualization[physicalPortName];

	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL) 
	{ 
    	PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_EDIT_PORT_ID);
    	if (pythonFunction && PyCallable_Check(pythonFunction)) 
    	{
    		//Set the arguments to the python function
	    	//TODO: check that pythonValue is not NULL
			PyObject *pythonArgs = PyTuple_New(2);	
	    	PyObject *pythonValue = PyString_FromString(portName.c_str());
	    	PyTuple_SetItem(pythonArgs, 0, pythonValue);
	    	pythonValue = PyInt_FromLong(ID);
	    	PyTuple_SetItem(pythonArgs, 1, pythonValue);
	    	
	    	//Call the python function
	    	PyObject *pythonRetVal = PyObject_CallObject(pythonFunction, pythonArgs);
            Py_DECREF(pythonArgs);
            
            long int retVal = PyInt_AsLong(pythonRetVal);    
            logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Result of call: %d\n", retVal);

            Py_DECREF(pythonRetVal);
            Py_XDECREF(pythonFunction);
	    	Py_DECREF(pythonFile);
	    	
	    	if(retVal)
	    		return true;
	    	else
	    		return false;
    	}
    	else 
        {
            if (PyErr_Occurred())
                PyErr_Print();
		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_EDIT_PORT_ID);
			Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);
	        return false;		
		}
	}
	else
    {
	   	PyErr_Print();
	   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
	   	return false;
	}
}

bool Virtualizer::addSupportedVNFs(set<NF*> nfs)
{
	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL) 
	{ 
		PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_ADD_SUUPORTED_VNFs);
		if (pythonFunction && PyCallable_Check(pythonFunction)) 
		{
			//Iterate on all the NFs
			for(set<NF*>::iterator nfIt = nfs.begin(); nfIt != nfs.end(); nfIt++)
			{
				//Iterate on all the available implementations for a NF
				NF *nf = *nfIt;
				
				list<Implementation*> implementations = nf->getAvailableImplementations();
				for(list<Implementation*>::iterator implementation = implementations.begin(); implementation != implementations.end(); implementation++)
				{
					ostringstream sID;
					sID << currentID;
					
					string id("NF"+sID.str());
					currentID++;
							
					//Set the arguments to the python function
					//TODO: check that pythonValue is not NULL
					PyObject *pythonArgs = PyTuple_New(4);	
					PyObject *pythonValue = PyString_FromString(id.c_str());
					PyTuple_SetItem(pythonArgs, 0, pythonValue);
					pythonValue = PyString_FromString((nf->getName()).c_str());
					PyTuple_SetItem(pythonArgs, 1, pythonValue);
					
					nf_t type = (*implementation)->getType();
					pythonValue = PyInt_FromLong(NFType::toID(type));
					PyTuple_SetItem(pythonArgs, 2, pythonValue);
					
					pythonValue = PyInt_FromLong(nf->getNumPorts());
					PyTuple_SetItem(pythonArgs, 3, pythonValue);
				
					//Call the python function
					PyObject *pythonRetVal = PyObject_CallObject(pythonFunction, pythonArgs);
				    Py_DECREF(pythonArgs);
				    
				    long int retVal = PyInt_AsLong(pythonRetVal);    
				    logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Result of call: %d\n", retVal);

				    Py_DECREF(pythonRetVal);
				
					if(!retVal)
					{
						Py_XDECREF(pythonFunction);
						Py_DECREF(pythonFile);
						return false;
					}
				}
				
			}	
			Py_XDECREF(pythonFunction);
			Py_DECREF(pythonFile);
		}
    	else 
        {
            if (PyErr_Occurred())
                PyErr_Print();
		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_ADD_SUUPORTED_VNFs);
			Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);
	        return false;		
		}

	}
	else
    {
	   	PyErr_Print();
	   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
	   	return false;
	}

	return true;
}

bool Virtualizer::handleRestRequest(char *message, char **answer, const char *url, const char *method)
{
	//In this case, the request in handled by the Python code
	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL) 
    {  
		PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_HANDLE_REQ);
		if (pythonFunction && PyCallable_Check(pythonFunction)) 
        {
	    	PyObject *pythonArgs = NULL, *pythonValue;
	    	

			//Set the arguments to the python function            			
 			int numArgs = (message != NULL)? 3 : 2;
			pythonArgs = PyTuple_New(numArgs);	
	    	pythonValue = PyString_FromString(method);
            PyTuple_SetItem(pythonArgs, 0, pythonValue);
            pythonValue = PyString_FromString(url);
            PyTuple_SetItem(pythonArgs, 1, pythonValue);
			if(message != NULL)
			{
				pythonValue = PyString_FromString(message);
				PyTuple_SetItem(pythonArgs, 2, pythonValue);
			}        
	    	
	    	//Call the python function
	    	PyObject *pythonRetVal = PyObject_CallObject(pythonFunction, pythonArgs);
            Py_DECREF(pythonArgs);
                
            logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Result of call: %s\n", PyString_AsString(pythonRetVal));
            
	    	
	    	//TODO: handle better the stuffs here
//	    	if (0 == strcmp (method, GET))
	    	{
	    		//All the GET have the same answer
	    			    		
			    string tmp = PyString_AsString(pythonRetVal);
				*answer = NULL;
		 		*answer = (char*)malloc(sizeof(char) * (tmp.size()+1));
		 		memcpy((*answer), tmp.c_str(),tmp.size()+1);
		 		(*answer)[tmp.size()] = '\0';
			    
				return true;
			}
			
			Py_DECREF(pythonRetVal);
            Py_XDECREF(pythonFunction);
	    	Py_DECREF(pythonFile);
	    }
	    else 
        {
            if (PyErr_Occurred())
                PyErr_Print();
            
		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_HANDLE_REQ);
			Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);
	        
	        return false;			
		}
    }
    else
    {
       	PyErr_Print();
       	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
		Py_DECREF(pythonFile);
		return false;
    }

}

