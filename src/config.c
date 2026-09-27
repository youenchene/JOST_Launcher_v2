#include "config.h"

static void copyField(char *dst, const char *src, uint32_t len) {
	if(len >= JL_CFG_STR) {
		len = JL_CFG_STR - 1;
	}
	for(uint32_t i = 0; i < len; ++i) dst[i] = src[i];
	dst[len] = '\0';
}

static int keyIs(const char *key, uint32_t len, const char *want) {
	uint32_t i = 0;
	for(; i < len && want[i]; ++i) {
		if(key[i] != want[i]) return 0;
	}
	return i == len && !want[i];
}

static int isTrue(const char *v, uint32_t len) {
	return (len == 1 && v[0] == '1') ||
		(len == 4 && (v[0] | 0x20) == 't' && (v[1] | 0x20) == 'r' &&
		 (v[2] | 0x20) == 'u' && (v[3] | 0x20) == 'e');
}

static void applyScanDir(tJlConfig *cfg, uint8_t slot, const char *v, uint32_t len) {
	while(len > 1 && v[len - 1] == '/') --len; // "Games:Games/" -> "Games:Games"
	copyField(cfg->scanDirs[slot], v, len);
	if(slot >= cfg->scanDirCount) cfg->scanDirCount = slot + 1;
}

static void applyLine(tJlConfig *cfg, const char *line, uint32_t len) {
	while(len && (line[len - 1] == ' ' || line[len - 1] == '\r' || line[len - 1] == '\t')) --len;
	uint32_t eq = 0;
	while(eq < len && line[eq] != '=') ++eq;
	if(eq == 0 || eq >= len || line[0] == ';' || line[0] == '#') {
		return;
	}
	const char *v = line + eq + 1;
	uint32_t vlen = len - eq - 1;
	if(eq == 10 && keyIs(line, 9, "scan_dir_") && line[9] >= '1' && line[9] < '1' + JL_MAX_SCAN_DIRS) {
		applyScanDir(cfg, (uint8_t)(line[9] - '1'), v, vlen);
	}
	else if(keyIs(line, eq, "inventory_file") && vlen) {
		copyField(cfg->inventoryFile, v, vlen);
	}
	else if(keyIs(line, eq, "jst_command") && vlen) {
		copyField(cfg->jstCommand, v, vlen);
	}
	else if(keyIs(line, eq, "launch_mode") && vlen) {
		cfg->launchMode = (uint8_t)(v[0] - '0');
	}
	else if(keyIs(line, eq, "folder_mode_by_default")) {
		cfg->folderModeByDefault = (uint8_t)isTrue(v, vlen);
	}
}

void jlConfigDefaults(tJlConfig *cfg) {
	for(uint8_t i = 0; i < JL_MAX_SCAN_DIRS; ++i) cfg->scanDirs[i][0] = '\0';
	cfg->scanDirCount = 0;
	copyField(cfg->inventoryFile, "S:jl-inventory.data", 19);
	copyField(cfg->jstCommand, "jst", 3);
	cfg->folderModeByDefault = 0;
	cfg->launchMode = 0;
}

void jlConfigParse(tJlConfig *cfg, const char *text, uint32_t len) {
	uint32_t start = 0;
	for(uint32_t i = 0; i <= len; ++i) {
		if(i == len || text[i] == '\n') {
			applyLine(cfg, text + start, i - start);
			start = i + 1;
		}
	}
}
