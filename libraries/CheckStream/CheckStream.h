#ifndef _CHECK_STREAM_H_
#define _CHECK_STREAM_H_
#include "Arduino.h"
#include "util/crc16.h"

/* - this code works, but isn't needed because we can use built-in crc code
uint16_t crc_ccitt_update( uint16_t crc, uint8_t data ) {
	data ^= (crc & 0xFF);
	data ^= data << 4;
	return ((((uint16_t) data << 8) | ((crc >> 8) & 0xFF)) ^ (uint8_t)(data >> 4) ^ ((uint16_t)data << 3));
}
*/


class CheckStream : public Stream {
public:

	CheckStream( Stream &stream ) : m_stream( stream ) {
		m_crc = 0xffff;
		m_firstEnd = true;
	}

	size_t write( byte data ) {
		if (data == 10 || data == 13) {
			if (m_firstEnd) {
				m_stream.print( "|" );
				m_stream.print( m_crc );
			}
			m_firstEnd = false;
#ifdef OLD_CHECKSUM
			m_crc = 0;
#else
			m_crc = 0xffff;
#endif
		} else {
#ifdef OLD_CHECKSUM
			m_crc = 0xff & (m_crc + data);
#else
			m_crc = _crc_ccitt_update( m_crc, data );
#endif
			m_firstEnd = true;
		}
		return m_stream.write( data );
	}

	// direct pass-through to stream class
	void flush() { m_stream.flush(); }
	int read() { return m_stream.read(); } 
	int available() { return m_stream.available(); }
	int peek() { return m_stream.peek(); }

private:

	// internal data
	Stream &m_stream;
	uint16_t m_crc;
	bool m_firstEnd;
};


#endif