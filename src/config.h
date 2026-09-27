// jl-config.cfg parsing. Portable C (host unit-tested).
//   scan_dir_1=Games:Games          mandatory (scan_dir_2..4 optional)
//   inventory_file=S:jl-inventory.data
//   folder_mode_by_default=true
//   jst_command=jst                 command used to start a slave
#ifndef JL_CONFIG_H
#define JL_CONFIG_H

#include <stdint.h>

#define JL_MAX_SCAN_DIRS 4
#define JL_CFG_STR 108

typedef struct {
	char scanDirs[JL_MAX_SCAN_DIRS][JL_CFG_STR];
	uint8_t scanDirCount;
	char inventoryFile[JL_CFG_STR];
	char jstCommand[JL_CFG_STR];
	uint8_t folderModeByDefault;
	uint8_t launchMode;   // SPIKE: 0 script (v0.x way), 1 execute, 2 loadseg
} tJlConfig;

void jlConfigDefaults(tJlConfig *cfg);

// Applies every "key=value" line of text (len bytes). Unknown keys are ignored.
void jlConfigParse(tJlConfig *cfg, const char *text, uint32_t len);

#endif
