/**
 * Reads a line from a file (or input stream).
 * Stops, when EOF (end of file) reached, or if "\n",
 * the line-terminator is reached.
 *
 * @author phi@gress.ly
 * @date   2018-11-15
 * @host   https://github.com/pheek/line.h
 * This code lies under the GPL: http://www.gnu.org/licenses/gpl-3.0.en.html
 */

#ifndef  LINE_H
#define  LINE_H

// Type of sizes (espec. "size_t")
#include <stdlib.h>

// FILE - Type
#include <stdio.h>

/**
 *  Least number of chars per Line
 *  Buffer will be 1 char larger to hold the 0-terminator.
 */
#define INITIAL_LINE_SIZE  7

/**
 * Encrease Percentage if a line longer than the
 * actual Line-Capacity is reached.
 * Example:
 * If actual Buffer Size is 4 (line size = 3),
 * 200% = 8 will be added to 4; this makes
 * the new buffer hold 12 chars (incl. line terminator);
 * this is 11 chars for max line length.
 */
#define ENLARGE_PERCENTAGE 50

/**
 * get_line() reads a line into a BUFFER. A pointer to this Buffer
 *            is returned (see example "testLineReader.c".
 * @param filePointer a Pointer to stdin or another readable file/stream.
 * @return  Pointer to 0 termintat string
 *          or NULL (0, ZERO) if "end of file" reached.
 *          Pay attention, that usually all calls to "get_line()" will
 *          return the same Pointer, but with different content. So,
 *          If you want to hold the content for later use (eg. parsing), then
 *          you will have to clone the content to your own string.
 */
char* get_line(FILE *filePointer);


/** 
 * Call this, if you stop the reading.
 * The freeBuffer is done automatically after an EOF (end of file)
 * was reached.
 * Sometimes, you don't want to read a whole file, and then you must
 * free the buffer to give the heap-memory back to the system.
 */
void freeBuffer();

#endif
