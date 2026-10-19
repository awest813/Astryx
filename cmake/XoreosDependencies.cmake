# xoreos - A reimplementation of BioWare's Aurora engine
#
# xoreos is the legal property of its developers, whose names
# can be found in the AUTHORS file distributed with this source
# distribution.
#
# xoreos is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 3
# of the License, or (at your option) any later version.
#
# xoreos is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with xoreos. If not, see <http://www.gnu.org/licenses/>.

# Turn a missing build dependency into a message that says what to install.
#
#   xoreos_check_dependency(<Name> <found>
#                           [OPTIONAL] WHAT <description>
#                           [DEBIAN <pkg>...] [FEDORA <pkg>...] [ARCH <pkg>...]
#                           [VCPKG <port>...])
#
#     Records the result of a find_package(). Pass the find's *_FOUND value
#     as <found>. A missing required dependency is reported (together with
#     every other missing one) by xoreos_report_missing_dependencies(); a
#     missing OPTIONAL one is listed by xoreos_report_disabled_features().
#
#   xoreos_report_missing_dependencies()
#     Stops configuration if any required dependency is missing, printing the
#     package names and a ready-to-paste install command for this system.
#
#   xoreos_report_disabled_features()
#     Prints one warning listing optional dependencies that were not found
#     and what was disabled because of it.

set_property(GLOBAL PROPERTY XOREOS_MISSING_REQUIRED "")
set_property(GLOBAL PROPERTY XOREOS_MISSING_OPTIONAL "")

# Which package family's names to use for the install command (DEBIAN,
# FEDORA, ARCH), or empty when unknown.
function(_xoreos_detect_package_family out_var)
  set(_family "")
  if(CMAKE_HOST_UNIX AND NOT CMAKE_HOST_APPLE AND EXISTS "/etc/os-release")
    file(STRINGS "/etc/os-release" _os_ids REGEX "^(ID|ID_LIKE)=")
    string(TOLOWER "${_os_ids}" _os_ids)
    if(_os_ids MATCHES "debian|ubuntu")
      set(_family DEBIAN)
    elseif(_os_ids MATCHES "fedora|rhel|centos")
      set(_family FEDORA)
    elseif(_os_ids MATCHES "arch|manjaro|endeavouros")
      set(_family ARCH)
    endif()
  endif()
  set(${out_var} "${_family}" PARENT_SCOPE)
endfunction()

function(xoreos_check_dependency NAME FOUND)
  cmake_parse_arguments(DEP "OPTIONAL" "WHAT" "DEBIAN;FEDORA;ARCH;VCPKG" ${ARGN})
  if(FOUND)
    return()
  endif()

  # Stash everything needed to describe this dependency later.
  set_property(GLOBAL PROPERTY "XOREOS_DEP_${NAME}_WHAT" "${DEP_WHAT}")
  foreach(_family DEBIAN FEDORA ARCH VCPKG)
    set_property(GLOBAL PROPERTY "XOREOS_DEP_${NAME}_${_family}" "${DEP_${_family}}")
  endforeach()

  if(DEP_OPTIONAL)
    set_property(GLOBAL APPEND PROPERTY XOREOS_MISSING_OPTIONAL "${NAME}")
  else()
    set_property(GLOBAL APPEND PROPERTY XOREOS_MISSING_REQUIRED "${NAME}")
  endif()
endfunction()

# Build the human-readable list of dependencies plus install hints.
function(_xoreos_describe_dependencies out_var)
  _xoreos_detect_package_family(_family)

  set(_text "")
  set(_install "")
  set(_vcpkg_ports "")
  set(_family_complete TRUE)
  foreach(_name ${ARGN})
    get_property(_what GLOBAL PROPERTY "XOREOS_DEP_${_name}_WHAT")
    get_property(_debian GLOBAL PROPERTY "XOREOS_DEP_${_name}_DEBIAN")
    get_property(_fedora GLOBAL PROPERTY "XOREOS_DEP_${_name}_FEDORA")
    get_property(_arch GLOBAL PROPERTY "XOREOS_DEP_${_name}_ARCH")
    get_property(_vcpkg GLOBAL PROPERTY "XOREOS_DEP_${_name}_VCPKG")

    foreach(_v _debian _fedora _arch)
      string(REPLACE ";" " " ${_v} "${${_v}}")
      if(NOT ${_v})
        set(${_v} "(no standard package)")
      endif()
    endforeach()
    string(REPLACE ";" " " _vcpkg_text "${_vcpkg}")
    if(NOT _vcpkg_text)
      set(_vcpkg_text "(no port; install from the system)")
    endif()

    string(APPEND _text "    ${_name} - ${_what}\n")
    string(APPEND _text "        Debian/Ubuntu: ${_debian}\n")
    string(APPEND _text "        Fedora:        ${_fedora}\n")
    string(APPEND _text "        Arch:          ${_arch}\n")
    string(APPEND _text "        vcpkg:         ${_vcpkg_text}\n")

    if(_family)
      get_property(_pkgs GLOBAL PROPERTY "XOREOS_DEP_${_name}_${_family}")
      if(_pkgs)
        list(APPEND _install ${_pkgs})
      else()
        set(_family_complete FALSE)
      endif()
    endif()
    list(APPEND _vcpkg_ports ${_vcpkg})
  endforeach()

  if(VCPKG_TOOLCHAIN)
    string(APPEND _text "  This build uses the vcpkg toolchain, so these normally come from vcpkg.json.\n")
    if(_vcpkg_ports)
      string(REPLACE ";" " " _vcpkg_ports "${_vcpkg_ports}")
      string(APPEND _text "  Check the vcpkg output above (or ${CMAKE_BINARY_DIR}/vcpkg-manifest-install.log)\n")
      string(APPEND _text "  for errors building: ${_vcpkg_ports}\n")
    endif()
    string(APPEND _text "  Dependencies without a vcpkg port must come from the system package manager.\n")
  endif()

  if(_install)
    list(REMOVE_DUPLICATES _install)
    string(REPLACE ";" " " _install "${_install}")
    if(_family STREQUAL "DEBIAN")
      set(_cmd "sudo apt-get install ${_install}")
    elseif(_family STREQUAL "FEDORA")
      set(_cmd "sudo dnf install ${_install}")
    elseif(_family STREQUAL "ARCH")
      set(_cmd "sudo pacman -S --needed ${_install}")
    endif()
    string(APPEND _text "  Install command for this system:\n      ${_cmd}\n")
    if(NOT _family_complete)
      string(APPEND _text "  (some of the above have no package here; see the list above)\n")
    endif()
  endif()

  set(${out_var} "${_text}" PARENT_SCOPE)
endfunction()

function(xoreos_report_missing_dependencies)
  get_property(_missing GLOBAL PROPERTY XOREOS_MISSING_REQUIRED)
  if(NOT _missing)
    return()
  endif()

  _xoreos_describe_dependencies(_text ${_missing})
  string(REPLACE ";" ", " _names "${_missing}")
  message(FATAL_ERROR
    "Missing required build dependencies: ${_names}\n"
    "${_text}"
    "  Full list and per-OS instructions: BUILDDEP.md\n"
    "  Linux helpers: scripts/build_ci_linux.sh --install-deps (system packages)\n"
    "                 scripts/bootstrap_linux.sh (self-contained vcpkg build)\n"
    "  Windows:       scripts/bootstrap_windows.ps1\n"
    "  After installing, re-run CMake (delete CMakeCache.txt if a stale path sticks).\n")
endfunction()

function(xoreos_report_disabled_features)
  get_property(_missing GLOBAL PROPERTY XOREOS_MISSING_OPTIONAL)
  if(NOT _missing)
    return()
  endif()

  _xoreos_describe_dependencies(_text ${_missing})
  string(REPLACE ";" ", " _names "${_missing}")
  message(WARNING
    "Optional dependencies not found, so their features are disabled: ${_names}\n"
    "${_text}"
    "  The build still works without them; install them and re-run CMake to enable.\n")
endfunction()
