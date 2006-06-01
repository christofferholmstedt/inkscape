#ifndef __PEDROUTIL_H__ 
#define __PEDROUTIL_H__ 
/*
 * Support classes for the Pedro mini-XMPP client.
 *
 * Authors:
 *   Bob Jamison
 *
 * Copyright (C) 2006 Bob Jamison
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
#include <vector>

#include <string>

#include "pedrodom.h"


#ifdef HAVE_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif



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


/**
 * This class is for Base-64 encoding
 */
class Base64Encoder
{

public:

    Base64Encoder()
        {
        reset();
        }

    virtual ~Base64Encoder()
        {}

    virtual void reset()
        {
        outBuf   = 0L;
        bitCount = 0;
        buf = "";
        }

    virtual void append(int ch);

    virtual void append(char *str);

    virtual void append(unsigned char *str, int len);

    virtual void append(const DOMString &str);

    virtual DOMString finish();

    static DOMString encode(const DOMString &str);


private:


    unsigned long outBuf;

    int bitCount;

    DOMString buf;

};




//#################
//# DECODER
//#################

class Base64Decoder
{
public:
    Base64Decoder()
        {
        reset();
        }

    virtual ~Base64Decoder()
        {}

    virtual void reset()
        {
        inCount = 0;
        buf.clear();
        }


    virtual void append(int ch);

    virtual void append(char *str);

    virtual void append(const DOMString &str);

    std::vector<unsigned char> finish();

    static std::vector<unsigned char> decode(const DOMString &str);

    static DOMString decodeToString(const DOMString &str);

private:

    int inBytes[4];
    int inCount;
    std::vector<unsigned char> buf;
};




//########################################################################
//########################################################################
//### S H A    1      H A S H I N G
//########################################################################
//########################################################################

/**
 *
 */
class Sha1
{
public:

    /**
     *
     */
    Sha1()
        { init(); }

    /**
     *
     */
    virtual ~Sha1()
        {}


    /**
     * Static convenience method.  This would be the most commonly used
     * version;
     * @parm digest points to a bufer of 20 unsigned chars
     */
    static void hash(unsigned char *dataIn, int len, unsigned char *digest);

    /**
     * Static convenience method.  This will fill a string with the hex
     * codex string.
     */
    static DOMString hashHex(unsigned char *dataIn, int len);

    /**
     *  Initialize the context (also zeroizes contents)
     */
    virtual void init();

    /**
     *
     */
    virtual void append(unsigned char *dataIn, int len);

    /**
     *
     * @parm digest points to a bufer of 20 unsigned chars
     */
    virtual void finish(unsigned char *digest);


private:

    void hashblock();

    unsigned long H[5];
    unsigned long W[80];
    unsigned long sizeHi,sizeLo;
    int lenW;

};





//########################################################################
//########################################################################
//### M D 5      H A S H I N G
//########################################################################
//########################################################################

/**
 *
 */
class Md5
{
public:

    /**
     *
     */
    Md5()
        { init(); }

    /**
     *
     */
    virtual ~Md5()
        {}

    /**
     * Static convenience method.
     * @parm digest points to an buffer of 16 unsigned chars
     */
    static void hash(unsigned char *dataIn,
                     unsigned long len, unsigned char *digest);

    static DOMString hashHex(unsigned char *dataIn, unsigned long len);

    /**
     *  Initialize the context (also zeroizes contents)
     */
    virtual void init();

    /**
     *
     */
    virtual void append(unsigned char *dataIn, unsigned long len);

    /**
     *
     */
    virtual void append(const DOMString &str);

    /**
     * Finalize and output the hash.
     * @parm digest points to an buffer of 16 unsigned chars
     */
    virtual void finish(unsigned char *digest);


    /**
     * Same as above , but hex to an output String
     */
    virtual DOMString finishHex();

private:

    void transform(unsigned long *buf, unsigned long *in);

    unsigned long buf[4];
    unsigned long bits[2];
    unsigned char in[64];

};







//########################################################################
//########################################################################
//### T H R E A D
//########################################################################
//########################################################################



/**
 * This is the interface for a delegate class which can
 * be run by a Thread.
 * Thread thread(runnable);
 * thread.start();
 */
class Runnable
{
public:

    Runnable()
        {}
    virtual ~Runnable()
        {}

    /**
     * The method of a delegate class which can
     * be run by a Thread.  Thread is completed when this
     * method is done.
     */
    virtual void run() = 0;

};



/**
 *  A simple wrapper of native threads in a portable class.
 *  It can be used either to execute its own run() method, or
 *  delegate to a Runnable class's run() method.
 */
class Thread
{
public:

    /**
     *  Create a thread which will execute its own run() method.
     */
    Thread()
        { runnable = NULL ; started = false; }

    /**
     * Create a thread which will run a Runnable class's run() method.
     */
    Thread(const Runnable &runner)
        { runnable = (Runnable *)&runner; started = false; }

    /**
     *  This does not kill a spawned thread.
     */
    virtual ~Thread()
        {}

    /**
     *  Static method to pause the current thread for a given
     *  number of milliseconds.
     */
    static void sleep(unsigned long millis);

    /**
     *  This method will be executed if the Thread was created with
     *  no delegated Runnable class.  The thread is completed when
     *  the method is done.
     */
    virtual void run()
        {}

    /**
     *  Starts the thread.
     */
    virtual void start();

    /**
     *  Calls either this class's run() method, or that of a Runnable.
     *  A user would normally not call this directly.
     */
    virtual void execute()
        {
        started = true;
        if (runnable)
            runnable->run();
        else
            run();
        }

private:

    Runnable *runnable;

    bool started;

};






//########################################################################
//########################################################################
//### S O C K E T
//########################################################################
//########################################################################



/**
 *  A socket wrapper that provides cross-platform capability, plus SSL
 */
class TcpSocket
{
public:

    TcpSocket();

    TcpSocket(const std::string &hostname, int port);

    TcpSocket(const char *hostname, int port);

    TcpSocket(const TcpSocket &other);

    virtual ~TcpSocket();

    bool isConnected();

    void enableSSL(bool val);

    bool getEnableSSL();

    bool connect(const std::string &hostname, int portno);

    bool connect(const char *hostname, int portno);

    bool startTls();

    bool connect();

    bool disconnect();

    bool setReceiveTimeout(unsigned long millis);

    long available();

    bool write(int ch);

    bool write(char *str);

    bool write(const std::string &str);

    int read();

    std::string readLine();

private:
    void init();

    std::string hostname;
    int  portno;
    int  sock;
    bool connected;

    bool sslEnabled;

    unsigned long receiveTimeout;

#ifdef HAVE_SSL
    SSL_CTX *sslContext;
    SSL *sslStream;
#endif

};






} //namespace Pedro

#endif /* __PEDROUTIL_H__ */

//########################################################################
//# E N D    O F     F I L E
//########################################################################

