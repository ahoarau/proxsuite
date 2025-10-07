# gersemi: off
cmake_minimum_required(VERSION 3.22..4.1)

# Usage: require_variable(<var> [<message>])
# Example: require_variable(MY_VAR "MY_VAR must be set to build this project")
# Example: require_variable(MY_VAR) # Will print "MY_VAR is not defined."
function(require_variable var)
    if(NOT DEFINED ${var})
        if(ARGC EQUAL 1)
            set(msg "Required variable '${ARGV0}' is not defined.")
        else()
            set(msg "${ARGV1}.")
        endif()
        message(FATAL_ERROR "${msg}")
    endif()
endfunction()

# Usage: xxx_configure_default_build_type(<default_build_type>)
# Valid values for <default_build_type> are: Debug, Release, MinSizeRel, RelWithDebInfo
# Example: xxx_configure_default_build_type(RelWithDebInfo)
function(xxx_configure_default_build_type default_build_type)
    set(allowed_build_types
        Debug
        Release
        MinSizeRel
        RelWithDebInfo
    )
    if(NOT default_build_type IN_LIST allowed_build_types)
        message(FATAL_ERROR "Invalid build type: ${default_build_type}, valid values are: ${allowed_build_types}")
    endif()

    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
        set(CMAKE_BUILD_TYPE ${default_build_type} CACHE STRING "Choose the type of build." FORCE)
        # set the possible values of build type for cmake-gui
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${allowed_build_types})
    endif()
endfunction()

function(xxx_configure_default_binary_dirs)
    # On windows, librairies (.dll, .lib) and executables (.exe) are in the same directory
    # On unix, we separate them in bin/ and lib/
    if(WIN32)
        set(bin_dir ${CMAKE_BINARY_DIR})
        set(lib_dir ${CMAKE_BINARY_DIR})
    else()
        set(bin_dir ${CMAKE_BINARY_DIR}/bin)
        set(lib_dir ${CMAKE_BINARY_DIR}/lib)
    endif()

    # doc: https://cmake.org/cmake/help/latest/variable/CMAKE_RUNTIME_OUTPUT_DIRECTORY.html
    # Sets the default output directory for runtime (.exe, .dll) and archive (.lib, .a) targets.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${bin_dir} CACHE PATH "" INTERNAL)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${lib_dir} CACHE PATH "" INTERNAL)

    set(config Debug Release RelWithDebInfo MinSizeRel)
    foreach(conf ${config})
        string(TOUPPER ${conf} conf_upper)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${conf_upper} ${bin_dir} CACHE PATH "Output directory for runtime targets in ${conf} configuration." INTERNAL)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${conf_upper} ${lib_dir} CACHE PATH "Output directory for library targets in ${conf} configuration." INTERNAL)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${conf_upper} ${lib_dir} CACHE PATH "Output directory for archive targets in ${conf} configuration." INTERNAL)
    endforeach()

    set(CMAKE_BINARY_BINDIR ${bin_dir} CACHE PATH "Directory for building executables" INTERNAL)
    set(CMAKE_BINARY_LIBDIR ${lib_dir} CACHE PATH "Directory for building libraries" INTERNAL)
endfunction()

function(xxx_configure_default_install_dirs)
    include(GNUInstallDirs)
    # On Windows, libraries are installed in the same directory as executables
    if(WIN32)
        set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_BINDIR} CACHE PATH "Installation directory for dlls" FORCE)
    endif()
endfunction()


function(xxx_configure_default_install_prefix default_install_prefix)
    if(NOT default_install_prefix)
        message(FATAL_ERROR "Use: xxx_configure_default_install_prefix(<default_install_prefix>)")
    endif()

    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        message(STATUS "Setting default install prefix to '${default_install_prefix}'")
        set(CMAKE_INSTALL_PREFIX ${default_install_prefix} CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
        mark_as_advanced(CMAKE_INSTALL_PREFIX)
    endif()
endfunction()

function(xxx_target_generate_config_header target_name)
    require_variable(PROJECT_NAME "PROJECT_NAME must be defined before calling xxx_target_generate_config_header")
    require_variable(PROJECT_VERSION "PROJECT_VERSION must be defined before calling xxx_target_generate_config_header")
    require_variable(PROJECT_VERSION_MAJOR "PROJECT_VERSION_MAJOR must be defined before calling xxx_target_generate_config_header")
    require_variable(PROJECT_VERSION_MINOR "PROJECT_VERSION_MINOR must be defined before calling xxx_target_generate_config_header")
    require_variable(PROJECT_VERSION_PATCH "PROJECT_VERSION_PATCH must be defined before calling xxx_target_generate_config_header")
    require_variable(CMAKE_BINARY_DIR "CMAKE_BINARY_DIR must be defined before calling xxx_target_generate_config_header")
    require_variable(CMAKE_INSTALL_INCLUDEDIR "CMAKE_INSTALL_INCLUDEDIR must be defined before calling xxx_target_generate_config_header with INSTALL option")

    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target ${target_name} does not exist.")
    endif()

    # The generated header file location in the build directory
    set(output_file ${CMAKE_BINARY_DIR}/generated/include/${PROJECT_NAME}/config.hpp)

    # The default install location
    set(install_location ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

    # We need PROJECT_NAME in uppercase to match the maestro convention for macro names
    string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPERCASE)

    # ref: https://cmake.org/cmake/help/latest/variable/CMAKE_CURRENT_FUNCTION_LIST_DIR.html
    set(input_file ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/config.hpp.in)

    if(NOT EXISTS ${input_file})
        message(FATAL_ERROR "Input file ${input_file} does not exist.")
    endif()

    configure_file(${input_file} ${output_file} @ONLY)

    get_target_property(type ${target_name} TYPE)
    set(visibility PRIVATE)
    if(${type} STREQUAL "INTERFACE_LIBRARY")
        set(visibility INTERFACE)
    endif()

    target_include_directories(${target_name} ${visibility} $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated/include>)
    install(FILES ${output_file} DESTINATION ${install_location})
endfunction()

function(xxx_target_set_standard_compile_options target_name visibility)
    # visibility is either PRIVATE, PUBLIC or INTERFACE
    set(vs PRIVATE PUBLIC INTERFACE)
    if(NOT visibility IN_LIST vs)
        message(FATAL_ERROR "visibility must be one of PRIVATE, PUBLIC or INTERFACE")
    endif()
    
    # In CMake >= 3.26, use CMAKE_CXX_COMPILER_FRONTEND_VARIANT¶
    # ref: https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_FRONTEND_VARIANT.html
    # ref: https://gitlab.kitware.com/cmake/cmake/-/issues/19724
    if(CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
        set(CXX_COMPILER_ID "MSVC")
    endif()

    if(CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target_name} ${visibility}
            /W4 # Enable most warnings
            /wd4250 # "Inherits via dominance" - happens with diamond inheritance, not really an issue
            /wd4706 # assignment within conditional expression
            /wd5030 # pointer or reference to potentially throwing function used in noexcept context
            /wd4996 # function may be unsafe
            /we4834 # discarding return value of function with 'nodiscard' attribute
            /we4062 # enumerator 'xyz' in switch of enum 'abc' is not handled
        )
    elseif(CXX_COMPILER_ID STREQUAL "GNU" OR CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target_name} ${visibility}
            -Wall   # Enable most warnings
            -Wextra # Enable extra warnings
            -Wconversion # Warn on type conversions that may lose information
            -Wpedantic # Warn on non-standard C++ usage
        )
    endif()
endfunction()

function(xxx_target_enforce_msvc_conformance target_name)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target ${target_name} does not exist.")
    endif()

    get_target_property(type ${target_name} TYPE)
    set(visibility PRIVATE)
    if(${type} STREQUAL "INTERFACE_LIBRARY")
        set(visibility INTERFACE)
    endif()

    target_compile_options(${target_name} ${visibility}
        $<$<CXX_COMPILER_ID:MSVC>:
        /permissive-  # Standards conformance
        /Zc:__cplusplus # Needed to have __cplusplus set correctly
        /EHsc  # Enable C++ exceptions standard conformance
        /bigobj # To avoid "fatal error C1128: number of sections exceeded object file format limit"
        >
    )
endfunction()

function(xxx_target_treat_all_warnings_as_errors target_name visibility)
    # visibility is either PRIVATE, PUBLIC or INTERFACE
    set(vs PRIVATE PUBLIC INTERFACE)
    if(NOT visibility IN_LIST vs)
        message(FATAL_ERROR "visibility must be one of PRIVATE, PUBLIC or INTERFACE")
    endif()

    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target ${target_name} does not exist.")
    endif()

    if(CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target_name} ${visibility}
            /WX # Treat all warnings as errors
        )
    elseif(CXX_COMPILER_ID STREQUAL "GNU" OR CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target_name} ${visibility}
            -Werror # Treat all warnings as errors
        )
    endif()
endfunction()

function(xxx_append_global_property property_name value)
    get_property(prop GLOBAL PROPERTY ${property_name})
    if(NOT prop)
        set(prop "")
    endif()
    list(APPEND prop ${value})
    list(REMOVE_DUPLICATES prop)
    set_property(GLOBAL PROPERTY ${property_name} ${prop})
endfunction()

# Usage: xxx_find_package(<package> [version] [REQUIRED] [COMPONENTS ...] [EXPECTED_TARGETS <target1> <target2> ...])
# Example: xxx_find_package(Eigen3 3.4.0 CONFIG REQUIRED EXPECTED_TARGETS Eigen3::Eigen)
function(xxx_find_package)
    string(ASCII 27 Esc)
    message("${Esc}[1;34m" "[${ARGV0}]" "${Esc}[m")
    set(CMAKE_MESSAGE_INDENT "  ")
    message(DEBUG "Executing xxx_find_package with args ${ARGV}")

    set(options)
    set(oneValueArgs MODULE_PATH)
    set(multiValueArgs EXPECTED_TARGETS)
    cmake_parse_arguments(PARSE_ARGV 0 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")

    # If all targets are already available, skip the find_package call)
    set(all_targets_available True)
    foreach(target ${arg_EXPECTED_TARGETS})
        if(NOT TARGET ${target})
            set(all_targets_available False)
            break()
        endif()
    endforeach()
    if(all_targets_available AND arg_EXPECTED_TARGETS)
        message("All expected targets from package '${ARGV0}' are already available, skipping find_package call.")
        return()
    endif()

    # Pkg name is the first argument of find_package(<pkg_name> ...)
    set(package_name ${ARGV0})

    # Handle custom module file
    if(arg_MODULE_PATH)
        set(module_file "${arg_MODULE_PATH}/Find${package_name}.cmake")
        # check if file exists
        if(NOT EXISTS ${module_file})
            message(FATAL_ERROR "Custom module file ${module_file} does not exist.")
        endif()

        # Copy the module file to the generated cmake directory in the build dir
        file(COPY ${module_file} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME}/modules/${package_name})

        # Add the parent path to the CMAKE_MODULE_PATH
        list(APPEND CMAKE_MODULE_PATH ${arg_MODULE_PATH})
        message("Using custom module file: ${module_file}")
    endif()

    # Call find_package with the provided arguments
    string(REPLACE ";" " " fp_pp "${arg_UNPARSED_ARGUMENTS}")
    message("Executing find_package(${fp_pp})")

    # The actual call to find_package
    find_package(${arg_UNPARSED_ARGUMENTS})

    # TODO: handle QUIET

    # Check if the expected targets are available
    set(missing_targets "")
    foreach(target ${arg_EXPECTED_TARGETS})
        message("Checking for target '${target}'...")
        if(NOT TARGET ${target})
            list(APPEND missing_targets ${target})
            message("Checking for target '${target}'... ❌ not found.")
        else()
            message("Checking for target '${target}'... ✅ found.")
        endif()
    endforeach()
    if(missing_targets)
        string(REPLACE ";" ", " missing_targets "${missing_targets}")
        message(SEND_ERROR "The following expected targets from package '${package_name}' are missing: ${missing_targets}")
        return()
    endif()

    set_property(GLOBAL PROPERTY _xxx_${PROJECT_NAME}_packages_found "${package_name}" APPEND)
    
    set_property(GLOBAL PROPERTY _xxx_${package_name}_expected_targets "${arg_EXPECTED_TARGETS}")
    set_property(GLOBAL PROPERTY _xxx_${package_name}_find_package_args "${arg_UNPARSED_ARGUMENTS}")
    set_property(GLOBAL PROPERTY _xxx_${package_name}_module_path "${arg_MODULE_PATH}")

    # Save the link between the expected targets and the original package name
    foreach(target ${arg_EXPECTED_TARGETS})
        set_property(GLOBAL PROPERTY _xxx_${PROJECT_NAME}_${target}_package_name "${package_name}")
    endforeach()
endfunction()

function(xxx_print_dependency_summary)
    include(CMakePrintHelpers)

    # Get the list of packages found via xxx_find_package via the global property _xxx_project_packages
    get_property(packages GLOBAL PROPERTY _xxx_project_packages)
    if(NOT packages)
        message(STATUS "No dependencies found via xxx_find_package.")
        return()
    endif()

    message(STATUS "Dependencies found via xxx_find_package:")
    foreach(package_name ${packages})
        # Try to find the _xxx_<package_name>_expected_targets property
        get_property(expected_targets GLOBAL PROPERTY _xxx_${package_name}_expected_targets)
        if(NOT expected_targets)
            set(expected_targets "None")
        endif()

        # Replace ; by , for better readability
        string(REPLACE ";" " " expected_targets_pp "${expected_targets}")
        message(STATUS "    package [${package_name}] ==> targets [${expected_targets_pp}]")

        # Print target properties
        if(expected_targets STREQUAL "None")
            continue()
        endif()
        cmake_print_properties(TARGETS ${expected_targets} PROPERTIES 
            LOCATION
            INCLUDE_DIRECTORIES
            COMPILE_DEFINITIONS
            COMPILE_OPTIONS
            LINK_LIBRARIES
            LINK_OPTIONS
            INTERFACE_INCLUDE_DIRECTORIES
            INTERFACE_COMPILE_DEFINITIONS
            INTERFACE_COMPILE_OPTIONS
            INTERFACE_LINK_LIBRARIES
            INTERFACE_LINK_OPTIONS
        )
    endforeach()
endfunction()

function(xxx_export_dependencies)
    set(options)
    set(oneValueArgs EXPORT FILE DESTINATION)
    set(multiValueArgs TARGETS)
    cmake_parse_arguments(PARSE_ARGV 0 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")
    
    require_variable(arg_EXPORT)
    require_variable(arg_FILE)
    require_variable(arg_TARGETS)
    require_variable(arg_DESTINATION)

    set(all_link_libraries_only_targets "")
    foreach(target ${arg_TARGETS})
        # Note: On CMake 3.23, we have LINK_LIBRARIES_ONLY_TARGETS that might be useful
        set(ll "")
        get_target_property(interface_link_libraries ${target} INTERFACE_LINK_LIBRARIES)
        list(APPEND ll ${interface_link_libraries})

        get_target_property(link_libraries ${target} LINK_LIBRARIES)
        list(APPEND ll ${link_libraries})
        
        message("Linked libraries of target '${target}':
            LINK_LIBRARIES          : ${link_libraries}
            INTERFACE_LINK_LIBRARIES: ${interface_link_libraries}
        ")

        # Filter only targets
        set(link_libraries_only_targets "")
        foreach(l ${ll})
            if(TARGET ${l})
                list(APPEND link_libraries_only_targets ${l})
            endif()
        endforeach()

        list(APPEND all_link_libraries_only_targets ${link_libraries_only_targets})
    endforeach()

    message("All link libraries of targets '${arg_TARGETS}': ${link_libraries_only_targets}")

    set(packages_to_export "")
    foreach(target ${link_libraries_only_targets})
        get_property(package_name GLOBAL PROPERTY _xxx_${PROJECT_NAME}_${target}_package_name)

        if(NOT package_name)
            continue()
        endif()

        message("Library '${target}' comes from package '${package_name}'")
        list(APPEND packages_to_export ${package_name})
    endforeach()

    message("Packages to export for EXPORT ${arg_EXPORT}: ${packages_to_export}")

    set(modules "")
    set(fd "")
    foreach(package_name ${packages_to_export})
        get_property(expected_targets GLOBAL PROPERTY _xxx_${package_name}_expected_targets)
        get_property(find_package_args GLOBAL PROPERTY _xxx_${package_name}_find_package_args)
        get_property(module_path GLOBAL PROPERTY _xxx_${package_name}_module_path)

        require_variable(find_package_args "find_package_args must be defined for package ${package_name}")
        require_variable(expected_targets "expected_targets must be defined for package ${package_name}")

        string(REPLACE ";" " " find_package_args "${find_package_args}")

        # Custom Modules
        if(module_path)
            string(APPEND modules "list(APPEND CMAKE_MODULE_PATH \${CMAKE_CURRENT_LIST_DIR}/modules/${package_name})\n")
            install(
                FILES ${module_path}/Find${package_name}.cmake
                DESTINATION ${arg_DESTINATION}/modules/${package_name}
            )
        endif()

        # Find Dependencies
        if(NOT expected_targets)
            string(APPEND fd "find_dependency(${find_package_args})\n")
        else()
            set(cond "")
            foreach(target IN LISTS expected_targets)
                if(cond STREQUAL "")
                    set(cond "NOT TARGET ${target}")
                else()
                    set(cond "${cond} OR NOT TARGET ${target}")
                endif()
            endforeach()

            string(APPEND fd "if(${cond})\n")
            string(APPEND fd "    find_dependency(${find_package_args})\n")
            string(APPEND fd "endif()\n\n")
        endif()
    endforeach()

    set(xxx_modules ${modules})
    set(xxx_find_dependencies ${fd})
    
    configure_file(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/dependencies.cmake.in ${arg_FILE} @ONLY)
    
    install(
        FILES ${arg_FILE}
        DESTINATION ${arg_DESTINATION}
    )
endfunction()

function(xxx_cmake_module_version)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(PARSE_ARGV 0 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")

    include(CMakePackageConfigHelpers)
    require_variable(PROJECT_NAME)
    require_variable(PROJECT_VERSION)

    # NOTE: Expose as options if needed
    set(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME}/${PROJECT_NAME}-version.cmake)
    set(VERSION ${PROJECT_VERSION})     # <major.minor.patch>
    set(COMPATIBILITY AnyNewerVersion) # <AnyNewerVersion|SameMajorVersion|SameMinorVersion|ExactVersion>
    set(ARCH_INDEPENDENT "")
    set(DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

    write_basic_package_version_file(
      ${OUTPUT}
      VERSION ${VERSION}
      COMPATIBILITY ${COMPATIBILITY}
      ${ARCH_INDEPENDENT}
    )

    install(
        FILES ${OUTPUT}
        DESTINATION ${DESTINATION}
    )
endfunction()

function(xxx_declare_component)
    set(options)
    set(oneValueArgs COMPONENT)
    set(multiValueArgs TARGETS)
    cmake_parse_arguments(PARSE_ARGV 0 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")

    require_variable(PROJECT_NAME)
    require_variable(arg_TARGETS)
    require_variable(arg_COMPONENT)

    # Check component is not already declared
    get_property(components GLOBAL PROPERTY _xxx_${PROJECT_NAME}_components)
    if(${arg_COMPONENT} IN_LIST components)
        message(FATAL_ERROR "Component '${arg_COMPONENT}' is already declared for project '${PROJECT_NAME}'.")
    endif()
    
    message("Declaring component '${arg_COMPONENT}' with targets: ${arg_TARGETS}")
    set_property(GLOBAL PROPERTY _xxx_${PROJECT_NAME}_components ${arg_COMPONENT} APPEND)
    set_property(GLOBAL PROPERTY _xxx_${PROJECT_NAME}_${arg_COMPONENT}_targets ${arg_TARGETS})
endfunction()

function(xxx_cmake_module_config)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(PARSE_ARGV 0 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")

    include(CMakePackageConfigHelpers)
    require_variable(PROJECT_NAME)
    require_variable(CMAKE_INSTALL_LIBDIR)

    get_property(declared_components GLOBAL PROPERTY _xxx_${PROJECT_NAME}_components)
    if(NOT declared_components)
        message(FATAL_ERROR "No components declared for project '${PROJECT_NAME}'.")
    endif()

    # NOTE: Expose as options if needed
    set(INPUT ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/config.cmake.in)
    set(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake)
    set(DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
    set(NO_SET_AND_CHECK_MACRO "NO_SET_AND_CHECK_MACRO")
    set(NO_CHECK_REQUIRED_COMPONENTS_MACRO "NO_CHECK_REQUIRED_COMPONENTS_MACRO")
    set(NAMESPACE "${PROJECT_NAME}::")
    
    string(REPLACE ";" " " xxx_project_components "${declared_components}")
    configure_package_config_file(
      ${INPUT}
      ${OUTPUT}
      INSTALL_DESTINATION ${DESTINATION}
      ${NO_SET_AND_CHECK_MACRO}
      ${NO_CHECK_REQUIRED_COMPONENTS_MACRO}
    )
    install(
        FILES ${OUTPUT}
        DESTINATION ${DESTINATION}
    )

    foreach(component ${declared_components})
        message("Generating cmake module files for component '${component}'")
        
        get_property(targets GLOBAL PROPERTY _xxx_${PROJECT_NAME}_${component}_targets)
        require_variable(targets)

        # <package>-<component>-dependencies.cmake
        xxx_export_dependencies(
            TARGETS ${targets}
            EXPORT ${PROJECT_NAME}-${component}
            FILE ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME}/${PROJECT_NAME}-${component}-dependencies.cmake
            DESTINATION ${DESTINATION}
        )
        # Create the export for the component targets
        install(TARGETS ${targets} 
            EXPORT ${PROJECT_NAME}-${component}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
        # <package>-<component>-targets.cmake
        install(EXPORT ${PROJECT_NAME}-${component}
            FILE ${PROJECT_NAME}-${component}-targets.cmake
            NAMESPACE ${NAMESPACE}
            DESTINATION ${DESTINATION}
        )
        # HACK: Copy the generated targets file to the generated cmake directory, so that we can install all cmake files in one go
        # ref: https://github.com/Kitware/CMake/blob/master/Source/cmInstallExportGenerator.cxx#L50-L58
        string(MD5 destdir_hash ${DESTINATION})
        set(generated_target_file ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/Export/${destdir_hash}/${PROJECT_NAME}-${component}-targets.cmake)
        file(COPY ${generated_target_file} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME})
    endforeach()
endfunction()

function(xxx_cmake_module_files)
    xxx_cmake_module_config()
    xxx_cmake_module_version()
endfunction()

# function(xxx_install_target target_name)
#     require_variable(PROJECT_NAME "PROJECT_NAME must be defined before calling xxx_install_target")
#     require_variable(CMAKE_INSTALL_LIBDIR "CMAKE_INSTALL_LIBDIR must be defined before calling xxx_install_target")
#     require_variable(CMAKE_INSTALL_BINDIR "CMAKE_INSTALL_BINDIR must be defined before calling xxx_install_target")
#     require_variable(CMAKE_INSTALL_INCLUDEDIR "CMAKE_INSTALL_INCLUDEDIR must be defined before calling xxx_install_target")

#     set(options)
#     set(oneValueArgs EXPORT)
#     set(multiValueArgs DEPENDS_ON)
#     cmake_parse_arguments(PARSE_ARGV 0 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")

#     # Allow to skip find package
#     set(skip False)
#     foreach(cond ${arg_DEPENDS_ON})
#         if(NOT ${${cond}})
#             set(skip True)
#             break()
#         endif()
#     endforeach()
#     if(skip)
#         return()
#     endif()

#     # List the properties of the target
#     # get_target_property(type ${target_name} TYPE)
#     # get_target_property(link_libraries ${target_name} LINK_LIBRARIES)
#     # get_target_property(link_interface_libraries ${target_name} INTERFACE_LINK_LIBRARIES)
#     # message(FATAL_ERROR "link_libraries: ${link_libraries} | link_interface_libraries: ${link_interface_libraries}")

#     if(NOT TARGET ${target_name})
#         message(FATAL_ERROR "Target ${target_name} does not exist.")
#     endif()

#     if(NOT arg_EXPORT)
#         set(arg_EXPORT ${PROJECT_NAME}-targets)
#     endif()

#     install(TARGETS ${target_name}
#         EXPORT ${arg_EXPORT}
#         ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#         INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#     )
# endfunction()

# xxx_option(<option_name> <description> <default_value>)
# Example: xxx_option(BUILD_TESTING "Build the tests" ON)
# Override cmake option() to get a nice summary at the end of the configuration step
function(xxx_option option_name description default_value)
    require_variable(option_name)
    require_variable(description)
    require_variable(default_value)

    # The call to the original option()
    option(${ARGV})

    # Save the default value in a property
    set_property(GLOBAL PROPERTY _xxx_option_${option_name}_default_value ${default_value})

    # Save the option name in the list
    set_property(GLOBAL PROPERTY _xxx_project_option_names ${option_name} APPEND)
endfunction()

# Helper function: pad or truncate a string to a fixed width
function(pad_string input width output_var)
    string(LENGTH "${input}" _len)
    if(_len GREATER width)
        # Truncate if too long
        string(SUBSTRING "${input}" 0 ${width} _padded)
    else()
        # Pad with spaces until desired width
        math(EXPR _pad "${width} - ${_len}")
        set(_spaces "")
        while(_pad GREATER 0)
            string(APPEND _spaces " ")
            math(EXPR _pad "${_pad} - 1")
        endwhile()
        set(_padded "${input}${_spaces}")
    endif()
    set(${output_var} "${_padded}" PARENT_SCOPE)
endfunction()

function(xxx_print_option_summary)
    get_property(option_names GLOBAL PROPERTY _xxx_project_option_names)
    if(NOT option_names)
        message(STATUS "No options defined via xxx_option.")
        return()
    endif()

    # Prepare pretty output
    message( "")
    message( "================= Configuration Summary ======================================")
    message( "")    
    pad_string("Option"      40 _menu_option)
    pad_string("Type"        5  _menu_type)
    pad_string("Value"       8  _menu_value)
    pad_string("Default"     5  _menu_default)
    pad_string("Description (default)" 25 _menu_description)
    message( "${_menu_option} | ${_menu_type} | ${_menu_value} | ${_menu_description}")
    message( "------------------------------------------------------------------------------")

    foreach(option_name ${option_names})
        get_property(_type CACHE ${option_name} PROPERTY TYPE)
        get_property(_val CACHE ${option_name} PROPERTY VALUE)
        get_property(_default GLOBAL PROPERTY _xxxoption_nameion_${option_name}_default_value)
        get_property(_help CACHE ${option_name} PROPERTY HELPSTRING)

        pad_string("${option_name}"      40 _name)
        pad_string("${_type}"     5 _type)
        pad_string("${_val}"      8 _val)
        pad_string("${_default}"  5 _default)
        pad_string("${_help}"     25 _help)

        message( "${_name} | ${_type} | ${_val} | ${_help} (${_default})")
    endforeach()

    message( "----------------------------------------------------------")
    message( "")
endfunction()

# gersemi: on
