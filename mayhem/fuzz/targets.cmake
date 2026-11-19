# Mayhem fuzz targets: upstream's tests/libfuzzer harnesses plus the ones in this directory.
# Each is linked twice: <name> (libFuzzer) and <name>-standalone (run-once reproducer).
# Included (deferred) into the top-level directory by inject.cmake.

add_library(mayhem_standalone_main OBJECT $ENV{STANDALONE_FUZZ_MAIN})
set_source_files_properties($ENV{STANDALONE_FUZZ_MAIN} PROPERTIES LANGUAGE C)
add_library(mayhem_lsan_off OBJECT ${MAYHEM_LSAN_OFF})

set(UPSTREAM_FUZZ ${CMAKE_SOURCE_DIR}/tests/libfuzzer)

macro(mayhem_fuzzer NAME)
	foreach(variant fuzz standalone)
		if(variant STREQUAL "fuzz")
			set(tgt ${NAME})
		else()
			set(tgt ${NAME}-standalone)
		endif()
		add_executable(${tgt} ${ARGN} $<TARGET_OBJECTS:mayhem_lsan_off>)
		target_link_libraries(${tgt} veyon-core)
		# upstream init.cpp relies on the PCH (PrecompiledHeader.h) + VeyonCore.h; PCH is off in the Mayhem build
		target_compile_options(${tgt} PRIVATE "SHELL:-include PrecompiledHeader.h" "SHELL:-include VeyonCore.h")
		set_target_properties(${tgt} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/mayhem-bin)
		if(variant STREQUAL "fuzz")
			target_compile_options(${tgt} PRIVATE "-fsanitize=fuzzer")
			target_link_options(${tgt} PRIVATE "-fsanitize=fuzzer")
		else()
			target_sources(${tgt} PRIVATE $<TARGET_OBJECTS:mayhem_standalone_main>)
		endif()
	endforeach()
endmacro()

# upstream harnesses (tests/libfuzzer/core/*)
mayhem_fuzzer(variantstream ${UPSTREAM_FUZZ}/core/variantstream/main.cpp ${UPSTREAM_FUZZ}/common/init.cpp)
# ported copy: also decodes the payload with read() after receive(), as every real consumer does
mayhem_fuzzer(variantarraymessage ${MAYHEM_FUZZ_DIR}/variantarraymessage.cpp ${UPSTREAM_FUZZ}/common/init.cpp)
# ported copy: state byte reduced into range (upstream harness trips UBSan on its own enum cast)
mayhem_fuzzer(vncclientprotocol ${MAYHEM_FUZZ_DIR}/vncclientprotocol.cpp ${UPSTREAM_FUZZ}/common/init.cpp)
# upstream main.cpp no longer compiles (constructor now takes QTcpSocket*); ported copy
mayhem_fuzzer(vncserverprotocol ${MAYHEM_FUZZ_DIR}/vncserverprotocol.cpp ${UPSTREAM_FUZZ}/common/init.cpp)

# new harnesses
mayhem_fuzzer(featuremessage ${MAYHEM_FUZZ_DIR}/featuremessage.cpp ${UPSTREAM_FUZZ}/common/init.cpp)
mayhem_fuzzer(configimport ${MAYHEM_FUZZ_DIR}/configimport.cpp ${UPSTREAM_FUZZ}/common/init.cpp)
mayhem_fuzzer(hostaddress ${MAYHEM_FUZZ_DIR}/hostaddress.cpp ${UPSTREAM_FUZZ}/common/init.cpp)
mayhem_fuzzer(serverauth ${MAYHEM_FUZZ_DIR}/serverauth.cpp
	${CMAKE_SOURCE_DIR}/server/src/ServerAuthenticationManager.cpp
	${CMAKE_SOURCE_DIR}/server/src/ServerAuthenticationManager.h
	${UPSTREAM_FUZZ}/common/init.cpp)
foreach(t serverauth serverauth-standalone)
	target_include_directories(${t} PRIVATE ${CMAKE_SOURCE_DIR}/server/src)
endforeach()
mayhem_fuzzer(networkobjectdirectory ${MAYHEM_FUZZ_DIR}/networkobjectdirectory.cpp
	${CMAKE_SOURCE_DIR}/plugins/builtindirectory/BuiltinDirectory.cpp
	${CMAKE_SOURCE_DIR}/plugins/builtindirectory/BuiltinDirectory.h
	${UPSTREAM_FUZZ}/common/init.cpp)
foreach(t networkobjectdirectory networkobjectdirectory-standalone)
	target_include_directories(${t} PRIVATE ${CMAKE_SOURCE_DIR}/plugins/builtindirectory)
endforeach()
# `veyon-cli networkobjects import`: the BuiltinDirectory plugin (loaded by the plugin manager from
# build-fuzz/lib/veyon) handles the import; only its configuration proxy header is needed here.
mayhem_fuzzer(networkobjectimport ${MAYHEM_FUZZ_DIR}/networkobjectimport.cpp ${UPSTREAM_FUZZ}/common/init.cpp)
foreach(t networkobjectimport networkobjectimport-standalone)
	target_include_directories(${t} PRIVATE ${CMAKE_SOURCE_DIR}/plugins/builtindirectory)
endforeach()
