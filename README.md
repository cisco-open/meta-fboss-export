This repository provides code used to integrate FBOSS
onto the following Cisco products:

	8101-32FH-O-BMC
	85-SCM-O-BMC

This repository is merely a snapshot of source code
that will be published independently in the future.
As such, PRs will be limited to emergency fixes only,
as the snapshot will be completely overwritten by
future releases.  The long term goal is to publish
the underlying source code (such as cisco-8000-bsp)
independently and have FBOSS reference that code
directly.  As such, the lifetime of this repository
will ideally be short.

This code builds with gcc-11 and clang-12, using
c++20 standard.

Support requests should be sent to ospo-kmod@cisco.com.
We will publish new snapshots periodically, including
bug fixes, new features, and support for additional
Cisco 8000 products.
