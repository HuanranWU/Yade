IF(CLP_INCLUDE_DIR AND CLP2_INCLUDE_DIR AND CLP_LIBRARY AND CLP2_LIBRARY AND CLP3_LIBRARY)
    SET(CLP_FOUND TRUE)
ELSE(CLP_INCLUDE_DIR AND CLP2_INCLUDE_DIR AND CLP_LIBRARY AND CLP2_LIBRARY AND CLP3_LIBRARY)
	FIND_LIBRARY(CLP_LIBRARY NAMES Clp 
		PATHS
		/usr/lib/x86_64-linux-gnu
	    )

	FIND_LIBRARY(CLP2_LIBRARY NAMES OsiClp 
		PATHS
		/usr/lib/x86_64-linux-gnu
	    )

	FIND_LIBRARY(CLP3_LIBRARY NAMES CoinUtils
		PATHS
		/usr/lib/x86_64-linux-gnu
	    )


	FIND_PATH(
	      CLP_INCLUDE_DIR ClpSimplexDual.hpp
	    PATHS
	      /usr/include/coin
	    )

	FIND_PATH(
	      CLP2_INCLUDE_DIR coinutils.pc
	    PATHS
	      /usr/lib/x86_64-linux-gnu/pkgconfig
	    )
    
    IF(CLP_INCLUDE_DIR AND CLP2_INCLUDE_DIR AND CLP_LIBRARY AND CLP2_LIBRARY AND CLP3_LIBRARY)
        SET(CLP_FOUND TRUE)
        MESSAGE(STATUS "Found CLP: ${CLP_INCLUDE_DIR}, ${CLP_LIBRARY}")
    ELSE(CLP_INCLUDE_DIR AND CLP2_INCLUDE_DIR AND CLP_LIBRARY AND CLP2_LIBRARY AND CLP3_LIBRARY)
        SET(CLP_FOUND FALSE)
        MESSAGE(STATUS "CLP not found.")
    ENDIF(CLP_INCLUDE_DIR AND CLP2_INCLUDE_DIR AND CLP_LIBRARY AND CLP2_LIBRARY AND CLP3_LIBRARY)
    
    MARK_AS_ADVANCED(CLP_INCLUDE_DIR CLP2_INCLUDE_DIR CLP_LIBRARY CLP2_LIBRARY CLP3_LIBRARY)
ENDIF(CLP_INCLUDE_DIR AND CLP2_INCLUDE_DIR AND CLP_LIBRARY AND CLP2_LIBRARY AND CLP3_LIBRARY)

if(CLP_FOUND)
  file(READ "${CLP_INCLUDE_DIR}/ClpConfig.h" _clp_version_header)
  string(REGEX MATCH "define[ \t]+CLP_VERSION_MAJOR[ \t]+([0-9]+)" _clp_world_version_match "${_clp_version_header}")
  set(CLP_WORLD_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+CLP_VERSION_MINOR[ \t]+([0-9]+)" _clp_major_version_match "${_clp_version_header}")
  set(CLP_MAJOR_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+CLP_VERSION_RELEASE[ \t]+([0-9]+)" _clp_minor_version_match "${_clp_version_header}")
  set(CLP_MINOR_VERSION "${CMAKE_MATCH_1}")
  set(CLP_VERSION "${CLP_WORLD_VERSION}.${CLP_MAJOR_VERSION}.${CLP_MINOR_VERSION}")

  file(READ "${CLP_INCLUDE_DIR}/CoinUtilsConfig.h" _coinutils_version_header)
  string(REGEX MATCH "define[ \t]+COINUTILS_VERSION_MAJOR[ \t]+([0-9]+)" _coinutils_world_version_match "${_coinutils_version_header}")
  set(COINUTILS_WORLD_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+COINUTILS_VERSION_MINOR[ \t]+([0-9]+)" _coinutils_major_version_match "${_coinutils_version_header}")
  set(COINUTILS_MAJOR_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+COINUTILS_VERSION_RELEASE[ \t]+([0-9]+)" _coinutils_minor_version_match "${_coinutils_version_header}")
  set(COINUTILS_MINOR_VERSION "${CMAKE_MATCH_1}")
  set(COINUTILS_VERSION "${COINUTILS_WORLD_VERSION}.${COINUTILS_MAJOR_VERSION}.${COINUTILS_MINOR_VERSION}")

ENDIF(CLP_FOUND)

