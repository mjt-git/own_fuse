/*
  Big Brother File System
  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.
  A copy of that code is included in the file fuse.h
  
  The point of this FUSE filesystem is to provide an introduction to
  FUSE.  It was my first FUSE filesystem as I got to know the
  software; hopefully, the comments in this code will help people who
  follow later to get a gentler introduction.

  This might be called a no-op filesystem:  it doesn't impose
  filesystem semantics on top of any other existing structure.  It
  simply reports the requests that come in, and passes them to an
  underlying filesystem.  The information is saved in a logfile named
  bbfs.log, in the directory from which you run bbfs.
*/
#include "params.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"

// global variable
int file1_size;
int file2_size;
int total_size;
char * fs_image_path;

///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come from /usr/include/fuse.h
//
/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int bb_getattr(const char *path, struct stat *statbuf)
{
    if(strcmp(path, "/file1") == 0 || strcmp(path, "/file2") == 0) {
        log_msg("\n**************\nINSIDE GETATTR\n**************\n");
        statbuf->st_mode = S_IFREG|0666;
        statbuf->st_nlink = 1;
        statbuf->st_uid = 0;
        statbuf->st_gid = 0;
        statbuf->st_rdev = 0;
        statbuf->st_size = file1_size;
        statbuf->st_atime = 0;
        statbuf->st_mtime = 0;
        statbuf->st_ctime = 0;
        log_stat(statbuf);
        return 0;
    } else if(strcmp(path, "/") == 0) {
        log_msg("\n**************\nINSIDE GETATTR\n**************\n");
        statbuf->st_mode = S_IFDIR|0666;
        statbuf->st_nlink = 1;
        statbuf->st_uid = 0;
        statbuf->st_gid = 0;
        statbuf->st_rdev = 0;
        statbuf->st_size = total_size;
        statbuf->st_atime = 0;
        statbuf->st_mtime = 0;
        statbuf->st_ctime = 0;
        log_stat(statbuf);
        return 0;
    } else {
        return -ENOENT;
    }
}

/** Change the size of a file */
int bb_truncate(const char *path, off_t newsize)
{
   return 0;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int bb_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    int fd;

    if(strcmp(path, "/file1") == 0 || strcmp(path, "/file2") == 0) {
        log_msg("\n**************\nINSIDE OPEN\n**************\n");
        log_msg("\nbb_open(path\"%s\", fi=0x%08x)\n",path, fi);
        fd = log_syscall("open", open(fs_image_path, fi->flags), 0);
        if (fd < 0) {
            retstat = log_error("open");
        }
        fi->fh = fd;
        log_fi(fi);
        return 0;
    } else {
        return -ENOENT;
    }
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
// I don't fully understand the documentation above -- it doesn't
// match the documentation for the read() system call which says it
// can return with anything up to the amount of data requested. nor
// with the fusexmp code which returns the amount of data also
// returned by read.
int bb_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;

    log_msg("\nbb_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
            path, buf, size, offset, fi);
        // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);

    if(strcmp(path, "/file1") == 0) {

        log_msg("\n**************\nINSIDE READ file1\n**************\n");
        if(offset > file1_size-1) {return 0;}
        if(offset+size > file1_size-1) {
            return log_syscall("pread", pread(fi->fh, buf, file1_size-offset, offset), 0);
        } else {
            return log_syscall("pread", pread(fi->fh, buf, size, offset), 0);
        }
        
    } else if(strcmp(path, "/file2") == 0) {

        log_msg("\n**************\nINSIDE READ file2\n**************\n");
        offset += file1_size;
        if(offset > total_size-1) {return 0;}
        if(offset+size > total_size-1) {
            return log_syscall("pread", pread(fi->fh, buf, total_size-offset, offset), 0);
        } else {
            return log_syscall("pread", pread(fi->fh, buf, size, offset), 0);
        }

    } else {
        log_msg("\n**************\nINSIDE READ invalid file\n**************\n");
        return -ENOENT;
    }

}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
// As  with read(), the documentation above is inconsistent with the
// documentation for the write() system call.
int bb_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    log_msg("\nbb_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",path, buf, size, offset, fi);
    log_fi(fi);
    if(strcmp(path, "/file1") == 0) {
        log_msg("\n**************\nINSIDE WRITE file1\n**************\n");
        if(offset > file1_size-1) {return 0;}
        if(offset+size > file1_size-1) {
            return log_syscall("pwrite", pwrite(fi->fh, buf, file1_size-offset, offset), 0);
        } else {
            return log_syscall("pwrite", pwrite(fi->fh, buf, size, offset), 0);
        }
    } else if(strcmp(path, "/file2") == 0) {
        log_msg("\n**************\nINSIDE WRITE file2\n**************\n");
        offset += file1_size;
        if(offset > total_size-1) {return 0;}
        if(offset+size > total_size-1) {
            return log_syscall("pwrite", pwrite(fi->fh, buf, total_size-offset, offset), 0);
        } else {
            return log_syscall("pwrite", pwrite(fi->fh, buf, size, offset), 0);
        }
    } else {
        log_msg("\n**************\nINSIDE WRITE invalid file\n**************\n");
        return -ENOENT;
    }
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */

int bb_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    if(strcmp(path, "/") == 0) {
        filler(buf, "file1", NULL, 0);
        filler(buf, "file2", NULL, 0);
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        return 0;
    }
    // if(filler(buf, "file1", NULL, 0) != 0) {
    //     log_msg("    ERROR bb_readdir filler:  buffer full");
    //     return -ENOMEM;
    // }
    // if(filler(buf, "file2", NULL, 0) != 0) {
    //     log_msg("    ERROR bb_readdir filler:  buffer full");
    //     return -ENOMEM;
    // }
    log_msg("WRONG READING OTHER DIRECTORY");
    return -ENOENT;
}


/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
// Not implemented.  I had a version that used creat() to create and
// open the file, which it turned out opened the file write-only.

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 */
int bb_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    return 0;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int bb_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    if(strcmp(path, "/file1") == 0 || strcmp(path, "/file2") == 0) {
        log_msg("\n**************\nINSIDE GETATTR\n**************\n");
        statbuf->st_mode = S_IFREG|0666;
        statbuf->st_nlink = 1;
        statbuf->st_uid = 0;
        statbuf->st_gid = 0;
        statbuf->st_rdev = 0;
        statbuf->st_size = file1_size;
        statbuf->st_atime = 0;
        statbuf->st_mtime = 0;
        statbuf->st_ctime = 0;
        log_stat(statbuf);
        return 0;
    } else if(strcmp(path, "/") == 0) {
        log_msg("\n**************\nINSIDE GETATTR\n**************\n");
        statbuf->st_mode = S_IFDIR|0666;
        statbuf->st_nlink = 1;
        statbuf->st_uid = 0;
        statbuf->st_gid = 0;
        statbuf->st_rdev = 0;
        statbuf->st_size = total_size;
        statbuf->st_atime = 0;
        statbuf->st_mtime = 0;
        statbuf->st_ctime = 0;
        log_stat(statbuf);
        return 0;
    } else {
        return -ENOENT;
    }
}

struct fuse_operations bb_oper = {
  .getattr = bb_getattr,
  .readlink = NULL,
  // no .getdir -- that's deprecated
  .getdir = NULL,
  .mknod = NULL,
  .mkdir = NULL,
  .unlink = NULL,
  .rmdir = NULL,
  .symlink = NULL,
  .rename = NULL,
  .link = NULL,
  .chmod = NULL,
  .chown = NULL,
  .truncate = bb_truncate,
  .utime = NULL,
  .open = bb_open,
  .read = bb_read,
  .write = bb_write,
  /** Just a placeholder, don't set */ // huh???
  .statfs = NULL,
  .flush = NULL,
  .release = NULL,
  .fsync = NULL,
  
// #ifdef HAVE_SYS_XATTR_H
  .setxattr = NULL,
  .getxattr = NULL,
  .listxattr = NULL,
  .removexattr = NULL,
// #endif
  
  .opendir = NULL,
  .readdir = bb_readdir,
  .releasedir = NULL,
  .fsyncdir = NULL,
  .init = NULL,
  .destroy = NULL,
  .access = NULL,
  .ftruncate = NULL,
  .fgetattr = bb_fgetattr
};

void bb_usage()
{
    fprintf(stderr, "usage:  bbfs [FUSE and mount options] rootDir mountPoint\n");
    abort();
}

int calcFileSize(char * path) {
    FILE * fp = fopen(path, "r");
    if (fp == NULL) { 
        printf("File Not Found!\n"); 
        return -1; 
    }
    fseek(fp, 0L, SEEK_END);
    int res = ftell(fp);
    fclose(fp);
    return res;
}

int main(int argc, char *argv[])
{
    int fuse_stat;
    struct bb_state *bb_data;

    // bbfs doesn't do any access checking on its own (the comment
    // blocks in fuse.h mention some of the functions that need
    // accesses checked -- but note there are other functions, like
    // chown(), that also need checking!).  Since running bbfs as root
    // will therefore open Metrodome-sized holes in the system
    // security, we'll check if root is trying to mount the filesystem
    // and refuse if it is.  The somewhat smaller hole of an ordinary
    // user doing it with the allow_other flag is still there because
    // I don't want to parse the options string.
    if ((getuid() == 0) || (geteuid() == 0)) {
    	fprintf(stderr, "Running BBFS as root opens unnacceptable security holes\n");
    	return 1;
    }

    // See which version of fuse we're running
    fprintf(stderr, "Fuse library version %d.%d\n", FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION);
    
    // Perform some sanity checking on the command line:  make sure
    // there are enough arguments, and that neither of the last two
    // start with a hyphen (this will break if you actually have a
    // rootpoint or mountpoint whose name starts with a hyphen, but so
    // will a zillion other programs)
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
	   bb_usage();

    bb_data = malloc(sizeof(struct bb_state));
    if (bb_data == NULL) {
    	perror("main calloc");
    	abort();
    }

    // Pull the rootdir out of the argument list and save it in my
    // internal data
    bb_data->rootdir = realpath(argv[argc-2], NULL);
    fs_image_path = bb_data->rootdir;
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    bb_data->logfile = log_open();

    total_size = calcFileSize(fs_image_path);
    file1_size = total_size / 2;
    file2_size = total_size / 2;
    
    printf("fs_image_path: %s\n", fs_image_path);
    printf("total_size: %d\n", total_size);

    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &bb_oper, bb_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    return fuse_stat;
}
