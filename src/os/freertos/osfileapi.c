/*
** File   : osfileapi.c
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
#include <sys/stat.h>

#include "common_types.h"
#include "osapi.h"

/****************************************************************************************
                                     DEFINES
****************************************************************************************/

/***************************************************************************************
                                 FUNCTION PROTOTYPES
***************************************************************************************/

/****************************************************************************************
                                   GLOBAL DATA
****************************************************************************************/

/****************************************************************************************
                                INITIALIZATION FUNCTION
****************************************************************************************/
int32 OS_FS_Init(void)
{
   return (OS_FS_UNIMPLEMENTED);
}
/****************************************************************************************
                                    Filesys API
****************************************************************************************/

/*
** Standard File system API
*/
/*--------------------------------------------------------------------------------------
    Name: OS_creat
    
    Purpose: creates a file specified by const char *path, with read/write 
             permissions by access. The file is also automatically opened by the
             create call.
    
    Returns: OS_FS_ERR_INVALID_POINTER if path is NULL
             OS_FS_ERR_PATH_TOO_LONG if path exceeds the maximum number of chars
             OS_FS_ERR_PATH_INVALID if path cannot be parsed
             OS_FS_ERR_NAME_TOO_LONG if the name of the file is too long
             OS_FS_ERROR if permissions are unknown or OS call fails
             OS_FS_ERR_NO_FREE_FDS if there are no free file descripors left
             File Descriptor is returned on success. 
---------------------------------------------------------------------------------------*/
int32 OS_creat (const char *path, int32 access)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_creat */

/*--------------------------------------------------------------------------------------
    Name: OS_open
    
    Purpose: Opens a file. access parameters are OS_READ_ONLY,OS_WRITE_ONLY, or 
             OS_READ_WRITE

    Returns: OS_FS_ERR_INVALID_POINTER if path is NULL
             OS_FS_ERR_PATH_TOO_LONG if path exceeds the maximum number of chars
             OS_FS_ERR_PATH_INVALID if path cannot be parsed
             OS_FS_ERR_NAME_TOO_LONG if the name of the file is too long
             OS_FS_ERROR if permissions are unknown or OS call fails
             OS_FS_ERR_NO_FREE_FDS if there are no free file descriptors left
             a file descriptor if success
---------------------------------------------------------------------------------------*/
int32 OS_open   (const char *path,  int32 access,  uint32  mode)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_open */

/*--------------------------------------------------------------------------------------
    Name: OS_close
    
    Purpose: Closes a file. 

    Returns: OS_FS_ERROR if file  descriptor could not be closed
             OS_FS_ERR_INVALID_FD if the file descriptor passed in is invalid
             OS_FS_SUCCESS if success
---------------------------------------------------------------------------------------*/
int32 OS_close (int32  filedes)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_close */

/*--------------------------------------------------------------------------------------
    Name: OS_read
    
    Purpose: reads up to nbytes from a file, and puts them into buffer. 
    
    Returns: OS_FS_ERR_INVALID_POINTER if buffer is a null pointer
             OS_FS_ERROR if OS call failed
             OS_FS_ERR_INVALID_FD if the file descriptor passed in is invalid
             number of bytes read if success
---------------------------------------------------------------------------------------*/
int32 OS_read  (int32  filedes, void *buffer, uint32 nbytes)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_read */

/*--------------------------------------------------------------------------------------
    Name: OS_write

    Purpose: writes to a file. copies up to a maximum of nbtyes of buffer to the file
             described in filedes

    Returns: OS_FS_ERR_INVALID_POINTER if buffer is NULL
             OS_FS_ERROR if OS call failed
             OS_FS_ERR_INVALID_FD if the file descriptor passed in is invalid
             number of bytes written if success
---------------------------------------------------------------------------------------*/
int32 OS_write (int32  filedes, void *buffer, uint32 nbytes)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_write */

/*--------------------------------------------------------------------------------------
    Name: OS_chmod

    Notes: This is not going to be implemented because there is no use for this function.
---------------------------------------------------------------------------------------*/
int32 OS_chmod  (const char *path, uint32 access)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_chmod */

/*--------------------------------------------------------------------------------------
    Name: OS_stat
    
    Purpose: returns information about a file or directory in a os_fs_stat structure
    
    Returns: OS_FS_ERR_INVALID_POINTER if path or filestats is NULL
             OS_FS_ERR_PATH_TOO_LONG if the path is too long to be stored locally
*****        OS_FS_ERR_NAME_TOO_LONG if the name of the file is too long to be stored
             OS_FS_ERR_PATH_INVALID if path cannot be parsed
             OS_FS_ERROR id the OS call failed
             OS_FS_SUCCESS if success

    Note: The information returned is in the structure pointed to by filestats         
---------------------------------------------------------------------------------------*/
int32 OS_stat   (const char *path, os_fstat_t  *filestats)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_stat */

/*--------------------------------------------------------------------------------------
    Name: OS_lseek

    Purpose: sets the read/write pointer to a specific offset in a specific file. 
             Whence is either OS_SEEK_SET,OS_SEEK_CUR, or OS_SEEK_END

    Returns: the new offset from the beginning of the file
             OS_FS_ERR_INVALID_FD if the file descriptor passed in is invalid
             OS_FS_ERROR if OS call failed
---------------------------------------------------------------------------------------*/
int32 OS_lseek  (int32  filedes, int32 offset, uint32 whence)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_lseek */

/*--------------------------------------------------------------------------------------
    Name: OS_remove

    Purpose: removes a given filename from the drive
 
    Returns: OS_FS_SUCCESS if the driver returns OK
             OS_FS_ERROR if there is no device or the driver returns error
             OS_FS_ERR_INVALID_POINTER if path is NULL
             OS_FS_ERR_PATH_TOO_LONG if path is too long to be stored locally
             OS_FS_ERR_PATH_INVALID if path cannot be parsed
             OS_FS_ERR_NAME_TOO_LONG if the name of the file to remove is too long to be
             stored locally
---------------------------------------------------------------------------------------*/
int32 OS_remove (const char *path)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_remove */

/*--------------------------------------------------------------------------------------
    Name: OS_rename
    
    Purpose: renames a file

    Returns: OS_FS_SUCCESS if the rename works
             OS_FS_ERROR if the file could not be opened or renamed.
             OS_FS_ERR_INVALID_POINTER if old_filename or new_filename are NULL
             OS_FS_ERR_PATH_INVALID if path cannot be parsed
             OS_FS_ERR_PATH_TOO_LONG if the paths given are too long to be stored locally
             OS_FS_ERR_NAME_TOO_LONG if the new name is too long to be stored locally
---------------------------------------------------------------------------------------*/
int32 OS_rename (const char *old_filename, const char *new_filename)
{
    return (OS_FS_UNIMPLEMENTED);
} /*end OS_rename */

/*--------------------------------------------------------------------------------------
    Name: OS_cp
    
    Purpose: Copies a single file from src to dest

    Returns: OS_FS_SUCCESS if the operation worked
             OS_FS_ERROR if the file could not be accessed
             OS_FS_ERR_INVALID_POINTER if src or dest are NULL
             OS_FS_ERR_PATH_INVALID if path cannot be parsed
             OS_FS_ERR_PATH_TOO_LONG if the paths given are too long to be stored locally
             OS_FS_ERR_NAME_TOO_LONG if the dest name is too long to be stored locally
---------------------------------------------------------------------------------------*/
int32 OS_cp (const char *src, const char *dest)
{
    return (OS_FS_UNIMPLEMENTED);
} /*end OS_cp */

/*--------------------------------------------------------------------------------------
    Name: OS_mv
    
    Purpose: moves a single file from src to dest

    Returns: OS_FS_SUCCESS if the rename works
             OS_FS_ERROR if the file could not be opened or renamed.
             OS_FS_ERR_INVALID_POINTER if src or dest are NULL
             OS_FS_ERR_PATH_INVALID if path cannot be parsed
             OS_FS_ERR_PATH_TOO_LONG if the paths given are too long to be stored locally
             OS_FS_ERR_NAME_TOO_LONG if the dest name is too long to be stored locally
---------------------------------------------------------------------------------------*/
int32 OS_mv (const char *src, const char *dest)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_mv */

/* --------------------------------------------------------------------------------------
Name: OS_FDGetInfo
    
Purpose: Copies the information of the given file descriptor into a structure passed in
    
Returns: OS_FS_ERR_INVALID_FD if the file descriptor passed in is invalid
         OS_FS_SUCCESS if the copying was successfull
 ---------------------------------------------------------------------------------------*/
int32 OS_FDGetInfo (int32 filedes, OS_FDTableEntry *fd_prop)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_FDGetInfo */

/* --------------------------------------------------------------------------------------
   Name: OS_FileOpenCheck
    
   Purpose: Checks to see if a file is open 
    
   Returns: OS_FS_ERROR if the file is not open 
            OS_FS_SUCCESS if the file is open 
 ---------------------------------------------------------------------------------------*/
int32 OS_FileOpenCheck(char *Filename)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_FileOpenCheck */

/* --------------------------------------------------------------------------------------
   Name: OS_CloseAllFiles

   Purpose: Closes All open files that were opened through the OSAL 

   Returns: OS_FS_ERROR   if one or more file close returned an error
            OS_FS_SUCCESS if the files were all closed without error
 ---------------------------------------------------------------------------------------*/
int32 OS_CloseAllFiles(void)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_CloseAllFiles */

/* --------------------------------------------------------------------------------------
   Name: OS_CloseFileByName

   Purpose: Allows a file to be closed by name.
            This will only work if the name passed in is the same name used to open 
            the file.

   Returns: OS_FS_ERR_PATH_INVALID if the file is not found 
            OS_FS_ERROR   if the file close returned an error
            OS_FS_SUCCESS if the file close suceeded  
 ---------------------------------------------------------------------------------------*/
int32 OS_CloseFileByName(char *Filename)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_CloseFileByName */

/*
** Directory API 
*/
/*--------------------------------------------------------------------------------------
    Name: OS_mkdir

    Purpose: makes a directory specified by path.

    Returns: OS_FS_ERR_INVALID_POINTER if path is NULL
             OS_FS_ERR_PATH_TOO_LONG if the path is too long to be stored locally
             OS_FS_ERR_PATH_INVALID if path cannot be parsed
             OS_FS_ERROR if the OS call fails
             OS_FS_SUCCESS if success

    Note: The access parameter is currently unused.
---------------------------------------------------------------------------------------*/
int32 OS_mkdir (const char *path, uint32 access)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_mkdir */

/*--------------------------------------------------------------------------------------
    Name: OS_opendir

    Purpose: opens a directory for searching

    Returns: NULL if the directory cannot be opened
             Directory pointer if the directory is opened
---------------------------------------------------------------------------------------*/
os_dirp_t OS_opendir (const char *path)
{
    return (NULL);
} /* end OS_opendir */

/*--------------------------------------------------------------------------------------
    Name: OS_closedir
    
    Purpose: closes a directory

    Returns: OS_FS_SUCCESS if success
             OS_FS_ERROR if close failed
---------------------------------------------------------------------------------------*/
int32 OS_closedir (os_dirp_t directory)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_closedir */

/*--------------------------------------------------------------------------------------
    Name: OS_rewinddir

    Purpose: Rewinds the directory pointer

    Returns: N/A
---------------------------------------------------------------------------------------*/
void  OS_rewinddir (os_dirp_t directory )
{
} /* end OS_rewinddir */

/*--------------------------------------------------------------------------------------
    Name: OS_readdir

    Purpose: obtains directory entry data for the next file from an open directory

    Returns: a pointer to the next entry for success
             NULL if error or end of directory is reached
---------------------------------------------------------------------------------------*/
os_dirent_t *  OS_readdir (os_dirp_t directory)
{
    return (NULL);
} /* end OS_readdir */

/*--------------------------------------------------------------------------------------
    Name: OS_rmdir
    
    Purpose: removes a directory from  the structure (must be an empty directory)

    Returns: OS_FS_ERR_INVALID_POINTER if path is NULL
             OS_FS_ERR_PATH_INVALID if path cannot be parsed    
             OS_FS_ERR_PATH_TOO_LONG
---------------------------------------------------------------------------------------*/
int32  OS_rmdir (const char *path)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_rmdir */

/*
** Shell API 
*/
/* --------------------------------------------------------------------------------------
    Name: OS_ShellOutputToFile
    
    Purpose: Takes a shell command in and writes the output of that command to the specified file
    
    Returns: OS_FS_ERROR if the command was not executed properly
             OS_FS_ERR_INVALID_FD if the file descriptor passed in is invalid
             OS_SUCCESS if success
 ---------------------------------------------------------------------------------------*/
int32 OS_ShellOutputToFile(char* Cmd, int32 OS_fd)
{
    return (OS_FS_UNIMPLEMENTED);
} /* end OS_ShellOutputToFile */
