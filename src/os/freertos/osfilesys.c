/*
** File   : osfilesys.c
*/

/****************************************************************************************
                                    INCLUDE FILES
****************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/statvfs.h>

#include "common_types.h"
#include "osapi.h"
#include "osconfig.h"

/****************************************************************************************
                                     DEFINES
****************************************************************************************/

/****************************************************************************************
                                   Prototypes
****************************************************************************************/

/****************************************************************************************
                                   GLOBAL DATA
****************************************************************************************/

/****************************************************************************************
                                    Filesys API
****************************************************************************************/
/*
** System Level API 
*/

/*---------------------------------------------------------------------------------------
    Name: OS_mkfs

    Purpose: Makes a RAM disk on the target with the RTEMS RFS file system
    
    Returns: OS_FS_ERR_INVALID_POINTER if devname is NULL
             OS_FS_ERR_DRIVE_NOT_CREATED if the OS calls to create the the drive failed
             OS_FS_ERR_DEVICE_NOT_FREE if the volume table is full
             OS_FS_SUCCESS on creating the disk

    Note: if address == 0, then a malloc will be called to create the disk
---------------------------------------------------------------------------------------*/
int32 OS_mkfs (char *address, char *devname,char * volname, uint32 blocksize, 
               uint32 numblocks)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_mkfs */

/*--------------------------------------------------------------------------------------
    Name: OS_mount
    
    Purpose: mounts a drive.
---------------------------------------------------------------------------------------*/
int32 OS_mount (const char *devname, char* mountpoint)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_mount */

/*---------------------------------------------------------------------------------------
    Name: OS_initfs

    Purpose: Inititalizes a file system on the target

    Returns: OS_FS_ERR_INVALID_POINTER if devname is NULL
             OS_FS_DRIVE_NOT_CREATED if the OS calls to create the the drive failed
             OS_FS_SUCCESS on creating the disk
             OS_FS_ERR_PATH_TOO_LONG if the name is too long
             OS_FS_ERR_DEVICE_NOT_FREE if the volume table is full
---------------------------------------------------------------------------------------*/
int32 OS_initfs (char *address,char *devname, char *volname, 
                uint32 blocksize, uint32 numblocks)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_initfs */

/*---------------------------------------------------------------------------------------
    Name: OS_rmfs

    Purpose: Removes a file system from the volume table. 
    
    Returns: OS_FS_ERR_INVALID_POINTER if devname is NULL
             OS_FS_ERROR is the drive specified cannot be located
             OS_FS_SUCCESS on removing  the disk
---------------------------------------------------------------------------------------*/
int32 OS_rmfs (char *devname)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_rmfs */

/*--------------------------------------------------------------------------------------
    Name: OS_unmount
    
    Purpose: unmounts a drive. and therefore makes all file descriptors pointing into
             the drive obsolete.

    Returns: OS_FS_ERR_INVALID_POINTER if name is NULL
             OS_FS_ERR_PATH_TOO_LONG if the absolute path given is too long
             OS_FS_ERROR if the OS calls failed
             OS_FS_SUCCESS if success
---------------------------------------------------------------------------------------*/
int32 OS_unmount (const char *mountpoint)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_umount */

/*--------------------------------------------------------------------------------------
    Name: OS_fsBlocksFree

    Purpose: Returns the number of free blocks in a volume
 
    Returns: OS_FS_ERR_INVALID_POINTER if name is NULL
             OS_FS_ERR_PATH_TOO_LONG if the name is too long
             OS_FS_ERROR if the OS call failed
             The number of blocks free in a volume if success
---------------------------------------------------------------------------------------*/
int32 OS_fsBlocksFree (const char *name)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_fsBlocksFree */

/*--------------------------------------------------------------------------------------
    Name: OS_fsBytesFree

    Purpose: Returns the number of free bytes in a volume
 
    Returns: OS_FS_ERR_INVALID_POINTER if name is NULL
             OS_FS_ERR_PATH_TOO_LONG if the name is too long
             OS_FS_ERROR if the OS call failed
             OS_FS_SUCCESS if the call succeeds 
---------------------------------------------------------------------------------------*/
int32 OS_fsBytesFree (const char *name, uint64 *bytes_free)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_fsBytesFree */

/*--------------------------------------------------------------------------------------
    Name: OS_chkfs
    
    Purpose: Checks the drives for inconsisenties and either repairs it or not

    Returns: OS_FS_ERR_INVALID_POINTER if name is NULL
             OS_FS_SUCCESS if success
             OS_FS_ERROR if the OS calls fail
---------------------------------------------------------------------------------------*/
os_fshealth_t OS_chkfs (const char *name, boolean repair)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_chkfs */

/*--------------------------------------------------------------------------------------
    Name: OS_FS_GetPhysDriveName
    
    Purpose: Returns the name of the physical volume associated with the drive,
             when given the mount point of the drive

    Returns: OS_FS_ERR_INVALID_POINTER if either  parameter is NULL
             OS_SUCCESS if success
             OS_FS_ERROR if the mountpoint could not be found
---------------------------------------------------------------------------------------*/
int32 OS_FS_GetPhysDriveName(char * PhysDriveName, char * MountPoint)
{
    return (OS_FS_UNIMPLEMENTED);;
} /* end OS_FS_GetPhysDriveName */

/*-------------------------------------------------------------------------------------
 * Name: OS_TranslatePath
 * 
 * Purpose: Because of the abstraction of the filesystem across OSes, we have to change
 *          the name of the {file, directory, drive} to be what the OS can actually 
 *          accept
---------------------------------------------------------------------------------------*/
int32 OS_TranslatePath(const char *VirtualPath, char *LocalPath)
{
    return OS_FS_SUCCESS;
} /* end OS_TranslatePath */

/*--------------------------------------------------------------------------------------
     Name: OS_GetFsInfo
       
     Purpose: returns information about the file system in an os_fsinfo_t
         
     Returns: OS_FS_ERR_INVALID_POINTER if filesys_info is NULL
              OS_FS_SUCCESS if success

              Note: The information returned is in the structure pointed to by filesys_info         
---------------------------------------------------------------------------------------*/
int32 OS_GetFsInfo(os_fsinfo_t  *filesys_info)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end of OS_GetFsInfo */

/*---------------------------------------------------------------------------------------
    Name: OS_FS_GetErrorName()

    Purpose: a handy debugging tool that will copy the name of the error code to a buffer

    Returns: OS_FS_ERROR if given error number is unknown
              OS_FS_SUCCESS if given error is found and copied to the buffer
--------------------------------------------------------------------------------------- */
int32 OS_FS_GetErrorName(int32 error_num, os_fs_err_name_t * err_name)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_FS_GetErrorName */
