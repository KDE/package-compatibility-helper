# Package Compatibility Helper

Provides support for running or finding alternatives to certain package types on immutable distributions.

Utilises Zorin's database for matching Windows executables for Flatpaks, and extracts AppStream metainfo from .rpm and .deb packages to match those to Flatpaks. 
If one can't be matched, it shows a generic message telling the user what to do. In the case of Windows executables, it shows an option to install or run Bottles (and in future, a few configurable choices of Wine layers).

Extensible for any mimetype - just implement `ICompatibilityHelper` and add a case to `CompatibilityHelperFactory`.

## Build and Installation Instructions

Can be built and installed with KDE Builder:
```
kde-builder package-compatibility-helper
```
