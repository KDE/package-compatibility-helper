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

## Installing Flatpaks on first use

The helper can also install the Flatpak behind a command, file type or
binfmt interpreter the first time it is used. A tree under `trees/`
ships shims into `/usr`. The shims call
`package-compatibility-helper-run`, which starts
`package-compatibility-helper --install <name>` if the Flatpak is
missing (a wizard asks the user, adds the remote and installs it) and
then runs the real command. After the first install the shim calls the
Flatpak directly.

`--install` exits 0 if the Flatpak is installed, 1 if the user declined
or the installation failed, and 2 on bad usage or a broken config.
Configs live in `/usr/share/package-compatibility-helper/apps/`;
downstream consumers can ship their own or override them in
`/etc/package-compatibility-helper/apps/`, which takes precedence.

A config can also list comma-separated MIME types with `MimeTypes=` in its `[App]` section. The
graphical helper then offers that Flatpak as the compatibility tool
when a matching file is opened, installing it first if needed. This
also works for file types the helper has no dedicated support for:
a generic dialog with the usual "Open With…", "Get Help" and
"Install/Run with <tool>" actions is shown, and trees register the
helper as the handler for their MIME types. See `data/apps/wine.conf`,
which provides Wine for Windows executables.

Trees are only installed when selected with `-DPACKAGE_COMPATIBILITY_HELPER_TREES=<name>` (or
`all`), since they are intended for OS image builds. See `trees/kjar`
for an example that makes `java`, `javac` and `.jar` files work by
installing a JDK Flatpak, and `tools/new-tree.py` for scaffolding new
trees.
