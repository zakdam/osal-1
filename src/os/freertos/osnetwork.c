/*
** File   : osnetwork.c
*/

/****************************************************************************************
                                    INCLUDE FILES
****************************************************************************************/

#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"

#include "common_types.h"
#include "osapi.h"

/****************************************************************************************
                                     DEFINES
****************************************************************************************/

/****************************************************************************************
                                   GLOBAL DATA
****************************************************************************************/

/****************************************************************************************
                                INITIALIZATION FUNCTION
****************************************************************************************/

/****************************************************************************************
                                    Network API
****************************************************************************************/

/*--------------------------------------------------------------------------------------
    Purpose: Get the Host ID from Networking

    Returns: OS_ERROR if the  host id could not be found
             an opaque 32 bit host id if success (NOT AN IPv4 ADDRESS).

    WARNING: OS_NetworkGetID is currently [[deprecated]] as its behavior is
             unknown and not consistent across operating systems.
---------------------------------------------------------------------------------------*/
int32 OS_NetworkGetID             (void)
{
#ifdef OS_INCLUDE_NETWORK
#else
#endif
    return (OS_ERR_NOT_IMPLEMENTED);
}/* end OS_NetworkGetID */

/*--------------------------------------------------------------------------------------
    Name: OS_NetworkGetHostName
    
    Purpose: Gets the name of the current host

    Returns: OS_ERROR if the  host name could not be found
             OS_SUCCESS if the name was copied to host_name successfully
---------------------------------------------------------------------------------------*/
int32 OS_NetworkGetHostName       (char *host_name, uint32 name_len)
{
#ifdef OS_INCLUDE_NETWORK
#else
#endif
    return (OS_ERR_NOT_IMPLEMENTED);
}/* end OS_NetworkGetHostName */
