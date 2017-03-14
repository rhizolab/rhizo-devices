#ifndef _COMMAND_PARSER_H_
#define _COMMAND_PARSER_H_
// this code adapted from wgCommands.h (copyright ManyLabs 2011-2013)
// MIT license
#include "Arduino.h"
#include "util/crc16.h"


// parameters for this library
#define INPUT_BUFFER_LENGTH 101
#define MAX_COMMAND_ARGS 10


//============================================
// STRING UTILITIES
//============================================


// get index of given character; return -1 if not found
// fix(smaller): replace with strchr
inline int indexOf( const char *str, char c ) {
	int index = 0;
	while (true) {
		char s = *str++;
		if (s == c)
			return index;
		if (s == 0)
			break;
		index++;
	}
	return -1;
}


// a simple helper macro for string comparision
#define strEq( a, b ) (strcmp( a, b ) == 0)


//============================================
// COMMAND PARSER CLASS
//============================================


class CommandParser {
public:

	CommandParser( void (*callback)( const char *boardId, const char *command, unsigned char argCount, char *args[] ), char *defaultBoardId = NULL ) {
		_callback = callback;
		_inputIndex = 0;
		_defaultBoardId = defaultBoardId; // pointer to outside string
		_requireCheckSum = false;
	}

	void feed( char c ) {
		if (c > 0) {
			if (c == 10 || c == 13) {
				if (_inputIndex)
					processCommand();
			} else if (_inputIndex >= INPUT_BUFFER_LENGTH - 1) { // want to be able to write 0 into last position after increment
				_inputIndex = 0;
			} else {
				_inputBuffer[ _inputIndex ] = c;
				_inputIndex++;
			}  
		}
	}
  
	void requireCheckSum( bool required = true ) {
		_requireCheckSum = required;
	}

private:

	void processCommand() {
		_inputBuffer[ _inputIndex ] = 0; // truncate the string
		_inputIndex = 0;
		char *command = _inputBuffer;
		char *args[ MAX_COMMAND_ARGS ];
		int argCount = 0;
		char *boardId = _defaultBoardId;

		// check for checksum
		int pipePos = indexOf( _inputBuffer, '|' ); // fix(faster): could start at end
		if (pipePos >= 0) {
#ifdef OLD_CHECKSUM
			int givenCheckSum = atoi( _inputBuffer + pipePos + 1 );
			unsigned char crc = 0;
			for (int i = 0; i < pipePos; i++)
				crc += _inputBuffer[ i ];
			if (givenCheckSum != crc) {
				/*
                Serial.print( "bad checksum; given: " );
				Serial.print( givenCheckSum );
				Serial.print( ", computed: " );
				Serial.println( (int) crc );
                 */
				return;
			}
#else
			uint16_t givenCheckSum = atoi( _inputBuffer + pipePos + 1 );
			uint16_t crc = 0xffff;
			for (int i = 0; i < pipePos; i++)
				crc = _crc_ccitt_update( crc, _inputBuffer[ i ] );
			if (givenCheckSum != crc) {
				/*
                Serial.print( "bad checksum; given: " );
				Serial.print( givenCheckSum );
				Serial.print( ", computed: " );
				Serial.println( crc );
                 */
				return;
			}
#endif
		} else if (_requireCheckSum) {
			//Serial.println( "checksum missing" );
			return;
		}

		// remove checksum
		if (pipePos >= 0) {
			_inputBuffer[ pipePos ] = 0; // remove checksum
		}

		// remove init spaces
		int commandStart = 0;
		while (command[ 0 ] == ' ') {
			command++;
			commandStart++;
		}

		// check for board identifier
		int colonPos = indexOf( _inputBuffer, ':' ); // fix(faster): could limit search to first few characters
		if (colonPos > 0 && colonPos < 5) {
			_inputBuffer[ colonPos ] = 0;
			boardId = _inputBuffer;
			command = _inputBuffer + colonPos + 1;
			commandStart = colonPos + 1;

			// remove spaces after board identifier
			while (command[ 0 ] == ' ') {
				command++;
				commandStart++;
			}
		}

		// check for command arguments
		int firstSpacePos = indexOf( _inputBuffer + commandStart, ' ' );
		if (firstSpacePos > 0) {
			firstSpacePos += commandStart;
			_inputBuffer[ firstSpacePos ] = 0;
			char *argStr = _inputBuffer + firstSpacePos + 1;
			int argSepPos = 0;
			bool quoted = false;
			argCount = 0;
			do {

				// strip leading spaces
				while (*argStr == ' ')
					argStr++;

				// check for quoted string
				if (*argStr == '"') {
					quoted = true;
					argStr++;
				}

				// store in argument array
				args[ argCount ] = argStr;

				// find end of arg (using space as argument seperator)
				argSepPos = indexOf( argStr, quoted ? '"' : ' ' );
				char *argEnd = 0;
				if (argSepPos > 0) {
					argStr[ argSepPos ] = 0;
					argStr += argSepPos + 1;
					argEnd = argStr - 2;
				} else {
					argEnd = argStr + strlen( argStr ) - 1;
				}

				// strip off tailing spaces (note: we're protected by 0 that replaced first space)
				while (*argEnd == ' ') {
					*argEnd = 0;
					argEnd--;
				}

				// done with this arg
				argCount++;
			} while (argSepPos > 0 && argCount < MAX_COMMAND_ARGS);
		}

		// remove empty arg caused by space at end of command string
		// fix(clean): is there a nice way of doing this in the above code?
		while (argCount && args[ argCount - 1 ][ 0 ] == 0)
			argCount--;

		// execute the command now that we have parsed it
		if (command[ 0 ]) {
			_callback( boardId, command, argCount, args );
		}
	}

	void (*_callback)( const char *boardId, const char *command, unsigned char argCount, char *args[] );
	char *_defaultBoardId; // pointer to outside string
	char _inputBuffer[ 100 ];
	unsigned char _inputIndex;
	bool _requireCheckSum;
};


#endif // _COMMAND_PARSER_H_
