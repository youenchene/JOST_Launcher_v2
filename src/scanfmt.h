// What the scanner writes: which files are slaves and how they are named.
// Portable C (host unit-tested).
#ifndef JL_SCANFMT_H
#define JL_SCANFMT_H

#include <stdint.h>

// True for "*.slave" (any case).
int jlIsSlaveFile(const char *name);

// True for directories the scanner skips ("data": a game's own files).
int jlIsSkippedDir(const char *name);

// Formats one inventory line "name;path;file\n" into out (cap bytes).
// The name is the slave's directory; the 2nd, 3rd... slave of a directory is
// "dir (file)", as in v0.x. A slave at the top of a scan dir is named after
// its file. Returns the length, or 0 if it doesn't fit.
uint16_t jlFormatLine(char *out, uint16_t cap, const char *dirName, const char *dirPath,
	const char *file, uint16_t nthInDir);

#endif
