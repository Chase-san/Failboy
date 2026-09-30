# Renders FRAMES frames of ROM headless and checks the last one against a SHA256 of the expected PGM.
#
#   cmake -DFAILBOY=<exe> -DROM=<rom> -DFRAMES=<n> -DOUT=<file.pgm> -DEXPECTED=<sha256> -P frame_test.cmake

file(REMOVE "${OUT}")
execute_process(
	COMMAND "${FAILBOY}" --headless --frames ${FRAMES} --dump "${OUT}" "${ROM}"
	RESULT_VARIABLE status)
if(NOT EXISTS "${OUT}")
	message(FATAL_ERROR "failboy didn't write ${OUT} (exit status ${status})")
endif()

file(SHA256 "${OUT}" actual)
if(NOT actual STREQUAL EXPECTED)
	message(FATAL_ERROR "${OUT} doesn't match the reference image (its SHA256 is ${actual})")
endif()
