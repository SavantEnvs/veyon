# Loaded via -DCMAKE_PROJECT_veyon_INCLUDE (mayhem/build.sh, fuzz build only): once upstream's
# top-level CMakeLists.txt has defined every target, include targets.cmake so the harnesses link
# the real veyon-core target (same defines/AUTOMOC as upstream's own fuzzers).
if(NOT MAYHEM_FUZZ_INJECTED)
	set(MAYHEM_FUZZ_INJECTED TRUE)
	set(MAYHEM_FUZZ_DIR ${CMAKE_CURRENT_LIST_DIR})
	cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR}
		CALL include ${MAYHEM_FUZZ_DIR}/targets.cmake)
endif()
