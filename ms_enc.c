/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>

#define PADDING 5

int help() {
	fputs(  "ms_enc <data> [output file]\n"
		"This program encodes content for track 2 magnetic stripes in ABA format\n"
		"Example:\n\tms_enc \";4123456789012345=100710100000?\" output.pcm\n"
		"Optional output is given in S16 LE format\n",
		stderr );
	return 1;
}

int aba_encode_char( char *bitStream, char content, char *lrc ) {
	int i,parity;

	/* lrc is null when the content is the lrc -- skip checks for LRC */
	if( lrc ) {
		content -= 48;//48 is the offset for ASCII (eg, 5+48 == 53 or '5' )

		if( content < 0 || content > 15 )
			return 1;//out of bounds!
	}
	
	for( i = 0, parity = 1; i < 4; i++ ) {
		if( lrc )
			*lrc ^= ( content & 1 ) << i;

		parity ^= ( content & 1 );
		bitStream[i] = ( content & 1 ) + 48;
		content >>= 1;
	}

	bitStream[4] = (parity) + 48;

	return 0;
}

char *aba_encode( char *content, int padding ) {
	char *bitStream,lrc;
	int i,x;

	/* malloc the array to hold the bitstream */
	i = strlen( content ) + ( padding * 2 ) + 1;
	i *= 5; // 5 bits per char
	bitStream = malloc( i + 1 );//+ term char

	/* Add the beginning padding */
	for( x = 0; x < padding; x++ )
		bitStream[ x ] = '0';

	/* Convert the string */
	lrc = 0;
	for( i = 0; content[i] != '\0'; i++, x+=5 ) {
		aba_encode_char( bitStream + x, content[i], &lrc );
	}

	/* Add the LRC */
	aba_encode_char( bitStream + x, lrc, NULL );
	x += 5;

	/* Add the end padding */
	for( i = 0; i < padding; i++, x++ )
		bitStream[ x ] = '0';
	
	/* Add the null terminator */
	bitStream[ x ] = '\0';

	return bitStream;
}

#define SAMPLE_SIZE 100
#define PEAK 1000

void mkPCM( char *filename, char *bitStream, int clock ) {
	FILE *pcmFile;
	int pos = 1;
	int i,x,increment;
	short sample = 0;


	pcmFile = fopen( filename, "w" );
	if( !pcmFile )
		return;

	for( i = 0; i < SAMPLE_SIZE * 2; i++ )
		fwrite( &sample, sizeof(short), 1, pcmFile );
	
	for( i = 0; bitStream[i] != '\0'; i++ ) {
		increment = ( PEAK * 2 ) / SAMPLE_SIZE;
		if( i == 0 )
			increment /= 2;
		if( bitStream[i] == '1' )
			increment *= 2;

		for( x = 0; x < SAMPLE_SIZE; x++ ) {
			if( bitStream[i] == '1' && x == SAMPLE_SIZE / 2 )
				pos = !pos;
			fwrite( &sample, sizeof( short ), 1, pcmFile );
			if( pos )
				sample += increment;
			else
				sample -= increment;
		}
		pos = !pos;
	}

	increment = PEAK / SAMPLE_SIZE;
	for( i = 0; i < SAMPLE_SIZE; i++ ) {
		fwrite( &sample, sizeof( short ), 1, pcmFile );
		if( pos )
			sample += increment;
		else
			sample -= increment;
	}

	for( i = 0; i < SAMPLE_SIZE * 2; i++ )
		fwrite( &sample, sizeof(short), 1, pcmFile );

	fclose( pcmFile );

}

int main( int argc, char **argv ) {
	char *bitStream;

	if( argc == 1 )
		return help();

	if( argv[1][0] != ';' && argv[1][ strlen( argv[1] ) ] != '?' )
		fputs(  "Warning: Provided content does not have a start and/or end sentinel!\n"
			"They are typically required in order to properly decode.\n"
			"Continuing anyway...\n", stderr );
	
	bitStream = aba_encode( argv[1], PADDING );
	
	printf("Encoded to bits:\n%s\n", bitStream );

	if( argc == 3 ) {
		mkPCM( argv[2], bitStream, 100 );
		printf( "Saved data to %s\n", argv[2] );
	}

	free( bitStream );

	return 0;
}
