## Loading shared object files from memory

[![build](https://github.com/saleyn/memfd_create/actions/workflows/test.yaml/badge.svg)](https://github.com/saleyn/memfd_create/actions/workflows/test.yaml)

Typically shared objects are loaded from a file system using `dlopen(2)`.

This project illustrates how to load shared object from memory using
`memfd_create(2)` system call.  This is helpful when you want to store
your project's shared object dependencies in containers such as zip files,
remote Dropbox, AWS S3 buckets, Google Drive, or outside of a local file
system.

## Author

Serge Aleynikov <saleyn(at)gmail.com>
