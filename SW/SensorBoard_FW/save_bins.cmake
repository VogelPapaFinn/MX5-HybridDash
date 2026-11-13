# Copies the build outputs into OUT_DIR/<timestamp>_<elf sha>
# The sha matches "ELF file SHA256" in the boot log and the app SHA256 in coredump errors

file(SHA256 ${BUILD_DIR}/${APP_NAME}.elf ELF_SHA)
string(SUBSTRING ${ELF_SHA} 0 9 ELF_SHA)

# Skip if this exact build was already saved (e.g. "idf.py flash" without code changes)
file(GLOB ALREADY_SAVED ${OUT_DIR}/*_${ELF_SHA})
if(ALREADY_SAVED)
	return()
endif()

string(TIMESTAMP NOW "%Y-%m-%d_%H-%M-%S")
set(TARGET_DIR ${OUT_DIR}/${NOW}_${ELF_SHA})

file(GLOB APP_BINS ${BUILD_DIR}/*.bin)
file(COPY
	${APP_BINS}
	${BUILD_DIR}/${APP_NAME}.elf
	${BUILD_DIR}/bootloader/bootloader.bin
	${BUILD_DIR}/partition_table/partition-table.bin
	DESTINATION ${TARGET_DIR})

message(STATUS "Saved binaries to ${TARGET_DIR}")
