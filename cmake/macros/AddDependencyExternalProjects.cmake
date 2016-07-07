macro(ADD_DEPENDENCY_EXTERNAL_PROJECTS)
    foreach(_PROJ_NAME ${ARGN})
        string(TOUPPER ${_PROJ_NAME} _PROJ_NAME_UPPER)
        # have we already detected we can't have this as external project on this OS?
        if (NOT DEFINED ${_PROJ_NAME_UPPER}_EXTERNAL_PROJECT OR ${_PROJ_NAME_UPPER}_EXTERNAL_PROJECT)
            # have we already setup the target?
            if (NOT TARGET ${_PROJ_NAME})
                add_subdirectory(${EXTERNAL_PROJECT_DIR}/${_PROJ_NAME} ${EXTERNALS_BINARY_DIR}/${_PROJ_NAME})
              
                # did we end up adding an external project target?
                if (NOT TARGET ${_PROJ_NAME})
                    set(${_PROJ_NAME_UPPER}_EXTERNAL_PROJECT FALSE CACHE BOOL "Presence of ${_PROJ_NAME} as external target")
                
                    message(STATUS "${_PROJ_NAME} was not added as an external project target for your OS."
                        " Either your system should already have the external library or you will need to install it separately.")
                else ()
                    add_dependencies(${_PROJ_NAME} SetupRelease SetupDebug)
                    set(${_PROJ_NAME_UPPER}_EXTERNAL_PROJECT TRUE CACHE BOOL "Presence of ${_PROJ_NAME} as external target")
                endif ()
            endif ()
          
        endif ()
    endforeach()
endmacro()
