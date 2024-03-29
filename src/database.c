/**
 * @file database.c
 * @author José Carvalho, João Carneiro
 * @brief Database C module implementation
 * @date 2022-01-13
 * 
 * @copyright Copyright (c) 2022
 */

#include "../inc/database.h"

PyObject *pName, *pModule, *pDict;
PyObject *pFuncSend_notification_flag, *pFuncGet_live_flag, *pFuncGet_swing_flag, *pFuncSend_temp_hum, *pFuncSend_swing_flag,*pResult;


int send_temp_hum( float temp, float hum)
{
    //Call the function with arguments -> i = one integer , "ii" = two integers , "d" -> double mkvalue-style format string
    pResult = PyObject_CallFunction(pFuncSend_temp_hum, "ff", temp, hum);
    if(pResult == NULL)
    {
        PyErr_Print();
        return 0;
    }

    //Cleanup
    Py_XDECREF(pResult);

    return 1;
}

int get_swing_flag()
{
    //Call the function with arguments -> i = one integer , "ii" = two integers , "d" -> double mkvalue-style format string
    pResult = PyObject_CallFunction(pFuncGet_swing_flag, NULL);
    if(pResult == NULL)
    {
        PyErr_Print();
        return 0;
    }

    //Truncates the result to int since the Python function results
    //Translates from Python to C double
    int aux = (int)PyLong_AsLong(pResult);

    if (aux < 0)
    {
        //Cleanup
        Py_XDECREF(pResult);
        printf("ERROR GETTING FLAG");
        return -ERROR;
    }

    //Cleanup
    Py_XDECREF(pResult);

    return aux;
}

int get_live_flag()
{
    //Call the function with arguments -> i = one integer , "ii" = two integers , "d" -> double mkvalue-style format string
    pResult = PyObject_CallFunction(pFuncGet_live_flag, NULL);
    if(pResult == NULL)
    {
        PyErr_Print();
        return 0;
    }
    PyErr_Print();

    //Truncates the result to int since the Python function results
    //Translates from Python to C double
    int aux = (int)PyLong_AsLong(pResult);

     if (aux < 0)
    {
        //Cleanup
        Py_XDECREF(pResult);
        printf("ERROR GETTING FLAG");
        return -ERROR;
    }

    //Cleanup
    Py_XDECREF(pResult);

    return aux;
}

int send_notification_flag( int notification_flag)
{
    //Call the function with arguments -> i = one integer , "ii" = two integers , "d" -> double mkvalue-style format string
    pResult = PyObject_CallFunction(pFuncSend_notification_flag, "i", notification_flag);
    if(pResult == NULL)
    {
        PyErr_Print();
        return 0;
    }

    //Cleanup
    Py_XDECREF(pResult);

    return 1;
}

int send_swing_flag(int swing_flag)
{
    //Call the function with arguments -> i = one integer , "ii" = two integers , "d" -> double mkvalue-style format string
    pResult = PyObject_CallFunction(pFuncSend_swing_flag, "i", swing_flag);
    if(pResult == NULL)
    {
        PyErr_Print();
        return 0;
    }
    
    //Cleanup
    Py_XDECREF(pResult);

    return 1;
}

int initDatabase()
{
    pName = PyUnicode_FromString((char*)"database");

    //Import the Python Script
    pModule = PyImport_Import(pName);

    if(pModule == NULL)
    {
        printf("No module with that name");
        return -ERROR;
    }

    //Get the dictionary referent to the imported module
    pDict = PyModule_GetDict(pModule);

    Py_DECREF(pName);
    Py_DECREF(pModule);

    pFuncSend_notification_flag = PyDict_GetItemString(pDict, (char*)"send_notification_flag");
    
    //Check if its callable
    if (!PyCallable_Check(pFuncSend_notification_flag))
    {
        PyErr_Print();
        return 0;
    }

    pFuncSend_temp_hum = PyDict_GetItemString(pDict, (char*) "send_temp_hum");

    //Check if its callable
    if (!PyCallable_Check(pFuncSend_temp_hum))
    {
        PyErr_Print();
        return 0;
    }

    pFuncGet_live_flag = PyDict_GetItemString(pDict, (char*) "get_live_flag");

    //Check if its callable
    if (!PyCallable_Check(pFuncGet_live_flag))
    {
        PyErr_Print();
        return 0;
    }

    pFuncGet_swing_flag = PyDict_GetItemString(pDict, (char*) "get_swing_flag");
    
    //Check if its callable
    if (!PyCallable_Check(pFuncGet_swing_flag))
    {
        PyErr_Print();
        return 0;
    }

    pFuncSend_swing_flag = PyDict_GetItemString(pDict, (char*) "send_swing_flag");
    
    //Check if its callable
    if (!PyCallable_Check(pFuncSend_swing_flag))
    {
        PyErr_Print();
        return 0;
    }
    
    return 1;
}

