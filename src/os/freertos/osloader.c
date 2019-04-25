/*
** File   : osloader.c
*/

/****************************************************************************************
                                    INCLUDE FILES
****************************************************************************************/
#include <stdio.h>
#include <unistd.h> /* close() */
#include <string.h> /* memset() */
#include <stdlib.h>
#include <fcntl.h>

#include "common_types.h"
#include "osapi.h"

/*
** If OS_INCLUDE_MODULE_LOADER is not defined, then skip the module 
*/
#ifdef OS_INCLUDE_MODULE_LOADER

/****************************************************************************************
                                     TYPEDEFS
****************************************************************************************/

/****************************************************************************************
                                     DEFINES
****************************************************************************************/

/****************************************************************************************
                                   GLOBAL DATA
****************************************************************************************/

/****************************************************************************************
                                INITIALIZATION FUNCTION
****************************************************************************************/
int32  OS_ModuleTableInit ( void )
{
#ifdef OS_STATIC_LOADER
#else
#endif
    return (OS_ERR_NOT_IMPLEMENTED);
}

/****************************************************************************************
                                    Symbol table API
****************************************************************************************/
/*--------------------------------------------------------------------------------------
    Name: OS_SymbolLookup
    
    Purpose: Find the Address of a Symbol 

    Parameters: 

    Returns: OS_ERROR if the symbol could not be found
             OS_SUCCESS if the symbol is found 
             OS_INVALID_POINTER if one of the pointers passed in are NULL 
---------------------------------------------------------------------------------------*/
int32 OS_SymbolLookup( cpuaddr *SymbolAddress, const char *SymbolName )
{
#ifdef OS_STATIC_LOADER
#else
#endif
    return (OS_ERR_NOT_IMPLEMENTED);
} /* end OS_SymbolLookup */

/*--------------------------------------------------------------------------------------
    Name: OS_SymbolTableDump
    
    Purpose: Dumps the system symbol table to a file

    Parameters: 

    Returns: OS_ERROR if the symbol table could not be read or dumped
             OS_FS_ERR_PATH_INVALID  if the file and/or path is invalid 
             OS_SUCCESS if the file is written correctly 
---------------------------------------------------------------------------------------*/
int32 OS_SymbolTableDump ( const char *filename, uint32 SizeLimit )
{
#ifdef OS_STATIC_LOADER
#else
#endif
    return (OS_ERR_NOT_IMPLEMENTED);
} /* end OS_SymbolTableDump */

/****************************************************************************************
                                    Module Loader API
****************************************************************************************/

/*--------------------------------------------------------------------------------------
    Name: OS_ModuleLoad
    
    Purpose: Loads an ELF object file into the running operating system

    Parameters: 

    Returns: OS_ERROR if the module cannot be loaded
             OS_INVALID_POINTER if one of the parameters is NULL
             OS_ERR_NO_FREE_IDS if the module table is full
             OS_ERR_NAME_TAKEN if the name is in use
             OS_SUCCESS if the module is loaded successfuly 
---------------------------------------------------------------------------------------*/
int32 OS_ModuleLoad ( uint32 *module_id, const char *module_name, const char *filename )
{
#ifdef OS_STATIC_LOADER
#else
#endif
    return (OS_ERR_NOT_IMPLEMENTED);
} /* end OS_ModuleLoad */

/*--------------------------------------------------------------------------------------
    Name: OS_ModuleUnload
    
    Purpose: Unloads the module file from the running operating system

    Parameters: 

    Returns: OS_ERROR if the module is invalid or cannot be unloaded
             OS_SUCCESS if the module was unloaded successfuly 
---------------------------------------------------------------------------------------*/
int32 OS_ModuleUnload ( uint32 module_id )
{
#ifdef OS_STATIC_LOADER
#else
#endif
    return (OS_ERR_NOT_IMPLEMENTED);
} /* end OS_ModuleUnload */

/*--------------------------------------------------------------------------------------
    Name: OS_ModuleInfo
    
    Purpose: Returns information about the loadable module

    Parameters: 

    Returns: OS_INVALID_POINTER if the pointer to the ModuleInfo structure is NULL 
             OS_ERR_INVALID_ID if the module ID is not valid
             OS_SUCCESS if the module info was filled out successfuly 
---------------------------------------------------------------------------------------*/
int32 OS_ModuleInfo ( uint32 module_id, OS_module_prop_t *module_info )
{
#ifdef OS_STATIC_LOADER
#else
#endif
    return (OS_ERR_NOT_IMPLEMENTED);
} /* end OS_ModuleInfo */

#endif /* OS_INCLUDE_MODULE_LOADER */
