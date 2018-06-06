// KAPow for OpenCL (KOCL) interface
// James Davis, 2016

#include <iostream>
#include <sstream>
#include <string>

#include <Python.h>
#include "KOCL.h"

struct KOCL_t {
	PyObject* script_obj;
};

KOCL_t* KOCL_init(float update_period) {

	Py_Initialize();

	PyRun_SimpleFile(fopen("../build/KOCL.py", "r"), "KOCL.py");

	PyObject* script_main = PyImport_AddModule("__main__");
    PyObject* script_dict = PyModule_GetDict(script_main);
	PyObject* script_class = PyDict_GetItemString(script_dict, "KOCL_iface");

	KOCL_t* KOCL = (KOCL_t*)malloc(sizeof(KOCL_t));
	PyObject* vars = Py_BuildValue("(f)", update_period);
	KOCL->script_obj = PyObject_CallObject(script_class, vars);

	Py_DECREF(vars);
	return KOCL;
}

float KOCL_get(KOCL_t* KOCL, char* kernel_name) {

	char method_name[64] = "split_get";
	char arg_type[8] = "(s)";

	PyObject* rtn = PyObject_CallMethod(KOCL->script_obj, method_name, arg_type, kernel_name);

	float power;
	if(rtn == Py_None) {
		power = NAN;
		// This should probably do something nicer when a non-kernel name is given
	}
	else {
		power = (float)PyFloat_AsDouble(rtn);
	}

	Py_DECREF(rtn);
	return power;
}

void KOCL_print(KOCL_t* KOCL) {

	char method_name[64] = "split_print";
	PyObject* rtn = PyObject_CallMethod(KOCL->script_obj, method_name, NULL);

	Py_DECREF(rtn);
}

int KOCL_built(KOCL_t* KOCL) {

	char method_name[64] = "built";
	PyObject* rtn = PyObject_CallMethod(KOCL->script_obj, method_name, NULL);

	int built = (rtn == Py_True);

	Py_DECREF(rtn);
	return built;
}

void KOCL_del(KOCL_t* KOCL) {

	Py_DECREF(KOCL->script_obj);
	Py_Finalize();

	free(KOCL);
}
