set(COMGEOM_V5_MODELS
  ADA073408.comgeom
)

macro(ADD_COMGEOM_V5_G_TARGET comgeom_model folder)
  string(REGEX REPLACE "([0-9a-z]*).comgeom" "\\1" comgeom_model_root "${comgeom_model}")
  set(output_file ${bin_root}/${DATA_DIR}/db/${comgeom_model_root}.g)
  add_custom_command(
    OUTPUT ${output_file}
    COMMAND ${CMAKE_COMMAND} -E remove ${output_file}
    COMMAND comgeom-g -v 5 ${CMAKE_CURRENT_SOURCE_DIR}/${comgeom_model} ${output_file}
    DEPENDS comgeom-g ${CMAKE_CURRENT_SOURCE_DIR}/${comgeom_model}
  )
  add_custom_target(${comgeom_model_root}.g ALL DEPENDS ${output_file})
  set_target_properties(${comgeom_model_root}.g PROPERTIES FOLDER "${folder}")
  if(BRLCAD_INSTALL_EXAMPLE_GEOMETRY)
    if(NOT "${CMAKE_CFG_INTDIR}" STREQUAL "." AND CMAKE_CONFIGURATION_FILES)
      string(REPLACE "${CMAKE_CFG_INTDIR}" "\${BUILD_TYPE}" output_file "${output_file}")
    endif(NOT "${CMAKE_CFG_INTDIR}" STREQUAL "." AND CMAKE_CONFIGURATION_FILES)
    install(FILES ${output_file} DESTINATION ${DATA_DIR}/db)
  endif(BRLCAD_INSTALL_EXAMPLE_GEOMETRY)
  set(BUILT_MODELS "${BUILT_MODELS};${output_file}")
endmacro(ADD_COMGEOM_V5_G_TARGET comgeom_model folder)

foreach(comgeom_input ${COMGEOM_V5_MODELS})
  ADD_COMGEOM_V5_G_TARGET(${comgeom_input} "BRL-CAD Geometry Models/Sample")
endforeach(comgeom_input ${COMGEOM_V5_MODELS})
CMAKEFILES(${COMGEOM_V5_MODELS})

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${BUILT_MODELS}")

CMAKEFILES(
  README_ADA073408
  ADA073408_inputs.tar.gz
)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
