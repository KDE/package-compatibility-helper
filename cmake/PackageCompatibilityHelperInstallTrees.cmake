# SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>
# SPDX-License-Identifier: BSD-3-Clause

#[=======================================================================[.rst:
PackageCompatibilityHelperInstallTrees
--------------------------------------

Installs the integration trees (command shims, MIME handlers, binfmt
rules plus an installer config) shipped under ``trees/``.

::

  package_compatibility_helper_install_trees(TREES <name>... | all)

``TREES`` is a list of tree directory names under ``trees/``, or
``all`` to install every available tree.

Tree files are templates: ``*.in`` files get ``@KDE_INSTALL_*@`` macros
substituted so shims and binfmt rules point at the real install dirs
(Arch uses /usr/lib as libexecdir, not /usr/libexec). ``usr/bin`` and
``usr/libexec`` map to the configured install dirs; everything else
keeps its path relative to the prefix.
#]=======================================================================]

function(package_compatibility_helper_install_trees)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "" "TREES")

    if(NOT ARG_TREES)
        return()
    endif()

    if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr")
        message(WARNING
            "systemd only reads binfmt rules from /usr/lib/binfmt.d and "
            "shims must be on the default PATH, but CMAKE_INSTALL_PREFIX "
            "is '${CMAKE_INSTALL_PREFIX}'. Use DESTDIR for staged installs.")
    endif()

    set(_trees_root ${CMAKE_CURRENT_SOURCE_DIR}/trees)

    file(GLOB _tree_candidates RELATIVE ${_trees_root}
        CONFIGURE_DEPENDS ${_trees_root}/*)
    set(_available "")
    foreach(_entry IN LISTS _tree_candidates)
        if(IS_DIRECTORY ${_trees_root}/${_entry})
            list(APPEND _available ${_entry})
        endif()
    endforeach()

    if(ARG_TREES STREQUAL "all")
        set(_selected ${_available})
    else()
        set(_selected ${ARG_TREES})
    endif()

    foreach(tree IN LISTS _selected)
        set(_tree_dir ${_trees_root}/${tree})
        if(NOT IS_DIRECTORY ${_tree_dir}/usr)
            message(FATAL_ERROR
                "Tree '${tree}' does not exist or has no usr/ directory. "
                "Available trees: ${_available}")
        endif()
        message(STATUS "Installing tree: ${tree}")

        file(GLOB_RECURSE _tree_files RELATIVE ${_tree_dir}
            CONFIGURE_DEPENDS ${_tree_dir}/usr/*)
        foreach(_file IN LISTS _tree_files)
            set(_src ${_tree_dir}/${_file})
            set(_rel ${_file})
            if(_rel MATCHES "\\.in$")
                string(REGEX REPLACE "\\.in$" "" _rel "${_rel}")
                set(_configured ${CMAKE_CURRENT_BINARY_DIR}/trees/${tree}/${_rel})
                configure_file(${_src} ${_configured} @ONLY)
                set(_src ${_configured})
            endif()
            get_filename_component(_subdir ${_rel} DIRECTORY)
            if(_subdir STREQUAL "usr/bin")
                install(PROGRAMS ${_src} DESTINATION ${KDE_INSTALL_BINDIR})
            elseif(_subdir STREQUAL "usr/libexec")
                install(PROGRAMS ${_src} DESTINATION ${KDE_INSTALL_LIBEXECDIR})
            else()
                string(REGEX REPLACE "^usr/" "" _dest "${_subdir}")
                if(_dest STREQUAL "")
                    set(_dest .)
                endif()
                install(FILES ${_src} DESTINATION ${_dest})
            endif()
        endforeach()
    endforeach()
endfunction()
