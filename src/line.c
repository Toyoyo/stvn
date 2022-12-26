/**
 * Line-Reader (for any line length in the input files)
 * @author phi@gress.ly
 * @date   2018-11-15
 * @host   https://github.com/pheek/line.h
 * This code lies under the GPL: http://www.gnu.org/licenses/gpl-3.0.en.html
 */

// Don't include twice
#ifndef  LINE_H

// line.h contains the function header "get_line()" and constants.
#include "line.h"

#define FALSE  0
#define TRUE  -1

/**
 * The following buffer contains enough space to hold a
 * line of the file.
 * If the BUFFER should be too small, it will be enlarged
 * automatically, like a Java-ArrayList.
 */
char* BUFFER = NULL; // holds linesize + 1 // null terminator


/**
 * Size of the buffer in bytes.
 */
size_t BUFFSIZE = 0; // if "0", this means, the buffer is not yet allocated

      // return this empty line, if '\n' found at last line.
char * LASTLINE           = ""   ;
int    LAST_LINE_RETURNED = FALSE;


/**
 * You don't have to call the following "intiBuffer()"-function,
 * because it is done automatically, if the BUFFSIZE is 0 (zero).
 * But you should call "freeBuffer()" at the end of your program,
 * when you stop reading in the middle of a stream.
 * If your program reaches the EOF (end of file), you don't have
 * to call either of both methods.
 */
void initBuffer() {
	//	printf("DEBUG line.c:initBuffer() -- initialize Buffer");
	BUFFSIZE = INITIAL_LINE_SIZE+sizeof(char); // space for 0-terminator
	BUFFER   = (char*) malloc((sizeof(char)) * BUFFSIZE);
	LAST_LINE_RETURNED = FALSE;
}


/** 
 * Free the Buffer, when EOF reached.
 * This is only done, when the last line was empty and an 
 * EOF (end of file) was found.
 * You have to call this, when you stop reading in the middle
 * of a file to free the Memory.
 * If you reach the EOF using "get_line()", this is called
 * automatically.
 */
void line_freeBuffer() {
	//printf("DEBUG line.c:freeBuffer() -- freeing buffer");
	LAST_LINE_RETURNED = TRUE;
	BUFFSIZE = 0;
	if(NULL != BUFFER) {
		free(BUFFER);
		BUFFER = NULL;
	}
}


/**
 * Calculates a new size.
 * This is called, when the actual buffer is too small.
 * This does NOT allocate a new Buffer.
 */
size_t calcNewSize() {
	size_t newSize = BUFFSIZE * (100 + ENLARGE_PERCENTAGE) / 100;
	if(newSize <= BUFFSIZE) {
		newSize = BUFFSIZE + sizeof(char);
	}
	//printf("DEBUG line.c:calcNewSize() -- new size calculated: %ld\n", (long) newSize);
	return newSize;
}


/**
 * Copy the old buffer to the new.
 * This old Kernighan/Ritche Trick stayed in my
 * brain since about 35 years. I have not
 * forgotten, what these clever guys did!
 * I didn't even open my book, which I still have!
 */
void copyBuffer(char* from, char* to) {
	from[BUFFSIZE-sizeof(char)]=0; // ensure buffer end; so the while will terminate.
	while(*to++ = *from++) {}
}


/**
 * As an ArrayList in Java, this enlages the buffer
 * and copies the content to the new buffer.
 */
void enlargeBuffer() {
	size_t newBuffSize = calcNewSize()      ;
	char * newBuffer   = malloc((sizeof(char)) * newBuffSize);

	copyBuffer(BUFFER, newBuffer);
	BUFFSIZE = newBuffSize;
	free(BUFFER);
	BUFFER = newBuffer;
}


/**
 * Check, if the buffer has a requested length.
 * If not, enlarge the buffer.
 * @requestedLineLength the length of a line (not the lenght of the
 *                      buffer, which must be one character larger).
 */
void ensureCapacity(size_t requestedLineLength) {
	size_t requestedBufferSize = requestedLineLength + sizeof(char);
	if(BUFFSIZE < requestedBufferSize) {
		enlargeBuffer();
	}
}



/**
 * the following algorithm is not able to see, if the last line
 * just ended on '\n' or on an EOF.
 * To solve this, it should store the last '\n' in a global variable.
 * If an EOF follows any character other than '\n', the last
 * NewLine was omitted.
 * In other words:
 * Because this algorithm never adds the '\n'-char to the lines read,
 * it is for the user not possible to distinguish if the last Line contained
 * '\n' or not.
 * Because this is normally not a problem, I omitted this case.
 * But be aware: It is not possible, to reconstruct a file correctly,
 * if handled by this algorihtm.
 */
// usage: see line.h
char* get_line(FILE *filePointer) {
	if(0 == BUFFSIZE) { // not initialized yet
		initBuffer();
	}
	int pos = 0;
	int getcResult = getc(filePointer);
	if(EOF==getcResult) {
		if(LAST_LINE_RETURNED) {
			line_freeBuffer();
			return 0;
		}
		LAST_LINE_RETURNED = TRUE;
		return LASTLINE;
	}
	while((EOF != getcResult) && ('\n' != getcResult)) {
		ensureCapacity(pos+sizeof(char));
		BUFFER[pos++] = (char) getcResult;
		getcResult = getc(filePointer);
	}
	BUFFER[pos] = 0;
	if(EOF == getcResult) {
		LAST_LINE_RETURNED = TRUE;
	}
	return BUFFER;
}


#endif
