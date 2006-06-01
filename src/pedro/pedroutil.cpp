/*
 * Support classes for the Pedro mini-XMPP client
 *
 * Authors:
 *   Bob Jamison
 *
 * Copyright (C) 2005-2006 Bob Jamison
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include <stdio.h>
#include <stdarg.h>

#include <sys/stat.h>

#include "pedroutil.h"



#ifdef __WIN32__

#include <windows.h>

#else /* UNIX */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <pthread.h>

#endif /* UNIX */



namespace Pedro
{





//########################################################################
//########################################################################
//# B A S E    6 4
//########################################################################
//########################################################################


//#################
//# ENCODER
//#################


static char *base64encode =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";



/**
 * Writes the specified byte to the output buffer
 */
void Base64Encoder::append(int ch)
{
    outBuf   <<=  8;
    outBuf   |=  (ch & 0xff);
    bitCount +=  8;
    if (bitCount >= 24)
        {
        int indx  = (int)((outBuf & 0x00fc0000L) >> 18);
        int obyte = (int)base64encode[indx & 63];
        buf.push_back(obyte);

        indx      = (int)((outBuf & 0x0003f000L) >> 12);
        obyte     = (int)base64encode[indx & 63];
        buf.push_back(obyte);

        indx      = (int)((outBuf & 0x00000fc0L) >>  6);
        obyte     = (int)base64encode[indx & 63];
        buf.push_back(obyte);

        indx      = (int)((outBuf & 0x0000003fL)      );
        obyte     = (int)base64encode[indx & 63];
        buf.push_back(obyte);

        bitCount = 0;
        outBuf   = 0L;
        }
}

/**
 * Writes the specified string to the output buffer
 */
void Base64Encoder::append(char *str)
{
    while (*str)
        append((int)*str++);
}

/**
 * Writes the specified string to the output buffer
 */
void Base64Encoder::append(unsigned char *str, int len)
{
    while (len>0)
        {
        append((int)*str++);
        len--;
        }
}

/**
 * Writes the specified string to the output buffer
 */
void Base64Encoder::append(const DOMString &str)
{
    append((char *)str.c_str());
}

/**
 * Closes this output stream and releases any system resources
 * associated with this stream.
 */
DOMString Base64Encoder::finish()
{
    //get any last bytes (1 or 2) out of the buffer
    if (bitCount == 16)
        {
        outBuf <<= 2;  //pad to make 18 bits

        int indx  = (int)((outBuf & 0x0003f000L) >> 12);
        int obyte = (int)base64encode[indx & 63];
        buf.push_back(obyte);

        indx      = (int)((outBuf & 0x00000fc0L) >>  6);
        obyte     = (int)base64encode[indx & 63];
        buf.push_back(obyte);

        indx      = (int)((outBuf & 0x0000003fL)      );
        obyte     = (int)base64encode[indx & 63];
        buf.push_back(obyte);

        buf.push_back('=');
        }
    else if (bitCount == 8)
        {
        outBuf <<= 4; //pad to make 12 bits

        int indx  = (int)((outBuf & 0x00000fc0L) >>  6);
        int obyte = (int)base64encode[indx & 63];
        buf.push_back(obyte);

        indx      = (int)((outBuf & 0x0000003fL)      );
        obyte     = (int)base64encode[indx & 63];
        buf.push_back(obyte);

        buf.push_back('=');
        buf.push_back('=');
        }

    DOMString ret = buf;
    reset();
    return ret;
}


DOMString Base64Encoder::encode(const DOMString &str)
{
    Base64Encoder encoder;
    encoder.append(str);
    DOMString ret = encoder.finish();
    return ret;
}



//#################
//# DECODER
//#################

static int base64decode[] =
{
/*00*/    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
/*08*/    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
/*10*/    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
/*18*/    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
/*20*/    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
/*28*/    -1,   -1,   -1,   62,   -1,   -1,   -1,   63,
/*30*/    52,   53,   54,   55,   56,   57,   58,   59,
/*38*/    60,   61,   -1,   -1,   -1,   -1,   -1,   -1,
/*40*/    -1,    0,    1,    2,    3,    4,    5,    6,
/*48*/     7,    8,    9,   10,   11,   12,   13,   14,
/*50*/    15,   16,   17,   18,   19,   20,   21,   22,
/*58*/    23,   24,   25,   -1,   -1,   -1,   -1,   -1,
/*60*/    -1,   26,   27,   28,   29,   30,   31,   32,
/*68*/    33,   34,   35,   36,   37,   38,   39,   40,
/*70*/    41,   42,   43,   44,   45,   46,   47,   48,
/*78*/    49,   50,   51,   -1,   -1,   -1,   -1,   -1
};



/**
 * Appends one char to the decoder
 */
void Base64Decoder::append(int ch)
{
    if (isspace(ch))
        return;
    else if (ch == '=') //padding
        {
        inBytes[inCount++] = 0;
        }
    else
        {
        int byteVal = base64decode[ch & 0x7f];
        //printf("char:%c %d\n", ch, byteVal);
        if (byteVal < 0)
            {
            //Bad lookup value
            }
        inBytes[inCount++] = byteVal;
        }

    if (inCount >=4 )
        {
        unsigned char b0 = ((inBytes[0]<<2) & 0xfc) | ((inBytes[1]>>4) & 0x03);
        unsigned char b1 = ((inBytes[1]<<4) & 0xf0) | ((inBytes[2]>>2) & 0x0f);
        unsigned char b2 = ((inBytes[2]<<6) & 0xc0) | ((inBytes[3]   ) & 0x3f);
        buf.push_back(b0);
        buf.push_back(b1);
        buf.push_back(b2);
        inCount = 0;
        }

}

void Base64Decoder::append(char *str)
{
    while (*str)
        append((int)*str++);
}

void Base64Decoder::append(const DOMString &str)
{
    append((char *)str.c_str());
}

std::vector<unsigned char> Base64Decoder::finish()
{
    std::vector<unsigned char> ret = buf;
    reset();
    return ret;
}

std::vector<unsigned char> Base64Decoder::decode(const DOMString &str)
{
    Base64Decoder decoder;
    decoder.append(str);
    std::vector<unsigned char> ret = decoder.finish();
    return ret;
}

DOMString Base64Decoder::decodeToString(const DOMString &str)
{
    Base64Decoder decoder;
    decoder.append(str);
    std::vector<unsigned char> ret = decoder.finish();
    DOMString buf;
    for (unsigned int i=0 ; i<ret.size() ; i++)
        buf.push_back(ret[i]);
    return buf;
}







//########################################################################
//########################################################################
//### S H A    1      H A S H I N G
//########################################################################
//########################################################################




void Sha1::hash(unsigned char *dataIn, int len, unsigned char *digest)
{
    Sha1 sha1;
    sha1.append(dataIn, len);
    sha1.finish(digest);
}

static char *sha1hex = "0123456789abcdef";

DOMString Sha1::hashHex(unsigned char *dataIn, int len)
{
    unsigned char hashout[20];
    hash(dataIn, len, hashout);
    DOMString ret;
    for (int i=0 ; i<20 ; i++)
        {
        unsigned char ch = hashout[i];
        ret.push_back(sha1hex[ (ch>>4) & 15 ]);
        ret.push_back(sha1hex[ ch      & 15 ]);
        }
    return ret;
}


void Sha1::init()
{

    lenW   = 0;
    sizeHi = 0;
    sizeLo = 0;

    // Initialize H with the magic constants (see FIPS180 for constants)
    H[0] = 0x67452301L;
    H[1] = 0xefcdab89L;
    H[2] = 0x98badcfeL;
    H[3] = 0x10325476L;
    H[4] = 0xc3d2e1f0L;

    for (int i = 0; i < 80; i++)
        W[i] = 0;
}


void Sha1::append(unsigned char *dataIn, int len)
{
    // Read the data into W and process blocks as they get full
    for (int i = 0; i < len; i++)
        {
        W[lenW / 4] <<= 8;
        W[lenW / 4] |= (unsigned long)dataIn[i];
        if ((++lenW) % 64 == 0)
            {
            hashblock();
            lenW = 0;
            }
        sizeLo += 8;
        sizeHi += (sizeLo < 8);
        }
}


void Sha1::finish(unsigned char hashout[20])
{
    unsigned char pad0x80 = 0x80;
    unsigned char pad0x00 = 0x00;
    unsigned char padlen[8];

    // Pad with a binary 1 (e.g. 0x80), then zeroes, then length
    padlen[0] = (unsigned char)((sizeHi >> 24) & 255);
    padlen[1] = (unsigned char)((sizeHi >> 16) & 255);
    padlen[2] = (unsigned char)((sizeHi >>  8) & 255);
    padlen[3] = (unsigned char)((sizeHi >>  0) & 255);
    padlen[4] = (unsigned char)((sizeLo >> 24) & 255);
    padlen[5] = (unsigned char)((sizeLo >> 16) & 255);
    padlen[6] = (unsigned char)((sizeLo >>  8) & 255);
    padlen[7] = (unsigned char)((sizeLo >>  0) & 255);

    append(&pad0x80, 1);

    while (lenW != 56)
        append(&pad0x00, 1);
    append(padlen, 8);

    // Output hash
    for (int i = 0; i < 20; i++)
        {
        hashout[i] = (unsigned char)(H[i / 4] >> 24);
        H[i / 4] <<= 8;
        }

    // Re-initialize the context (also zeroizes contents)
    init();
}


#define SHA_ROTL(X,n) ((((X) << (n)) | ((X) >> (32-(n)))) & 0xffffffffL)

void Sha1::hashblock()
{

    for (int t = 16; t <= 79; t++)
        W[t] = SHA_ROTL(W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16], 1);

    unsigned long A = H[0];
    unsigned long B = H[1];
    unsigned long C = H[2];
    unsigned long D = H[3];
    unsigned long E = H[4];

    unsigned long TEMP;

    for (int t = 0; t <= 19; t++)
        {
        TEMP = (SHA_ROTL(A,5) + (((C^D)&B)^D) +
                E + W[t] + 0x5a827999L) & 0xffffffffL;
        E = D; D = C; C = SHA_ROTL(B, 30); B = A; A = TEMP;
        }
    for (int t = 20; t <= 39; t++)
        {
        TEMP = (SHA_ROTL(A,5) + (B^C^D) +
                E + W[t] + 0x6ed9eba1L) & 0xffffffffL;
        E = D; D = C; C = SHA_ROTL(B, 30); B = A; A = TEMP;
        }
    for (int t = 40; t <= 59; t++)
        {
        TEMP = (SHA_ROTL(A,5) + ((B&C)|(D&(B|C))) +
                E + W[t] + 0x8f1bbcdcL) & 0xffffffffL;
        E = D; D = C; C = SHA_ROTL(B, 30); B = A; A = TEMP;
        }
    for (int t = 60; t <= 79; t++)
        {
        TEMP = (SHA_ROTL(A,5) + (B^C^D) +
                E + W[t] + 0xca62c1d6L) & 0xffffffffL;
        E = D; D = C; C = SHA_ROTL(B, 30); B = A; A = TEMP;
        }

    H[0] += A;
    H[1] += B;
    H[2] += C;
    H[3] += D;
    H[4] += E;
}






//########################################################################
//########################################################################
//### M D 5      H A S H I N G
//########################################################################
//########################################################################





void Md5::hash(unsigned char *dataIn, unsigned long len, unsigned char *digest)
{
    Md5 md5;
    md5.append(dataIn, len);
    md5.finish(digest);
}

DOMString Md5::hashHex(unsigned char *dataIn, unsigned long len)
{
    Md5 md5;
    md5.append(dataIn, len);
    DOMString ret = md5.finishHex();
    return ret;
}



/*
 * Note: this code is harmless on little-endian machines.
 */
/*
static void byteReverse(unsigned char *buf, unsigned long longs)
{
    do
        {
        unsigned long t = (unsigned long)
            ((unsigned) buf[3] << 8 | buf[2]) << 16 |
            ((unsigned) buf[1] << 8 | buf[0]);
        *(unsigned long *) buf = t;
        buf += 4;
        } while (--longs);
}
*/

static void md5_memcpy(void *dest, void *src, int n)
{
    unsigned char *s1 = (unsigned char *)dest;
    unsigned char *s2 = (unsigned char *)src;
    while (n--)
        *s1++ = *s2++;
}

static void md5_memset(void *dest, char v, int n)
{
    unsigned char *s = (unsigned char *)dest;
    while (n--)
        *s++ = v;
}

/**
 * Initialize MD5 polynomials and storage
 */
void Md5::init()
{
    buf[0]  = 0x67452301;
    buf[1]  = 0xefcdab89;
    buf[2]  = 0x98badcfe;
    buf[3]  = 0x10325476;

    bits[0] = 0;
    bits[1] = 0;
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void Md5::append(unsigned char *source, unsigned long len)
{

    // Update bitcount
    unsigned long t = bits[0];
    if ((bits[0] = t + ((unsigned long) len << 3)) < t)
	    bits[1]++;// Carry from low to high
    bits[1] += len >> 29;

	//Bytes already in shsInfo->data
    t = (t >> 3) & 0x3f;


    // Handle any leading odd-sized chunks
    if (t)
        {
        unsigned char *p = (unsigned char *) in + t;
        t = 64 - t;
        if (len < t)
            {
            md5_memcpy(p, source, len);
            return;
            }
        md5_memcpy(p, source, t);
        //byteReverse(in, 16);
        transform(buf, (unsigned long *) in);
        source += t;
        len    -= t;
        }

    // Process data in 64-byte chunks
    while (len >= 64)
        {
        md5_memcpy(in, source, 64);
        //byteReverse(in, 16);
        transform(buf, (unsigned long *) in);
        source += 64;
        len    -= 64;
        }

    // Handle any remaining bytes of data.
    md5_memcpy(in, source, len);
}

/*
 * Update context to reflect the concatenation of another string
 */
void Md5::append(const DOMString &str)
{
    append((unsigned char *)str.c_str(), str.size());
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void Md5::finish(unsigned char *digest)
{
    // Compute number of bytes mod 64
    unsigned int count = (bits[0] >> 3) & 0x3F;

    // Set the first char of padding to 0x80.
    // This is safe since there is always at least one byte free
    unsigned char *p = in + count;
    *p++ = 0x80;

    // Bytes of padding needed to make 64 bytes
    count = 64 - 1 - count;

    // Pad out to 56 mod 64
    if (count < 8)
        {
	    // Two lots of padding:  Pad the first block to 64 bytes
	    md5_memset(p, 0, count);
	    //byteReverse(in, 16);
	    transform(buf, (unsigned long *) in);

	    // Now fill the next block with 56 bytes
	    md5_memset(in, 0, 56);
        }
    else
        {
        // Pad block to 56 bytes
        md5_memset(p, 0, count - 8);
        }
    //byteReverse(in, 14);

    // Append length in bits and transform
    ((unsigned long *) in)[14] = bits[0];
    ((unsigned long *) in)[15] = bits[1];

    transform(buf, (unsigned long *) in);
    //byteReverse((unsigned char *) buf, 4);
    md5_memcpy(digest, buf, 16);
    init();  // Security!  ;-)
}

static char *md5hex = "0123456789abcdef";

DOMString Md5::finishHex()
{
    unsigned char hashout[16];
    finish(hashout);
    DOMString ret;
    for (int i=0 ; i<16 ; i++)
        {
        unsigned char ch = hashout[i];
        ret.push_back(md5hex[ (ch>>4) & 15 ]);
        ret.push_back(md5hex[ ch      & 15 ]);
        }
    return ret;
}



//#  The four core functions - F1 is optimized somewhat

//  #define F1(x, y, z) (x & y | ~x & z)
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

// ## This is the central step in the MD5 algorithm.
#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 * @parm buf points to an array of 4 unsigned longs
 * @parm in points to an array of 16 unsigned longs
 */
void Md5::transform(unsigned long *buf, unsigned long *in)
{
    unsigned long a = buf[0];
    unsigned long b = buf[1];
    unsigned long c = buf[2];
    unsigned long d = buf[3];

    MD5STEP(F1, a, b, c, d, in[ 0] + 0xd76aa478,  7);
    MD5STEP(F1, d, a, b, c, in[ 1] + 0xe8c7b756, 12);
    MD5STEP(F1, c, d, a, b, in[ 2] + 0x242070db, 17);
    MD5STEP(F1, b, c, d, a, in[ 3] + 0xc1bdceee, 22);
    MD5STEP(F1, a, b, c, d, in[ 4] + 0xf57c0faf,  7);
    MD5STEP(F1, d, a, b, c, in[ 5] + 0x4787c62a, 12);
    MD5STEP(F1, c, d, a, b, in[ 6] + 0xa8304613, 17);
    MD5STEP(F1, b, c, d, a, in[ 7] + 0xfd469501, 22);
    MD5STEP(F1, a, b, c, d, in[ 8] + 0x698098d8,  7);
    MD5STEP(F1, d, a, b, c, in[ 9] + 0x8b44f7af, 12);
    MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
    MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
    MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122,  7);
    MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
    MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
    MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

    MD5STEP(F2, a, b, c, d, in[ 1] + 0xf61e2562,  5);
    MD5STEP(F2, d, a, b, c, in[ 6] + 0xc040b340,  9);
    MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
    MD5STEP(F2, b, c, d, a, in[ 0] + 0xe9b6c7aa, 20);
    MD5STEP(F2, a, b, c, d, in[ 5] + 0xd62f105d,  5);
    MD5STEP(F2, d, a, b, c, in[10] + 0x02441453,  9);
    MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
    MD5STEP(F2, b, c, d, a, in[ 4] + 0xe7d3fbc8, 20);
    MD5STEP(F2, a, b, c, d, in[ 9] + 0x21e1cde6,  5);
    MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6,  9);
    MD5STEP(F2, c, d, a, b, in[ 3] + 0xf4d50d87, 14);
    MD5STEP(F2, b, c, d, a, in[ 8] + 0x455a14ed, 20);
    MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905,  5);
    MD5STEP(F2, d, a, b, c, in[ 2] + 0xfcefa3f8,  9);
    MD5STEP(F2, c, d, a, b, in[ 7] + 0x676f02d9, 14);
    MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

    MD5STEP(F3, a, b, c, d, in[ 5] + 0xfffa3942,  4);
    MD5STEP(F3, d, a, b, c, in[ 8] + 0x8771f681, 11);
    MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
    MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
    MD5STEP(F3, a, b, c, d, in[ 1] + 0xa4beea44,  4);
    MD5STEP(F3, d, a, b, c, in[ 4] + 0x4bdecfa9, 11);
    MD5STEP(F3, c, d, a, b, in[ 7] + 0xf6bb4b60, 16);
    MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
    MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6,  4);
    MD5STEP(F3, d, a, b, c, in[ 0] + 0xeaa127fa, 11);
    MD5STEP(F3, c, d, a, b, in[ 3] + 0xd4ef3085, 16);
    MD5STEP(F3, b, c, d, a, in[ 6] + 0x04881d05, 23);
    MD5STEP(F3, a, b, c, d, in[ 9] + 0xd9d4d039,  4);
    MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
    MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
    MD5STEP(F3, b, c, d, a, in[ 2] + 0xc4ac5665, 23);

    MD5STEP(F4, a, b, c, d, in[ 0] + 0xf4292244,  6);
    MD5STEP(F4, d, a, b, c, in[ 7] + 0x432aff97, 10);
    MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
    MD5STEP(F4, b, c, d, a, in[ 5] + 0xfc93a039, 21);
    MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3,  6);
    MD5STEP(F4, d, a, b, c, in[ 3] + 0x8f0ccc92, 10);
    MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
    MD5STEP(F4, b, c, d, a, in[ 1] + 0x85845dd1, 21);
    MD5STEP(F4, a, b, c, d, in[ 8] + 0x6fa87e4f,  6);
    MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
    MD5STEP(F4, c, d, a, b, in[ 6] + 0xa3014314, 15);
    MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
    MD5STEP(F4, a, b, c, d, in[ 4] + 0xf7537e82,  6);
    MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
    MD5STEP(F4, c, d, a, b, in[ 2] + 0x2ad7d2bb, 15);
    MD5STEP(F4, b, c, d, a, in[ 9] + 0xeb86d391, 21);

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}










//########################################################################
//########################################################################
//### T H R E A D
//########################################################################
//########################################################################





#ifdef __WIN32__


static DWORD WINAPI WinThreadFunction(LPVOID context)
{
    Thread *thread = (Thread *)context;
    thread->execute();
    return 0;
}


void Thread::start()
{
    DWORD dwThreadId;
    HANDLE hThread = CreateThread(NULL, 0, WinThreadFunction,
               (LPVOID)this,  0,  &dwThreadId);
    //Make sure the thread is started before 'this' is deallocated
    while (!started)
        sleep(10);
    CloseHandle(hThread);
}

void Thread::sleep(unsigned long millis)
{
    Sleep(millis);
}

#else /* UNIX */


void *PthreadThreadFunction(void *context)
{
    Thread *thread = (Thread *)context;
    thread->execute();
    return NULL;
}


void Thread::start()
{
    pthread_t thread;

    int ret = pthread_create(&thread, NULL,
            PthreadThreadFunction, (void *)this);
    if (ret != 0)
        printf("Thread::start: thread creation failed: %s\n", strerror(ret));

    //Make sure the thread is started before 'this' is deallocated
    while (!started)
        sleep(10);

}

void Thread::sleep(unsigned long millis)
{
    timespec requested;
    requested.tv_sec = millis / 1000;
    requested.tv_nsec = (millis % 1000 ) * 1000000L;
    nanosleep(&requested, NULL);
}

#endif








//########################################################################
//########################################################################
//### S O C K E T
//########################################################################
//########################################################################





//#########################################################################
//# U T I L I T Y
//#########################################################################

static void mybzero(void *s, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    while (n > 0)
        {
        *p++ = (unsigned char)0;
        n--;
        }
}

static void mybcopy(void *src, void *dest, size_t n)
{
    unsigned char *p = (unsigned char *)dest;
    unsigned char *q = (unsigned char *)src;
    while (n > 0)
        {
        *p++ = *q++;
        n--;
        }
}



//#########################################################################
//# T C P    C O N N E C T I O N
//#########################################################################

TcpSocket::TcpSocket()
{
    init();
}


TcpSocket::TcpSocket(const char *hostnameArg, int port)
{
    init();
    hostname  = hostnameArg;
    portno    = port;
}

TcpSocket::TcpSocket(const std::string &hostnameArg, int port)
{
    init();
    hostname  = hostnameArg;
    portno    = port;
}


#ifdef HAVE_SSL

static void cryptoLockCallback(int mode, int type, const char *file, int line)
{
    //printf("########### LOCK\n");
    static int modes[CRYPTO_NUM_LOCKS]; /* = {0, 0, ... } */
    const char *errstr = NULL;

    int rw = mode & (CRYPTO_READ|CRYPTO_WRITE);
    if (!((rw == CRYPTO_READ) || (rw == CRYPTO_WRITE)))
        {
        errstr = "invalid mode";
        goto err;
        }

    if (type < 0 || type >= CRYPTO_NUM_LOCKS)
        {
        errstr = "type out of bounds";
        goto err;
        }

    if (mode & CRYPTO_LOCK)
        {
        if (modes[type])
            {
            errstr = "already locked";
            /* must not happen in a single-threaded program
             * (would deadlock)
             */
            goto err;
            }

        modes[type] = rw;
        }
    else if (mode & CRYPTO_UNLOCK)
        {
        if (!modes[type])
            {
             errstr = "not locked";
             goto err;
             }

        if (modes[type] != rw)
            {
            errstr = (rw == CRYPTO_READ) ?
                  "CRYPTO_r_unlock on write lock" :
                  "CRYPTO_w_unlock on read lock";
            }

        modes[type] = 0;
        }
    else
        {
        errstr = "invalid mode";
        goto err;
        }

    err:
    if (errstr)
        {
        /* we cannot use bio_err here */
        fprintf(stderr, "openssl (lock_dbg_cb): %s (mode=%d, type=%d) at %s:%d\n",
                errstr, mode, type, file, line);
        }
}

static unsigned long cryptoIdCallback()
{
#ifdef __WIN32__
    unsigned long ret = (unsigned long) GetCurrentThreadId();
#else
    unsigned long ret = (unsigned long) pthread_self();
#endif
    return ret;
}

#endif


TcpSocket::TcpSocket(const TcpSocket &other)
{
    init();
    sock      = other.sock;
    hostname  = other.hostname;
    portno    = other.portno;
}

static bool tcp_socket_inited = false;

void TcpSocket::init()
{
    if (!tcp_socket_inited)
        {
#ifdef __WIN32__
        WORD wVersionRequested = MAKEWORD( 2, 2 );
        WSADATA wsaData;
        WSAStartup( wVersionRequested, &wsaData );
#endif
#ifdef HAVE_SSL
        sslStream  = NULL;
        sslContext = NULL;
	    CRYPTO_set_locking_callback(cryptoLockCallback);
        CRYPTO_set_id_callback(cryptoIdCallback);
        SSL_library_init();
        SSL_load_error_strings();
#endif
        tcp_socket_inited = true;
        }
    sock           = -1;
    connected      = false;
    hostname       = "";
    portno         = -1;
    sslEnabled     = false;
    receiveTimeout = 0;
}

TcpSocket::~TcpSocket()
{
    disconnect();
}

bool TcpSocket::isConnected()
{
    if (!connected || sock < 0)
        return false;
    return true;
}

void TcpSocket::enableSSL(bool val)
{
    sslEnabled = val;
}

bool TcpSocket::getEnableSSL()
{
    return sslEnabled;
}


bool TcpSocket::connect(const char *hostnameArg, int portnoArg)
{
    hostname = hostnameArg;
    portno   = portnoArg;
    return connect();
}

bool TcpSocket::connect(const std::string &hostnameArg, int portnoArg)
{
    hostname = hostnameArg;
    portno   = portnoArg;
    return connect();
}



#ifdef HAVE_SSL
/*
static int password_cb(char *buf, int bufLen, int rwflag, void *userdata)
{
    char *password = "password";
    if (bufLen < (int)(strlen(password)+1))
        return 0;

    strcpy(buf,password);
    int ret = strlen(password);
    return ret;
}

static void infoCallback(const SSL *ssl, int where, int ret)
{
    switch (where)
        {
        case SSL_CB_ALERT:
            {
            printf("## %d SSL ALERT: %s\n",  where, SSL_alert_desc_string_long(ret));
            break;
            }
        default:
            {
            printf("## %d SSL: %s\n",  where, SSL_state_string_long(ssl));
            break;
            }
        }
}
*/
#endif


bool TcpSocket::startTls()
{
#ifdef HAVE_SSL
    sslStream  = NULL;
    sslContext = NULL;

    //SSL_METHOD *meth = SSLv23_method();
    //SSL_METHOD *meth = SSLv3_client_method();
    SSL_METHOD *meth = TLSv1_client_method();
    sslContext = SSL_CTX_new(meth);
    //SSL_CTX_set_info_callback(sslContext, infoCallback);

#if 0
    char *keyFile  = "client.pem";
    char *caList   = "root.pem";
    /* Load our keys and certificates*/
    if (!(SSL_CTX_use_certificate_chain_file(sslContext, keyFile)))
        {
        fprintf(stderr, "Can't read certificate file\n");
        disconnect();
        return false;
        }

    SSL_CTX_set_default_passwd_cb(sslContext, password_cb);

    if (!(SSL_CTX_use_PrivateKey_file(sslContext, keyFile, SSL_FILETYPE_PEM)))
        {
        fprintf(stderr, "Can't read key file\n");
        disconnect();
        return false;
        }

    /* Load the CAs we trust*/
    if (!(SSL_CTX_load_verify_locations(sslContext, caList, 0)))
        {
        fprintf(stderr, "Can't read CA list\n");
        disconnect();
        return false;
        }
#endif

    /* Connect the SSL socket */
    sslStream  = SSL_new(sslContext);
    SSL_set_fd(sslStream, sock);

    int ret = SSL_connect(sslStream);
    if (ret == 0)
        {
        fprintf(stderr, "SSL connection not successful\n");
        disconnect();
        return false;
        }
    else if (ret < 0)
        {
        int err = SSL_get_error(sslStream, ret);
        fprintf(stderr, "SSL connect error %d\n", err);
        disconnect();
        return false;
        }

    sslEnabled = true;
#endif /*HAVE_SSL*/
    return true;
}


bool TcpSocket::connect()
{
    if (hostname.size()<1)
        {
        fprintf(stderr, "open: null hostname\n");
        return false;
        }

    if (portno<1)
        {
        fprintf(stderr, "open: bad port number\n");
        return false;
        }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        {
        fprintf(stderr, "open: error creating socket\n");
        return false;
        }

    char *c_hostname = (char *)hostname.c_str();
    struct hostent *server = gethostbyname(c_hostname);
    if (!server)
        {
        fprintf(stderr, "open: could not locate host '%s'\n", c_hostname);
        return false;
        }

    struct sockaddr_in serv_addr;
    mybzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    mybcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    int ret = ::connect(sock, (const sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret < 0)
        {
        fprintf(stderr, "open: could not connect to host '%s'\n", c_hostname);
        return false;
        }

     if (sslEnabled)
        {
        if (!startTls())
            return false;
        }
    connected = true;
    return true;
}

bool TcpSocket::disconnect()
{
    bool ret  = true;
    connected = false;
#ifdef HAVE_SSL
    if (sslEnabled)
        {
        if (sslStream)
            {
            int r = SSL_shutdown(sslStream);
            switch(r)
                {
                case 1:
                    break; /* Success */
                case 0:
                case -1:
                default:
                    //printf("Shutdown failed");
                    ret = false;
                }
            SSL_free(sslStream);
            }
        if (sslContext)
            SSL_CTX_free(sslContext);
        }
    sslStream  = NULL;
    sslContext = NULL;
#endif /*HAVE_SSL*/

#ifdef __WIN32__
    closesocket(sock);
#else
    ::close(sock);
#endif
    sock = -1;
    sslEnabled = false;

    return ret;
}



bool TcpSocket::setReceiveTimeout(unsigned long millis)
{
    receiveTimeout = millis;
    return true;
}

/**
 * For normal sockets, return the number of bytes waiting to be received.
 * For SSL, just return >0 when something is ready to be read.
 */
long TcpSocket::available()
{
    if (!isConnected())
        return -1;

    long count = 0;
#ifdef __WIN32__
    if (ioctlsocket(sock, FIONREAD, (unsigned long *)&count) != 0)
        return -1;
#else
    if (ioctl(sock, FIONREAD, &count) != 0)
        return -1;
#endif
    if (count<=0 && sslEnabled)
        {
#ifdef HAVE_SSL
        return SSL_pending(sslStream);
#endif
        }
    return count;
}



bool TcpSocket::write(int ch)
{
    if (!isConnected())
        {
        fprintf(stderr, "write: socket closed\n");
        return false;
        }
    unsigned char c = (unsigned char)ch;

    if (sslEnabled)
        {
#ifdef HAVE_SSL
        int r = SSL_write(sslStream, &c, 1);
        if (r<=0)
            {
            switch(SSL_get_error(sslStream, r))
                {
                default:
                    fprintf(stderr, "SSL write problem");
                    return -1;
                }
            }
#endif
        }
    else
        {
        if (send(sock, (const char *)&c, 1, 0) < 0)
        //if (send(sock, &c, 1, 0) < 0)
            {
            fprintf(stderr, "write: could not send data\n");
            return false;
            }
        }
    return true;
}

bool TcpSocket::write(char *str)
{
   if (!isConnected())
        {
        fprintf(stderr, "write(str): socket closed\n");
        return false;
        }
    int len = strlen(str);

    if (sslEnabled)
        {
#ifdef HAVE_SSL
        int r = SSL_write(sslStream, (unsigned char *)str, len);
        if (r<=0)
            {
            switch(SSL_get_error(sslStream, r))
                {
                default:
                    fprintf(stderr, "SSL write problem");
                    return -1;
                }
            }
#endif
        }
    else
        {
        if (send(sock, str, len, 0) < 0)
        //if (send(sock, &c, 1, 0) < 0)
            {
            fprintf(stderr, "write: could not send data\n");
            return false;
            }
        }
    return true;
}

bool TcpSocket::write(const std::string &str)
{
    return write((char *)str.c_str());
}

int TcpSocket::read()
{
    if (!isConnected())
        return -1;

    //We'll use this loop for timeouts, so that SSL and plain sockets
    //will behave the same way
    if (receiveTimeout > 0)
        {
        unsigned long tim = 0;
        while (true)
            {
            int avail = available();
            if (avail > 0)
                break;
            if (tim >= receiveTimeout)
                return -2;
            Thread::sleep(20);
            tim += 20;
            }
        }

    //check again
    if (!isConnected())
        return -1;

    unsigned char ch;
    if (sslEnabled)
        {
#ifdef HAVE_SSL
        if (!sslStream)
            return -1;
        int r = SSL_read(sslStream, &ch, 1);
        unsigned long err = SSL_get_error(sslStream, r);
        switch (err)
            {
            case SSL_ERROR_NONE:
                 break;
            case SSL_ERROR_ZERO_RETURN:
                return -1;
            case SSL_ERROR_SYSCALL:
                fprintf(stderr, "SSL read problem(syscall) %s\n",
                     ERR_error_string(ERR_get_error(), NULL));
                return -1;
            default:
                fprintf(stderr, "SSL read problem %s\n",
                     ERR_error_string(ERR_get_error(), NULL));
                return -1;
            }
#endif
        }
    else
        {
        if (recv(sock, (char *)&ch, 1, 0) <= 0)
            {
            fprintf(stderr, "read: could not receive data\n");
            disconnect();
            return -1;
            }
        }
    return (int)ch;
}

std::string TcpSocket::readLine()
{
    std::string ret;

    while (isConnected())
        {
        int ch = read();
        if (ch<0)
            return ret;
        if (ch=='\r' || ch=='\n')
            return ret;
        ret.push_back((char)ch);
        }

    return ret;
}









} //namespace Pedro
//########################################################################
//# E N D    O F     F I L E
//########################################################################











