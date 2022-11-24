# Overview

This repository provides code used to integrate FBOSS onto the following Cisco products:

	8101-32FH-O-BMC
	85-SCM-O-BMC

This repository is merely a snapshot of source code that will be published independently in the future. As such, PRs will be limited to emergency fixes only, as the snapshot will be completely overwritten by future releases.  The long term goal is to publish the underlying source code (such as cisco-8000-bsp) independently and have FBOSS reference that code directly.  As such, the lifetime of this repository will ideally be short.

This code builds with gcc-11 and clang-12, using c++20 standard.

Support requests should be sent to ospo-kmod@cisco.com. We will publish new snapshots periodically, including bug fixes, new features, and support for additional Cisco 8000 products.

## Build
#### Environment
   Centos 8 
   
#### Prerequisite Packages:
- ###### gcc-toolset-11
- ###### gflags
- ###### gflags-devel
- ###### CMake 3.18 or higher

### Build Procedure
1. Create  directory for the cloned repository and build artifacts:
```console
        $ mkdir cisco-open
        $ cd cisco-open
```    
2. Clone this repository 
```console
        $ git clone git@github.com:cisco-open/meta-fboss-export.git
        $ cd meta-fboss-export
```
3. Create a directory for the build artifacts
```console
        $ mkdir obj
        $ cd obj
```

4. Export the following environment variables if the required packages are already available on the system. Package shall be downloaded from the GitHub if an environment variable is not provided.
```console
      GLOG_PATH
      NLOHMANN_JSON_PATH
      HOWARDHINNAN_DATE_PATH
      FBOSS_PATH
```

        eg.
```console
    export GLOG_PATH=/usr
    export NLOHMANN_JSON_PATH=/cisco/export/pull_form_upstream/external_repos/json/
    export HOWARDHINNAN_DATE_PATH=/cisco/export/pull_form_upstream/external_repos/date-3.0.1/
    export FBOSS_PATH=/cisco/export/pull_form_upstream/external_repos/fboss/
```

5. Generate makefiles with CMake
```console
        $ cmake ../
```   
6. Initiate the build
```console
        $ make
```   


## Release Notes

Release notes shall be found under RELEASE-NOTES.txt
