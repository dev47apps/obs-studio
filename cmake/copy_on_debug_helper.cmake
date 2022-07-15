string(REGEX REPLACE "\\.(dll|exe)$" ".pdb" FNAME "${FNAME}")

if(CONFIG STREQUAL Debug OR CONFIG STREQUAL RelWithDebInfo)
	if (EXISTS ${INPUT}/${FNAME})
		file(COPY "${INPUT}/${FNAME}" DESTINATION "${OUTPUT}")
	endif()
elseif(EXISTS "${OUTPUT}/${FNAME}")
	file(REMOVE "${OUTPUT}/${FNAME}")
endif()
