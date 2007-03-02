/**
 * Simple build automation tool.
 *
 * Authors:
 *   Bob Jamison
 *
 * Copyright (C) 2006-2007 Bob Jamison
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

/**
 * To use this file, compile with:
 * <pre>
 * g++ -O3 buildtool.cpp -o btool.exe
 * (or whatever your compiler might be) 
 * Then
 * btool
 * or 
 * btool {target}
 * 
 * Note: recent win32api builds from MinGW have gettimeofday()
 * defined, so you might need to build with 
 * g++ -O3 -DHAVE_GETTIMEOFDAY buildtool.cpp -o btool.exe
 *     
 */  

#define BUILDTOOL_VERSION  "BuildTool v0.6.2, 2007 Bob Jamison"

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>

#include <string>
#include <map>
#include <set>
#include <vector>

#ifdef __WIN32__
#include <windows.h>
#endif


#include <errno.h>


//########################################################################
//# Definition of gettimeofday() for those who don't have it
//########################################################################
#ifndef HAVE_GETTIMEOFDAY
#include <sys/timeb.h>

struct timezone {
      int tz_minuteswest; /* minutes west of Greenwich */
      int tz_dsttime;     /* type of dst correction */
    };

static int gettimeofday (struct timeval *tv, struct timezone *tz)
{
   struct _timeb tb;

   if (!tv)
      return (-1);

    _ftime (&tb);
    tv->tv_sec  = tb.time;
    tv->tv_usec = tb.millitm * 1000 + 500;
    if (tz)
        {
        tz->tz_minuteswest = -60 * _timezone;
        tz->tz_dsttime = _daylight;
        }
    return 0;
}

#endif







namespace buildtool
{




//########################################################################
//########################################################################
//##  R E G E X P
//########################################################################
//########################################################################

/**
 * This is the T-Rex regular expression library, which we
 * gratefully acknowledge.  It's clean code and small size allow
 * us to embed it in BuildTool without adding a dependency
 *
 */    

//begin trex.h

#ifndef _TREX_H_
#define _TREX_H_
/***************************************************************
    T-Rex a tiny regular expression library

    Copyright (C) 2003-2006 Alberto Demichelis

    This software is provided 'as-is', without any express 
    or implied warranty. In no event will the authors be held 
    liable for any damages arising from the use of this software.

    Permission is granted to anyone to use this software for 
    any purpose, including commercial applications, and to alter
    it and redistribute it freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented;
        you must not claim that you wrote the original software.
        If you use this software in a product, an acknowledgment
        in the product documentation would be appreciated but
        is not required.

        2. Altered source versions must be plainly marked as such,
        and must not be misrepresented as being the original software.

        3. This notice may not be removed or altered from any
        source distribution.

****************************************************************/

#ifdef _UNICODE
#define TRexChar unsigned short
#define MAX_CHAR 0xFFFF
#define _TREXC(c) L##c 
#define trex_strlen wcslen
#define trex_printf wprintf
#else
#define TRexChar char
#define MAX_CHAR 0xFF
#define _TREXC(c) (c) 
#define trex_strlen strlen
#define trex_printf printf
#endif

#ifndef TREX_API
#define TREX_API extern
#endif

#define TRex_True 1
#define TRex_False 0

typedef unsigned int TRexBool;
typedef struct TRex TRex;

typedef struct {
    const TRexChar *begin;
    int len;
} TRexMatch;

TREX_API TRex *trex_compile(const TRexChar *pattern,const TRexChar **error);
TREX_API void trex_free(TRex *exp);
TREX_API TRexBool trex_match(TRex* exp,const TRexChar* text);
TREX_API TRexBool trex_search(TRex* exp,const TRexChar* text, const TRexChar** out_begin, const TRexChar** out_end);
TREX_API TRexBool trex_searchrange(TRex* exp,const TRexChar* text_begin,const TRexChar* text_end,const TRexChar** out_begin, const TRexChar** out_end);
TREX_API int trex_getsubexpcount(TRex* exp);
TREX_API TRexBool trex_getsubexp(TRex* exp, int n, TRexMatch *subexp);

#endif

//end trex.h

//start trex.c


#include <stdio.h>
#include <string>

/* see copyright notice in trex.h */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <setjmp.h>
//#include "trex.h"

#ifdef _UINCODE
#define scisprint iswprint
#define scstrlen wcslen
#define scprintf wprintf
#define _SC(x) L(x)
#else
#define scisprint isprint
#define scstrlen strlen
#define scprintf printf
#define _SC(x) (x)
#endif

#ifdef _DEBUG
#include <stdio.h>

static const TRexChar *g_nnames[] =
{
    _SC("NONE"),_SC("OP_GREEDY"),    _SC("OP_OR"),
    _SC("OP_EXPR"),_SC("OP_NOCAPEXPR"),_SC("OP_DOT"),    _SC("OP_CLASS"),
    _SC("OP_CCLASS"),_SC("OP_NCLASS"),_SC("OP_RANGE"),_SC("OP_CHAR"),
    _SC("OP_EOL"),_SC("OP_BOL"),_SC("OP_WB")
};

#endif
#define OP_GREEDY        (MAX_CHAR+1) // * + ? {n}
#define OP_OR            (MAX_CHAR+2)
#define OP_EXPR            (MAX_CHAR+3) //parentesis ()
#define OP_NOCAPEXPR    (MAX_CHAR+4) //parentesis (?:)
#define OP_DOT            (MAX_CHAR+5)
#define OP_CLASS        (MAX_CHAR+6)
#define OP_CCLASS        (MAX_CHAR+7)
#define OP_NCLASS        (MAX_CHAR+8) //negates class the [^
#define OP_RANGE        (MAX_CHAR+9)
#define OP_CHAR            (MAX_CHAR+10)
#define OP_EOL            (MAX_CHAR+11)
#define OP_BOL            (MAX_CHAR+12)
#define OP_WB            (MAX_CHAR+13)

#define TREX_SYMBOL_ANY_CHAR ('.')
#define TREX_SYMBOL_GREEDY_ONE_OR_MORE ('+')
#define TREX_SYMBOL_GREEDY_ZERO_OR_MORE ('*')
#define TREX_SYMBOL_GREEDY_ZERO_OR_ONE ('?')
#define TREX_SYMBOL_BRANCH ('|')
#define TREX_SYMBOL_END_OF_STRING ('$')
#define TREX_SYMBOL_BEGINNING_OF_STRING ('^')
#define TREX_SYMBOL_ESCAPE_CHAR ('\\')


typedef int TRexNodeType;

typedef struct tagTRexNode{
    TRexNodeType type;
    int left;
    int right;
    int next;
}TRexNode;

struct TRex{
    const TRexChar *_eol;
    const TRexChar *_bol;
    const TRexChar *_p;
    int _first;
    int _op;
    TRexNode *_nodes;
    int _nallocated;
    int _nsize;
    int _nsubexpr;
    TRexMatch *_matches;
    int _currsubexp;
    void *_jmpbuf;
    const TRexChar **_error;
};

static int trex_list(TRex *exp);

static int trex_newnode(TRex *exp, TRexNodeType type)
{
    TRexNode n;
    int newid;
    n.type = type;
    n.next = n.right = n.left = -1;
    if(type == OP_EXPR)
        n.right = exp->_nsubexpr++;
    if(exp->_nallocated < (exp->_nsize + 1)) {
        //int oldsize = exp->_nallocated;
        exp->_nallocated *= 2;
        exp->_nodes = (TRexNode *)realloc(exp->_nodes, exp->_nallocated * sizeof(TRexNode));
    }
    exp->_nodes[exp->_nsize++] = n;
    newid = exp->_nsize - 1;
    return (int)newid;
}

static void trex_error(TRex *exp,const TRexChar *error)
{
    if(exp->_error) *exp->_error = error;
    longjmp(*((jmp_buf*)exp->_jmpbuf),-1);
}

static void trex_expect(TRex *exp, int n){
    if((*exp->_p) != n) 
        trex_error(exp, _SC("expected paren"));
    exp->_p++;
}

static TRexChar trex_escapechar(TRex *exp)
{
    if(*exp->_p == TREX_SYMBOL_ESCAPE_CHAR){
        exp->_p++;
        switch(*exp->_p) {
        case 'v': exp->_p++; return '\v';
        case 'n': exp->_p++; return '\n';
        case 't': exp->_p++; return '\t';
        case 'r': exp->_p++; return '\r';
        case 'f': exp->_p++; return '\f';
        default: return (*exp->_p++);
        }
    } else if(!scisprint(*exp->_p)) trex_error(exp,_SC("letter expected"));
    return (*exp->_p++);
}

static int trex_charclass(TRex *exp,int classid)
{
    int n = trex_newnode(exp,OP_CCLASS);
    exp->_nodes[n].left = classid;
    return n;
}

static int trex_charnode(TRex *exp,TRexBool isclass)
{
    TRexChar t;
    if(*exp->_p == TREX_SYMBOL_ESCAPE_CHAR) {
        exp->_p++;
        switch(*exp->_p) {
            case 'n': exp->_p++; return trex_newnode(exp,'\n');
            case 't': exp->_p++; return trex_newnode(exp,'\t');
            case 'r': exp->_p++; return trex_newnode(exp,'\r');
            case 'f': exp->_p++; return trex_newnode(exp,'\f');
            case 'v': exp->_p++; return trex_newnode(exp,'\v');
            case 'a': case 'A': case 'w': case 'W': case 's': case 'S': 
            case 'd': case 'D': case 'x': case 'X': case 'c': case 'C': 
            case 'p': case 'P': case 'l': case 'u': 
                {
                t = *exp->_p; exp->_p++; 
                return trex_charclass(exp,t);
                }
            case 'b': 
            case 'B':
                if(!isclass) {
                    int node = trex_newnode(exp,OP_WB);
                    exp->_nodes[node].left = *exp->_p;
                    exp->_p++; 
                    return node;
                } //else default
            default: 
                t = *exp->_p; exp->_p++; 
                return trex_newnode(exp,t);
        }
    }
    else if(!scisprint(*exp->_p)) {
        
        trex_error(exp,_SC("letter expected"));
    }
    t = *exp->_p; exp->_p++; 
    return trex_newnode(exp,t);
}
static int trex_class(TRex *exp)
{
    int ret = -1;
    int first = -1,chain;
    if(*exp->_p == TREX_SYMBOL_BEGINNING_OF_STRING){
        ret = trex_newnode(exp,OP_NCLASS);
        exp->_p++;
    }else ret = trex_newnode(exp,OP_CLASS);
    
    if(*exp->_p == ']') trex_error(exp,_SC("empty class"));
    chain = ret;
    while(*exp->_p != ']' && exp->_p != exp->_eol) {
        if(*exp->_p == '-' && first != -1){ 
            int r,t;
            if(*exp->_p++ == ']') trex_error(exp,_SC("unfinished range"));
            r = trex_newnode(exp,OP_RANGE);
            if(first>*exp->_p) trex_error(exp,_SC("invalid range"));
            if(exp->_nodes[first].type == OP_CCLASS) trex_error(exp,_SC("cannot use character classes in ranges"));
            exp->_nodes[r].left = exp->_nodes[first].type;
            t = trex_escapechar(exp);
            exp->_nodes[r].right = t;
            exp->_nodes[chain].next = r;
            chain = r;
            first = -1;
        }
        else{
            if(first!=-1){
                int c = first;
                exp->_nodes[chain].next = c;
                chain = c;
                first = trex_charnode(exp,TRex_True);
            }
            else{
                first = trex_charnode(exp,TRex_True);
            }
        }
    }
    if(first!=-1){
        int c = first;
        exp->_nodes[chain].next = c;
        chain = c;
        first = -1;
    }
    /* hack? */
    exp->_nodes[ret].left = exp->_nodes[ret].next;
    exp->_nodes[ret].next = -1;
    return ret;
}

static int trex_parsenumber(TRex *exp)
{
    int ret = *exp->_p-'0';
    int positions = 10;
    exp->_p++;
    while(isdigit(*exp->_p)) {
        ret = ret*10+(*exp->_p++-'0');
        if(positions==1000000000) trex_error(exp,_SC("overflow in numeric constant"));
        positions *= 10;
    };
    return ret;
}

static int trex_element(TRex *exp)
{
    int ret = -1;
    switch(*exp->_p)
    {
    case '(': {
        int expr,newn;
        exp->_p++;


        if(*exp->_p =='?') {
            exp->_p++;
            trex_expect(exp,':');
            expr = trex_newnode(exp,OP_NOCAPEXPR);
        }
        else
            expr = trex_newnode(exp,OP_EXPR);
        newn = trex_list(exp);
        exp->_nodes[expr].left = newn;
        ret = expr;
        trex_expect(exp,')');
              }
              break;
    case '[':
        exp->_p++;
        ret = trex_class(exp);
        trex_expect(exp,']');
        break;
    case TREX_SYMBOL_END_OF_STRING: exp->_p++; ret = trex_newnode(exp,OP_EOL);break;
    case TREX_SYMBOL_ANY_CHAR: exp->_p++; ret = trex_newnode(exp,OP_DOT);break;
    default:
        ret = trex_charnode(exp,TRex_False);
        break;
    }

    {
        int op;
        TRexBool isgreedy = TRex_False;
        unsigned short p0 = 0, p1 = 0;
        switch(*exp->_p){
            case TREX_SYMBOL_GREEDY_ZERO_OR_MORE: p0 = 0; p1 = 0xFFFF; exp->_p++; isgreedy = TRex_True; break;
            case TREX_SYMBOL_GREEDY_ONE_OR_MORE: p0 = 1; p1 = 0xFFFF; exp->_p++; isgreedy = TRex_True; break;
            case TREX_SYMBOL_GREEDY_ZERO_OR_ONE: p0 = 0; p1 = 1; exp->_p++; isgreedy = TRex_True; break;
            case '{':
                exp->_p++;
                if(!isdigit(*exp->_p)) trex_error(exp,_SC("number expected"));
                p0 = (unsigned short)trex_parsenumber(exp);
                /*******************************/
                switch(*exp->_p) {
            case '}':
                p1 = p0; exp->_p++;
                break;
            case ',':
                exp->_p++;
                p1 = 0xFFFF;
                if(isdigit(*exp->_p)){
                    p1 = (unsigned short)trex_parsenumber(exp);
                }
                trex_expect(exp,'}');
                break;
            default:
                trex_error(exp,_SC(", or } expected"));
        }
        /*******************************/
        isgreedy = TRex_True; 
        break;

        }
        if(isgreedy) {
            int nnode = trex_newnode(exp,OP_GREEDY);
            op = OP_GREEDY;
            exp->_nodes[nnode].left = ret;
            exp->_nodes[nnode].right = ((p0)<<16)|p1;
            ret = nnode;
        }
    }
    if((*exp->_p != TREX_SYMBOL_BRANCH) && (*exp->_p != ')') && (*exp->_p != TREX_SYMBOL_GREEDY_ZERO_OR_MORE) && (*exp->_p != TREX_SYMBOL_GREEDY_ONE_OR_MORE) && (*exp->_p != '\0')) {
        int nnode = trex_element(exp);
        exp->_nodes[ret].next = nnode;
    }

    return ret;
}

static int trex_list(TRex *exp)
{
    int ret=-1,e;
    if(*exp->_p == TREX_SYMBOL_BEGINNING_OF_STRING) {
        exp->_p++;
        ret = trex_newnode(exp,OP_BOL);
    }
    e = trex_element(exp);
    if(ret != -1) {
        exp->_nodes[ret].next = e;
    }
    else ret = e;

    if(*exp->_p == TREX_SYMBOL_BRANCH) {
        int temp,tright;
        exp->_p++;
        temp = trex_newnode(exp,OP_OR);
        exp->_nodes[temp].left = ret;
        tright = trex_list(exp);
        exp->_nodes[temp].right = tright;
        ret = temp;
    }
    return ret;
}

static TRexBool trex_matchcclass(int cclass,TRexChar c)
{
    switch(cclass) {
    case 'a': return isalpha(c)?TRex_True:TRex_False;
    case 'A': return !isalpha(c)?TRex_True:TRex_False;
    case 'w': return (isalnum(c) || c == '_')?TRex_True:TRex_False;
    case 'W': return (!isalnum(c) && c != '_')?TRex_True:TRex_False;
    case 's': return isspace(c)?TRex_True:TRex_False;
    case 'S': return !isspace(c)?TRex_True:TRex_False;
    case 'd': return isdigit(c)?TRex_True:TRex_False;
    case 'D': return !isdigit(c)?TRex_True:TRex_False;
    case 'x': return isxdigit(c)?TRex_True:TRex_False;
    case 'X': return !isxdigit(c)?TRex_True:TRex_False;
    case 'c': return iscntrl(c)?TRex_True:TRex_False;
    case 'C': return !iscntrl(c)?TRex_True:TRex_False;
    case 'p': return ispunct(c)?TRex_True:TRex_False;
    case 'P': return !ispunct(c)?TRex_True:TRex_False;
    case 'l': return islower(c)?TRex_True:TRex_False;
    case 'u': return isupper(c)?TRex_True:TRex_False;
    }
    return TRex_False; /*cannot happen*/
}

static TRexBool trex_matchclass(TRex* exp,TRexNode *node,TRexChar c)
{
    do {
        switch(node->type) {
            case OP_RANGE:
                if(c >= node->left && c <= node->right) return TRex_True;
                break;
            case OP_CCLASS:
                if(trex_matchcclass(node->left,c)) return TRex_True;
                break;
            default:
                if(c == node->type)return TRex_True;
        }
    } while((node->next != -1) && (node = &exp->_nodes[node->next]));
    return TRex_False;
}

static const TRexChar *trex_matchnode(TRex* exp,TRexNode *node,const TRexChar *str,TRexNode *next)
{
    
    TRexNodeType type = node->type;
    switch(type) {
    case OP_GREEDY: {
        //TRexNode *greedystop = (node->next != -1) ? &exp->_nodes[node->next] : NULL;
        TRexNode *greedystop = NULL;
        int p0 = (node->right >> 16)&0x0000FFFF, p1 = node->right&0x0000FFFF, nmaches = 0;
        const TRexChar *s=str, *good = str;

        if(node->next != -1) {
            greedystop = &exp->_nodes[node->next];
        }
        else {
            greedystop = next;
        }

        while((nmaches == 0xFFFF || nmaches < p1)) {

            const TRexChar *stop;
            if(!(s = trex_matchnode(exp,&exp->_nodes[node->left],s,greedystop)))
                break;
            nmaches++;
            good=s;
            if(greedystop) {
                //checks that 0 matches satisfy the expression(if so skips)
                //if not would always stop(for instance if is a '?')
                if(greedystop->type != OP_GREEDY ||
                (greedystop->type == OP_GREEDY && ((greedystop->right >> 16)&0x0000FFFF) != 0))
                {
                    TRexNode *gnext = NULL;
                    if(greedystop->next != -1) {
                        gnext = &exp->_nodes[greedystop->next];
                    }else if(next && next->next != -1){
                        gnext = &exp->_nodes[next->next];
                    }
                    stop = trex_matchnode(exp,greedystop,s,gnext);
                    if(stop) {
                        //if satisfied stop it
                        if(p0 == p1 && p0 == nmaches) break;
                        else if(nmaches >= p0 && p1 == 0xFFFF) break;
                        else if(nmaches >= p0 && nmaches <= p1) break;
                    }
                }
            }
            
            if(s >= exp->_eol)
                break;
        }
        if(p0 == p1 && p0 == nmaches) return good;
        else if(nmaches >= p0 && p1 == 0xFFFF) return good;
        else if(nmaches >= p0 && nmaches <= p1) return good;
        return NULL;
    }
    case OP_OR: {
            const TRexChar *asd = str;
            TRexNode *temp=&exp->_nodes[node->left];
            while( (asd = trex_matchnode(exp,temp,asd,NULL)) ) {
                if(temp->next != -1)
                    temp = &exp->_nodes[temp->next];
                else
                    return asd;
            }
            asd = str;
            temp = &exp->_nodes[node->right];
            while( (asd = trex_matchnode(exp,temp,asd,NULL)) ) {
                if(temp->next != -1)
                    temp = &exp->_nodes[temp->next];
                else
                    return asd;
            }
            return NULL;
            break;
    }
    case OP_EXPR:
    case OP_NOCAPEXPR:{
            TRexNode *n = &exp->_nodes[node->left];
            const TRexChar *cur = str;
            int capture = -1;
            if(node->type != OP_NOCAPEXPR && node->right == exp->_currsubexp) {
                capture = exp->_currsubexp;
                exp->_matches[capture].begin = cur;
                exp->_currsubexp++;
            }
            
            do {
                TRexNode *subnext = NULL;
                if(n->next != -1) {
                    subnext = &exp->_nodes[n->next];
                }else {
                    subnext = next;
                }
                if(!(cur = trex_matchnode(exp,n,cur,subnext))) {
                    if(capture != -1){
                        exp->_matches[capture].begin = 0;
                        exp->_matches[capture].len = 0;
                    }
                    return NULL;
                }
            } while((n->next != -1) && (n = &exp->_nodes[n->next]));

            if(capture != -1) 
                exp->_matches[capture].len = cur - exp->_matches[capture].begin;
            return cur;
    }                 
    case OP_WB:
        if(str == exp->_bol && !isspace(*str)
         || (str == exp->_eol && !isspace(*(str-1)))
         || (!isspace(*str) && isspace(*(str+1)))
         || (isspace(*str) && !isspace(*(str+1))) ) {
            return (node->left == 'b')?str:NULL;
        }
        return (node->left == 'b')?NULL:str;
    case OP_BOL:
        if(str == exp->_bol) return str;
        return NULL;
    case OP_EOL:
        if(str == exp->_eol) return str;
        return NULL;
    case OP_DOT:{
        *str++;
                }
        return str;
    case OP_NCLASS:
    case OP_CLASS:
        if(trex_matchclass(exp,&exp->_nodes[node->left],*str)?(type == OP_CLASS?TRex_True:TRex_False):(type == OP_NCLASS?TRex_True:TRex_False)) {
            *str++;
            return str;
        }
        return NULL;
    case OP_CCLASS:
        if(trex_matchcclass(node->left,*str)) {
            *str++;
            return str;
        }
        return NULL;
    default: /* char */
        if(*str != node->type) return NULL;
        *str++;
        return str;
    }
    return NULL;
}

/* public api */
TRex *trex_compile(const TRexChar *pattern,const TRexChar **error)
{
    TRex *exp = (TRex *)malloc(sizeof(TRex));
    exp->_eol = exp->_bol = NULL;
    exp->_p = pattern;
    exp->_nallocated = (int)scstrlen(pattern) * sizeof(TRexChar);
    exp->_nodes = (TRexNode *)malloc(exp->_nallocated * sizeof(TRexNode));
    exp->_nsize = 0;
    exp->_matches = 0;
    exp->_nsubexpr = 0;
    exp->_first = trex_newnode(exp,OP_EXPR);
    exp->_error = error;
    exp->_jmpbuf = malloc(sizeof(jmp_buf));
    if(setjmp(*((jmp_buf*)exp->_jmpbuf)) == 0) {
        int res = trex_list(exp);
        exp->_nodes[exp->_first].left = res;
        if(*exp->_p!='\0')
            trex_error(exp,_SC("unexpected character"));
#ifdef _DEBUG
        {
            int nsize,i;
            TRexNode *t;
            nsize = exp->_nsize;
            t = &exp->_nodes[0];
            scprintf(_SC("\n"));
            for(i = 0;i < nsize; i++) {
                if(exp->_nodes[i].type>MAX_CHAR)
                    scprintf(_SC("[%02d] %10s "),i,g_nnames[exp->_nodes[i].type-MAX_CHAR]);
                else
                    scprintf(_SC("[%02d] %10c "),i,exp->_nodes[i].type);
                scprintf(_SC("left %02d right %02d next %02d\n"),exp->_nodes[i].left,exp->_nodes[i].right,exp->_nodes[i].next);
            }
            scprintf(_SC("\n"));
        }
#endif
        exp->_matches = (TRexMatch *) malloc(exp->_nsubexpr * sizeof(TRexMatch));
        memset(exp->_matches,0,exp->_nsubexpr * sizeof(TRexMatch));
    }
    else{
        trex_free(exp);
        return NULL;
    }
    return exp;
}

void trex_free(TRex *exp)
{
    if(exp)    {
        if(exp->_nodes) free(exp->_nodes);
        if(exp->_jmpbuf) free(exp->_jmpbuf);
        if(exp->_matches) free(exp->_matches);
        free(exp);
    }
}

TRexBool trex_match(TRex* exp,const TRexChar* text)
{
    const TRexChar* res = NULL;
    exp->_bol = text;
    exp->_eol = text + scstrlen(text);
    exp->_currsubexp = 0;
    res = trex_matchnode(exp,exp->_nodes,text,NULL);
    if(res == NULL || res != exp->_eol)
        return TRex_False;
    return TRex_True;
}

TRexBool trex_searchrange(TRex* exp,const TRexChar* text_begin,const TRexChar* text_end,const TRexChar** out_begin, const TRexChar** out_end)
{
    const TRexChar *cur = NULL;
    int node = exp->_first;
    if(text_begin >= text_end) return TRex_False;
    exp->_bol = text_begin;
    exp->_eol = text_end;
    do {
        cur = text_begin;
        while(node != -1) {
            exp->_currsubexp = 0;
            cur = trex_matchnode(exp,&exp->_nodes[node],cur,NULL);
            if(!cur)
                break;
            node = exp->_nodes[node].next;
        }
        *text_begin++;
    } while(cur == NULL && text_begin != text_end);

    if(cur == NULL)
        return TRex_False;

    --text_begin;

    if(out_begin) *out_begin = text_begin;
    if(out_end) *out_end = cur;
    return TRex_True;
}

TRexBool trex_search(TRex* exp,const TRexChar* text, const TRexChar** out_begin, const TRexChar** out_end)
{
    return trex_searchrange(exp,text,text + scstrlen(text),out_begin,out_end);
}

int trex_getsubexpcount(TRex* exp)
{
    return exp->_nsubexpr;
}

TRexBool trex_getsubexp(TRex* exp, int n, TRexMatch *subexp)
{
    if( n<0 || n >= exp->_nsubexpr) return TRex_False;
    *subexp = exp->_matches[n];
    return TRex_True;
}


//########################################################################
//########################################################################
//##  E N D    R E G E X P
//########################################################################
//########################################################################





//########################################################################
//########################################################################
//##  X M L
//########################################################################
//########################################################################

// Note:  This mini-dom library comes from Pedro, another little project
// of mine.

typedef std::string String;
typedef unsigned int XMLCh;


class Namespace
{
public:
    Namespace()
        {}

    Namespace(const String &prefixArg, const String &namespaceURIArg)
        {
        prefix       = prefixArg;
        namespaceURI = namespaceURIArg;
        }

    Namespace(const Namespace &other)
        {
        assign(other);
        }

    Namespace &operator=(const Namespace &other)
        {
        assign(other);
        return *this;
        }

    virtual ~Namespace()
        {}

    virtual String getPrefix()
        { return prefix; }

    virtual String getNamespaceURI()
        { return namespaceURI; }

protected:

    void assign(const Namespace &other)
        {
        prefix       = other.prefix;
        namespaceURI = other.namespaceURI;
        }

    String prefix;
    String namespaceURI;

};

class Attribute
{
public:
    Attribute()
        {}

    Attribute(const String &nameArg, const String &valueArg)
        {
        name  = nameArg;
        value = valueArg;
        }

    Attribute(const Attribute &other)
        {
        assign(other);
        }

    Attribute &operator=(const Attribute &other)
        {
        assign(other);
        return *this;
        }

    virtual ~Attribute()
        {}

    virtual String getName()
        { return name; }

    virtual String getValue()
        { return value; }

protected:

    void assign(const Attribute &other)
        {
        name  = other.name;
        value = other.value;
        }

    String name;
    String value;

};


class Element
{
friend class Parser;

public:
    Element()
        {
        parent = NULL;
        }

    Element(const String &nameArg)
        {
        parent = NULL;
        name   = nameArg;
        }

    Element(const String &nameArg, const String &valueArg)
        {
        parent = NULL;
        name   = nameArg;
        value  = valueArg;
        }

    Element(const Element &other)
        {
        assign(other);
        }

    Element &operator=(const Element &other)
        {
        assign(other);
        return *this;
        }

    virtual Element *clone();

    virtual ~Element()
        {
        for (unsigned int i=0 ; i<children.size() ; i++)
            delete children[i];
        }

    virtual String getName()
        { return name; }

    virtual String getValue()
        { return value; }

    Element *getParent()
        { return parent; }

    std::vector<Element *> getChildren()
        { return children; }

    std::vector<Element *> findElements(const String &name);

    String getAttribute(const String &name);

    std::vector<Attribute> &getAttributes()
        { return attributes; } 

    String getTagAttribute(const String &tagName, const String &attrName);

    String getTagValue(const String &tagName);

    void addChild(Element *child);

    void addAttribute(const String &name, const String &value);

    void addNamespace(const String &prefix, const String &namespaceURI);


    /**
     * Prettyprint an XML tree to an output stream.  Elements are indented
     * according to element hierarchy.
     * @param f a stream to receive the output
     * @param elem the element to output
     */
    void writeIndented(FILE *f);

    /**
     * Prettyprint an XML tree to standard output.  This is the equivalent of
     * writeIndented(stdout).
     * @param elem the element to output
     */
    void print();

protected:

    void assign(const Element &other)
        {
        parent     = other.parent;
        children   = other.children;
        attributes = other.attributes;
        namespaces = other.namespaces;
        name       = other.name;
        value      = other.value;
        }

    void findElementsRecursive(std::vector<Element *>&res, const String &name);

    void writeIndentedRecursive(FILE *f, int indent);

    Element *parent;

    std::vector<Element *>children;

    std::vector<Attribute> attributes;
    std::vector<Namespace> namespaces;

    String name;
    String value;

};





class Parser
{
public:
    /**
     * Constructor
     */
    Parser()
        { init(); }

    virtual ~Parser()
        {}

    /**
     * Parse XML in a char buffer.
     * @param buf a character buffer to parse
     * @param pos position to start parsing
     * @param len number of chars, from pos, to parse.
     * @return a pointer to the root of the XML document;
     */
    Element *parse(const char *buf,int pos,int len);

    /**
     * Parse XML in a char buffer.
     * @param buf a character buffer to parse
     * @param pos position to start parsing
     * @param len number of chars, from pos, to parse.
     * @return a pointer to the root of the XML document;
     */
    Element *parse(const String &buf);

    /**
     * Parse a named XML file.  The file is loaded like a data file;
     * the original format is not preserved.
     * @param fileName the name of the file to read
     * @return a pointer to the root of the XML document;
     */
    Element *parseFile(const String &fileName);

    /**
     * Utility method to preprocess a string for XML
     * output, escaping its entities.
     * @param str the string to encode
     */
    static String encode(const String &str);

    /**
     *  Removes whitespace from beginning and end of a string
     */
    String trim(const String &s);

private:

    void init()
        {
        keepGoing       = true;
        currentNode     = NULL;
        parselen        = 0;
        parsebuf        = NULL;
        currentPosition = 0;
        }

    void getLineAndColumn(long pos, long *lineNr, long *colNr);

    void error(char *fmt, ...);

    int peek(long pos);

    int match(long pos, const char *text);

    int skipwhite(long p);

    int getWord(int p0, String &buf);

    int getQuoted(int p0, String &buf, int do_i_parse);

    int parseVersion(int p0);

    int parseDoctype(int p0);

    int parseElement(int p0, Element *par,int depth);

    Element *parse(XMLCh *buf,int pos,int len);

    bool       keepGoing;
    Element    *currentNode;
    long       parselen;
    XMLCh      *parsebuf;
    String  cdatabuf;
    long       currentPosition;
    int        colNr;

};




//########################################################################
//# E L E M E N T
//########################################################################

Element *Element::clone()
{
    Element *elem = new Element(name, value);
    elem->parent     = parent;
    elem->attributes = attributes;
    elem->namespaces = namespaces;

    std::vector<Element *>::iterator iter;
    for (iter = children.begin(); iter != children.end() ; iter++)
        {
        elem->addChild((*iter)->clone());
        }
    return elem;
}


void Element::findElementsRecursive(std::vector<Element *>&res, const String &name)
{
    if (getName() == name)
        {
        res.push_back(this);
        }
    for (unsigned int i=0; i<children.size() ; i++)
        children[i]->findElementsRecursive(res, name);
}

std::vector<Element *> Element::findElements(const String &name)
{
    std::vector<Element *> res;
    findElementsRecursive(res, name);
    return res;
}

String Element::getAttribute(const String &name)
{
    for (unsigned int i=0 ; i<attributes.size() ; i++)
        if (attributes[i].getName() ==name)
            return attributes[i].getValue();
    return "";
}

String Element::getTagAttribute(const String &tagName, const String &attrName)
{
    std::vector<Element *>elems = findElements(tagName);
    if (elems.size() <1)
        return "";
    String res = elems[0]->getAttribute(attrName);
    return res;
}

String Element::getTagValue(const String &tagName)
{
    std::vector<Element *>elems = findElements(tagName);
    if (elems.size() <1)
        return "";
    String res = elems[0]->getValue();
    return res;
}

void Element::addChild(Element *child)
{
    if (!child)
        return;
    child->parent = this;
    children.push_back(child);
}


void Element::addAttribute(const String &name, const String &value)
{
    Attribute attr(name, value);
    attributes.push_back(attr);
}

void Element::addNamespace(const String &prefix, const String &namespaceURI)
{
    Namespace ns(prefix, namespaceURI);
    namespaces.push_back(ns);
}

void Element::writeIndentedRecursive(FILE *f, int indent)
{
    int i;
    if (!f)
        return;
    //Opening tag, and attributes
    for (i=0;i<indent;i++)
        fputc(' ',f);
    fprintf(f,"<%s",name.c_str());
    for (unsigned int i=0 ; i<attributes.size() ; i++)
        {
        fprintf(f," %s=\"%s\"",
              attributes[i].getName().c_str(),
              attributes[i].getValue().c_str());
        }
    for (unsigned int i=0 ; i<namespaces.size() ; i++)
        {
        fprintf(f," xmlns:%s=\"%s\"",
              namespaces[i].getPrefix().c_str(),
              namespaces[i].getNamespaceURI().c_str());
        }
    fprintf(f,">\n");

    //Between the tags
    if (value.size() > 0)
        {
        for (int i=0;i<indent;i++)
            fputc(' ', f);
        fprintf(f," %s\n", value.c_str());
        }

    for (unsigned int i=0 ; i<children.size() ; i++)
        children[i]->writeIndentedRecursive(f, indent+2);

    //Closing tag
    for (int i=0; i<indent; i++)
        fputc(' ',f);
    fprintf(f,"</%s>\n", name.c_str());
}

void Element::writeIndented(FILE *f)
{
    writeIndentedRecursive(f, 0);
}

void Element::print()
{
    writeIndented(stdout);
}


//########################################################################
//# P A R S E R
//########################################################################



typedef struct
    {
    char *escaped;
    char value;
    } EntityEntry;

static EntityEntry entities[] =
{
    { "&amp;" , '&'  },
    { "&lt;"  , '<'  },
    { "&gt;"  , '>'  },
    { "&apos;", '\'' },
    { "&quot;", '"'  },
    { NULL    , '\0' }
};



/**
 *  Removes whitespace from beginning and end of a string
 */
String Parser::trim(const String &s)
{
    if (s.size() < 1)
        return s;
    
    //Find first non-ws char
    unsigned int begin = 0;
    for ( ; begin < s.size() ; begin++)
        {
        if (!isspace(s[begin]))
            break;
        }

    //Find first non-ws char, going in reverse
    unsigned int end = s.size() - 1;
    for ( ; end > begin ; end--)
        {
        if (!isspace(s[end]))
            break;
        }
    //trace("begin:%d  end:%d", begin, end);

    String res = s.substr(begin, end-begin+1);
    return res;
}

void Parser::getLineAndColumn(long pos, long *lineNr, long *colNr)
{
    long line = 1;
    long col  = 1;
    for (long i=0 ; i<pos ; i++)
        {
        XMLCh ch = parsebuf[i];
        if (ch == '\n' || ch == '\r')
            {
            col = 0;
            line ++;
            }
        else
            col++;
        }
    *lineNr = line;
    *colNr  = col;

}


void Parser::error(char *fmt, ...)
{
    long lineNr;
    long colNr;
    getLineAndColumn(currentPosition, &lineNr, &colNr);
    va_list args;
    fprintf(stderr, "xml error at line %ld, column %ld:", lineNr, colNr);
    va_start(args,fmt);
    vfprintf(stderr,fmt,args);
    va_end(args) ;
    fprintf(stderr, "\n");
}



int Parser::peek(long pos)
{
    if (pos >= parselen)
        return -1;
    currentPosition = pos;
    int ch = parsebuf[pos];
    //printf("ch:%c\n", ch);
    return ch;
}



String Parser::encode(const String &str)
{
    String ret;
    for (unsigned int i=0 ; i<str.size() ; i++)
        {
        XMLCh ch = (XMLCh)str[i];
        if (ch == '&')
            ret.append("&amp;");
        else if (ch == '<')
            ret.append("&lt;");
        else if (ch == '>')
            ret.append("&gt;");
        else if (ch == '\'')
            ret.append("&apos;");
        else if (ch == '"')
            ret.append("&quot;");
        else
            ret.push_back(ch);

        }
    return ret;
}


int Parser::match(long p0, const char *text)
{
    int p = p0;
    while (*text)
        {
        if (peek(p) != *text)
            return p0;
        p++; text++;
        }
    return p;
}



int Parser::skipwhite(long p)
{

    while (p<parselen)
        {
        int p2 = match(p, "<!--");
        if (p2 > p)
            {
            p = p2;
            while (p<parselen)
              {
              p2 = match(p, "-->");
              if (p2 > p)
                  {
                  p = p2;
                  break;
                  }
              p++;
              }
          }
      XMLCh b = peek(p);
      if (!isspace(b))
          break;
      p++;
      }
  return p;
}

/* modify this to allow all chars for an element or attribute name*/
int Parser::getWord(int p0, String &buf)
{
    int p = p0;
    while (p<parselen)
        {
        XMLCh b = peek(p);
        if (b<=' ' || b=='/' || b=='>' || b=='=')
            break;
        buf.push_back(b);
        p++;
        }
    return p;
}

int Parser::getQuoted(int p0, String &buf, int do_i_parse)
{

    int p = p0;
    if (peek(p) != '"' && peek(p) != '\'')
        return p0;
    p++;

    while ( p<parselen )
        {
        XMLCh b = peek(p);
        if (b=='"' || b=='\'')
            break;
        if (b=='&' && do_i_parse)
            {
            bool found = false;
            for (EntityEntry *ee = entities ; ee->value ; ee++)
                {
                int p2 = match(p, ee->escaped);
                if (p2>p)
                    {
                    buf.push_back(ee->value);
                    p = p2;
                    found = true;
                    break;
                    }
                }
            if (!found)
                {
                error("unterminated entity");
                return false;
                }
            }
        else
            {
            buf.push_back(b);
            p++;
            }
        }
    return p;
}

int Parser::parseVersion(int p0)
{
    //printf("### parseVersion: %d\n", p0);

    int p = p0;

    p = skipwhite(p0);

    if (peek(p) != '<')
        return p0;

    p++;
    if (p>=parselen || peek(p)!='?')
        return p0;

    p++;

    String buf;

    while (p<parselen)
        {
        XMLCh ch = peek(p);
        if (ch=='?')
            {
            p++;
            break;
            }
        buf.push_back(ch);
        p++;
        }

    if (peek(p) != '>')
        return p0;
    p++;

    //printf("Got version:%s\n",buf.c_str());
    return p;
}

int Parser::parseDoctype(int p0)
{
    //printf("### parseDoctype: %d\n", p0);

    int p = p0;
    p = skipwhite(p);

    if (p>=parselen || peek(p)!='<')
        return p0;

    p++;

    if (peek(p)!='!' || peek(p+1)=='-')
        return p0;
    p++;

    String buf;
    while (p<parselen)
        {
        XMLCh ch = peek(p);
        if (ch=='>')
            {
            p++;
            break;
            }
        buf.push_back(ch);
        p++;
        }

    //printf("Got doctype:%s\n",buf.c_str());
    return p;
}

int Parser::parseElement(int p0, Element *par,int depth)
{

    int p = p0;

    int p2 = p;

    p = skipwhite(p);

    //## Get open tag
    XMLCh ch = peek(p);
    if (ch!='<')
        return p0;

    p++;

    String openTagName;
    p = skipwhite(p);
    p = getWord(p, openTagName);
    //printf("####tag :%s\n", openTagName.c_str());
    p = skipwhite(p);

    //Add element to tree
    Element *n = new Element(openTagName);
    n->parent = par;
    par->addChild(n);

    // Get attributes
    if (peek(p) != '>')
        {
        while (p<parselen)
            {
            p = skipwhite(p);
            ch = peek(p);
            //printf("ch:%c\n",ch);
            if (ch=='>')
                break;
            else if (ch=='/' && p<parselen+1)
                {
                p++;
                p = skipwhite(p);
                ch = peek(p);
                if (ch=='>')
                    {
                    p++;
                    //printf("quick close\n");
                    return p;
                    }
                }
            String attrName;
            p2 = getWord(p, attrName);
            if (p2==p)
                break;
            //printf("name:%s",buf);
            p=p2;
            p = skipwhite(p);
            ch = peek(p);
            //printf("ch:%c\n",ch);
            if (ch!='=')
                break;
            p++;
            p = skipwhite(p);
            // ch = parsebuf[p];
            // printf("ch:%c\n",ch);
            String attrVal;
            p2 = getQuoted(p, attrVal, true);
            p=p2+1;
            //printf("name:'%s'   value:'%s'\n",attrName.c_str(),attrVal.c_str());
            char *namestr = (char *)attrName.c_str();
            if (strncmp(namestr, "xmlns:", 6)==0)
                n->addNamespace(attrName, attrVal);
            else
                n->addAttribute(attrName, attrVal);
            }
        }

    bool cdata = false;

    p++;
    // ### Get intervening data ### */
    String data;
    while (p<parselen)
        {
        //# COMMENT
        p2 = match(p, "<!--");
        if (!cdata && p2>p)
            {
            p = p2;
            while (p<parselen)
                {
                p2 = match(p, "-->");
                if (p2 > p)
                    {
                    p = p2;
                    break;
                    }
                p++;
                }
            }

        ch = peek(p);
        //# END TAG
        if (ch=='<' && !cdata && peek(p+1)=='/')
            {
            break;
            }
        //# CDATA
        p2 = match(p, "<![CDATA[");
        if (p2 > p)
            {
            cdata = true;
            p = p2;
            continue;
            }

        //# CHILD ELEMENT
        if (ch == '<')
            {
            p2 = parseElement(p, n, depth+1);
            if (p2 == p)
                {
                /*
                printf("problem on element:%s.  p2:%d p:%d\n",
                      openTagName.c_str(), p2, p);
                */
                return p0;
                }
            p = p2;
            continue;
            }
        //# ENTITY
        if (ch=='&' && !cdata)
            {
            bool found = false;
            for (EntityEntry *ee = entities ; ee->value ; ee++)
                {
                int p2 = match(p, ee->escaped);
                if (p2>p)
                    {
                    data.push_back(ee->value);
                    p = p2;
                    found = true;
                    break;
                    }
                }
            if (!found)
                {
                error("unterminated entity");
                return -1;
                }
            continue;
            }

        //# NONE OF THE ABOVE
        data.push_back(ch);
        p++;
        }/*while*/


    n->value = data;
    //printf("%d : data:%s\n",p,data.c_str());

    //## Get close tag
    p = skipwhite(p);
    ch = peek(p);
    if (ch != '<')
        {
        error("no < for end tag\n");
        return p0;
        }
    p++;
    ch = peek(p);
    if (ch != '/')
        {
        error("no / on end tag");
        return p0;
        }
    p++;
    ch = peek(p);
    p = skipwhite(p);
    String closeTagName;
    p = getWord(p, closeTagName);
    if (openTagName != closeTagName)
        {
        error("Mismatched closing tag.  Expected </%S>. Got '%S'.",
                openTagName.c_str(), closeTagName.c_str());
        return p0;
        }
    p = skipwhite(p);
    if (peek(p) != '>')
        {
        error("no > on end tag for '%s'", closeTagName.c_str());
        return p0;
        }
    p++;
    // printf("close element:%s\n",closeTagName.c_str());
    p = skipwhite(p);
    return p;
}




Element *Parser::parse(XMLCh *buf,int pos,int len)
{
    parselen = len;
    parsebuf = buf;
    Element *rootNode = new Element("root");
    pos = parseVersion(pos);
    pos = parseDoctype(pos);
    pos = parseElement(pos, rootNode, 0);
    return rootNode;
}


Element *Parser::parse(const char *buf, int pos, int len)
{
    XMLCh *charbuf = new XMLCh[len + 1];
    long i = 0;
    for ( ; i < len ; i++)
        charbuf[i] = (XMLCh)buf[i];
    charbuf[i] = '\0';

    Element *n = parse(charbuf, pos, len);
    delete[] charbuf;
    return n;
}

Element *Parser::parse(const String &buf)
{
    long len = (long)buf.size();
    XMLCh *charbuf = new XMLCh[len + 1];
    long i = 0;
    for ( ; i < len ; i++)
        charbuf[i] = (XMLCh)buf[i];
    charbuf[i] = '\0';

    Element *n = parse(charbuf, 0, len);
    delete[] charbuf;
    return n;
}

Element *Parser::parseFile(const String &fileName)
{

    //##### LOAD INTO A CHAR BUF, THEN CONVERT TO XMLCh
    FILE *f = fopen(fileName.c_str(), "rb");
    if (!f)
        return NULL;

    struct stat  statBuf;
    if (fstat(fileno(f),&statBuf)<0)
        {
        fclose(f);
        return NULL;
        }
    long filelen = statBuf.st_size;

    //printf("length:%d\n",filelen);
    XMLCh *charbuf = new XMLCh[filelen + 1];
    for (XMLCh *p=charbuf ; !feof(f) ; p++)
        {
        *p = (XMLCh)fgetc(f);
        }
    fclose(f);
    charbuf[filelen] = '\0';


    /*
    printf("nrbytes:%d\n",wc_count);
    printf("buf:%ls\n======\n",charbuf);
    */
    Element *n = parse(charbuf, 0, filelen);
    delete[] charbuf;
    return n;
}



//########################################################################
//########################################################################
//##  E N D    X M L
//########################################################################
//########################################################################


//########################################################################
//########################################################################
//##  U R I
//########################################################################
//########################################################################

//This would normally be a call to a UNICODE function
#define isLetter(x) isalpha(x)

/**
 *  A class that implements the W3C URI resource reference.
 */
class URI
{
public:

    typedef enum
        {
        SCHEME_NONE =0,
        SCHEME_DATA,
        SCHEME_HTTP,
        SCHEME_HTTPS,
        SCHEME_FTP,
        SCHEME_FILE,
        SCHEME_LDAP,
        SCHEME_MAILTO,
        SCHEME_NEWS,
        SCHEME_TELNET
        } SchemeTypes;

    /**
     *
     */
    URI()
        {
        init();
        }

    /**
     *
     */
    URI(const String &str)
        {
        init();
        parse(str);
        }


    /**
     *
     */
    URI(const char *str)
        {
        init();
        String domStr = str;
        parse(domStr);
        }


    /**
     *
     */
    URI(const URI &other)
        {
        init();
        assign(other);
        }


    /**
     *
     */
    URI &operator=(const URI &other)
        {
        init();
        assign(other);
        return *this;
        }


    /**
     *
     */
    virtual ~URI()
        {}



    /**
     *
     */
    virtual bool parse(const String &str);

    /**
     *
     */
    virtual String toString() const;

    /**
     *
     */
    virtual int getScheme() const;

    /**
     *
     */
    virtual String getSchemeStr() const;

    /**
     *
     */
    virtual String getAuthority() const;

    /**
     *  Same as getAuthority, but if the port has been specified
     *  as host:port , the port will not be included
     */
    virtual String getHost() const;

    /**
     *
     */
    virtual int getPort() const;

    /**
     *
     */
    virtual String getPath() const;

    /**
     *
     */
    virtual String getNativePath() const;

    /**
     *
     */
    virtual bool isAbsolute() const;

    /**
     *
     */
    virtual bool isOpaque() const;

    /**
     *
     */
    virtual String getQuery() const;

    /**
     *
     */
    virtual String getFragment() const;

    /**
     *
     */
    virtual URI resolve(const URI &other) const;

    /**
     *
     */
    virtual void normalize();

private:

    /**
     *
     */
    void init()
        {
        parsebuf  = NULL;
        parselen  = 0;
        scheme    = SCHEME_NONE;
        schemeStr = "";
        port      = 0;
        authority = "";
        path      = "";
        absolute  = false;
        opaque    = false;
        query     = "";
        fragment  = "";
        }


    /**
     *
     */
    void assign(const URI &other)
        {
        scheme    = other.scheme;
        schemeStr = other.schemeStr;
        authority = other.authority;
        port      = other.port;
        path      = other.path;
        absolute  = other.absolute;
        opaque    = other.opaque;
        query     = other.query;
        fragment  = other.fragment;
        }

    int scheme;

    String schemeStr;

    String authority;

    bool portSpecified;

    int port;

    String path;

    bool absolute;

    bool opaque;

    String query;

    String fragment;

    void error(const char *fmt, ...);

    void trace(const char *fmt, ...);


    int peek(int p);

    int match(int p, char *key);

    int parseScheme(int p);

    int parseHierarchicalPart(int p0);

    int parseQuery(int p0);

    int parseFragment(int p0);

    int parse(int p);

    char *parsebuf;

    int parselen;

};



typedef struct
{
    int  ival;
    char *sval;
    int  port;
} LookupEntry;

LookupEntry schemes[] =
{
    { URI::SCHEME_DATA,   "data:",    0 },
    { URI::SCHEME_HTTP,   "http:",   80 },
    { URI::SCHEME_HTTPS,  "https:", 443 },
    { URI::SCHEME_FTP,    "ftp",     12 },
    { URI::SCHEME_FILE,   "file:",    0 },
    { URI::SCHEME_LDAP,   "ldap:",  123 },
    { URI::SCHEME_MAILTO, "mailto:", 25 },
    { URI::SCHEME_NEWS,   "news:",  117 },
    { URI::SCHEME_TELNET, "telnet:", 23 },
    { 0,                  NULL,       0 }
};


String URI::toString() const
{
    String str = schemeStr;
    if (authority.size() > 0)
        {
        str.append("//");
        str.append(authority);
        }
    str.append(path);
    if (query.size() > 0)
        {
        str.append("?");
        str.append(query);
        }
    if (fragment.size() > 0)
        {
        str.append("#");
        str.append(fragment);
        }
    return str;
}


int URI::getScheme() const
{
    return scheme;
}

String URI::getSchemeStr() const
{
    return schemeStr;
}


String URI::getAuthority() const
{
    String ret = authority;
    if (portSpecified && port>=0)
        {
        char buf[7];
        snprintf(buf, 6, ":%6d", port);
        ret.append(buf);
        }
    return ret;
}

String URI::getHost() const
{
    return authority;
}

int URI::getPort() const
{
    return port;
}


String URI::getPath() const
{
    return path;
}

String URI::getNativePath() const
{
    String npath;
#ifdef __WIN32__
    unsigned int firstChar = 0;
    if (path.size() >= 3)
        {
        if (path[0] == '/' &&
            isLetter(path[1]) &&
            path[2] == ':')
            firstChar++;
         }
    for (unsigned int i=firstChar ; i<path.size() ; i++)
        {
        XMLCh ch = (XMLCh) path[i];
        if (ch == '/')
            npath.push_back((XMLCh)'\\');
        else
            npath.push_back(ch);
        }
#else
    npath = path;
#endif
    return npath;
}


bool URI::isAbsolute() const
{
    return absolute;
}

bool URI::isOpaque() const
{
    return opaque;
}


String URI::getQuery() const
{
    return query;
}


String URI::getFragment() const
{
    return fragment;
}


URI URI::resolve(const URI &other) const
{
    //### According to w3c, this is handled in 3 cases

    //## 1
    if (opaque || other.isAbsolute())
        return other;

    //## 2
    if (other.fragment.size()  >  0 &&
        other.path.size()      == 0 &&
        other.scheme           == SCHEME_NONE &&
        other.authority.size() == 0 &&
        other.query.size()     == 0 )
        {
        URI fragUri = *this;
        fragUri.fragment = other.fragment;
        return fragUri;
        }

    //## 3 http://www.ietf.org/rfc/rfc2396.txt, section 5.2
    URI newUri;
    //# 3.1
    newUri.scheme    = scheme;
    newUri.schemeStr = schemeStr;
    newUri.query     = other.query;
    newUri.fragment  = other.fragment;
    if (other.authority.size() > 0)
        {
        //# 3.2
        if (absolute || other.absolute)
            newUri.absolute = true;
        newUri.authority = other.authority;
        newUri.port      = other.port;//part of authority
        newUri.path      = other.path;
        }
    else
        {
        //# 3.3
        if (other.absolute)
            {
            newUri.absolute = true;
            newUri.path     = other.path;
            }
        else
            {
            unsigned int pos = path.find_last_of('/');
            if (pos != path.npos)
                {
                String tpath = path.substr(0, pos+1);
                tpath.append(other.path);
                newUri.path = tpath;
                }
            else
                newUri.path = other.path;
            }
        }

    newUri.normalize();
    return newUri;
}


/**
 *  This follows the Java URI algorithm:
 *   1. All "." segments are removed.
 *   2. If a ".." segment is preceded by a non-".." segment
 *          then both of these segments are removed. This step
 *          is repeated until it is no longer applicable.
 *   3. If the path is relative, and if its first segment
 *          contains a colon character (':'), then a "." segment
 *          is prepended. This prevents a relative URI with a path
 *          such as "a:b/c/d" from later being re-parsed as an
 *          opaque URI with a scheme of "a" and a scheme-specific
 *          part of "b/c/d". (Deviation from RFC 2396)
 */
void URI::normalize()
{
    std::vector<String> segments;

    //## Collect segments
    if (path.size()<2)
        return;
    bool abs = false;
    unsigned int pos=0;
    if (path[0]=='/')
        {
        abs = true;
        pos++;
        }
    while (pos < path.size())
        {
        unsigned int pos2 = path.find('/', pos);
        if (pos2==path.npos)
            {
            String seg = path.substr(pos);
            //printf("last segment:%s\n", seg.c_str());
            segments.push_back(seg);
            break;
            }
        if (pos2>pos)
            {
            String seg = path.substr(pos, pos2-pos);
            //printf("segment:%s\n", seg.c_str());
            segments.push_back(seg);
            }
        pos = pos2;
        pos++;
        }

    //## Clean up (normalize) segments
    bool edited = false;
    std::vector<String>::iterator iter;
    for (iter=segments.begin() ; iter!=segments.end() ; )
        {
        String s = *iter;
        if (s == ".")
            {
            iter = segments.erase(iter);
            edited = true;
            }
        else if (s == ".." &&
                 iter != segments.begin() &&
                 *(iter-1) != "..")
            {
            iter--; //back up, then erase two entries
            iter = segments.erase(iter);
            iter = segments.erase(iter);
            edited = true;
            }
        else
            iter++;
        }

    //## Rebuild path, if necessary
    if (edited)
        {
        path.clear();
        if (abs)
            {
            path.append("/");
            }
        std::vector<String>::iterator iter;
        for (iter=segments.begin() ; iter!=segments.end() ; iter++)
            {
            if (iter != segments.begin())
                path.append("/");
            path.append(*iter);
            }
        }

}



//#########################################################################
//# M E S S A G E S
//#########################################################################

void URI::error(const char *fmt, ...)
{
    va_list args;
    fprintf(stderr, "URI error: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

void URI::trace(const char *fmt, ...)
{
    va_list args;
    fprintf(stdout, "URI: ");
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fprintf(stdout, "\n");
}



//#########################################################################
//# P A R S I N G
//#########################################################################



int URI::peek(int p)
{
    if (p<0 || p>=parselen)
        return -1;
    return parsebuf[p];
}



int URI::match(int p0, char *key)
{
    int p = p0;
    while (p < parselen)
        {
        if (*key == '\0')
            return p;
        else if (*key != parsebuf[p])
            break;
        p++; key++;
        }
    return p0;
}

//#########################################################################
//#  Parsing is performed according to:
//#  http://www.gbiv.com/protocols/uri/rfc/rfc3986.html#components
//#########################################################################

int URI::parseScheme(int p0)
{
    int p = p0;
    for (LookupEntry *entry = schemes; entry->sval ; entry++)
        {
        int p2 = match(p, entry->sval);
        if (p2 > p)
            {
            schemeStr = entry->sval;
            scheme    = entry->ival;
            port      = entry->port;
            p = p2;
            return p;
            }
        }

    return p;
}


int URI::parseHierarchicalPart(int p0)
{
    int p = p0;
    int ch;

    //# Authority field (host and port, for example)
    int p2 = match(p, "//");
    if (p2 > p)
        {
        p = p2;
        portSpecified = false;
        String portStr;
        while (p < parselen)
            {
            ch = peek(p);
            if (ch == '/')
                break;
            else if (ch == ':')
                portSpecified = true;
            else if (portSpecified)
                portStr.push_back((XMLCh)ch);
            else
                authority.push_back((XMLCh)ch);
            p++;
            }
        if (portStr.size() > 0)
            {
            char *pstr = (char *)portStr.c_str();
            char *endStr;
            long val = strtol(pstr, &endStr, 10);
            if (endStr > pstr) //successful parse?
                port = val;
            }
        }

    //# Are we absolute?
    ch = peek(p);
    if (isLetter(ch) && peek(p+1)==':')
        {
        absolute = true;
        path.push_back((XMLCh)'/');
        }
    else if (ch == '/')
        {
        absolute = true;
        if (p>p0) //in other words, if '/' is not the first char
            opaque = true;
        path.push_back((XMLCh)ch);
        p++;
        }

    while (p < parselen)
        {
        ch = peek(p);
        if (ch == '?' || ch == '#')
            break;
        path.push_back((XMLCh)ch);
        p++;
        }

    return p;
}

int URI::parseQuery(int p0)
{
    int p = p0;
    int ch = peek(p);
    if (ch != '?')
        return p0;

    p++;
    while (p < parselen)
        {
        ch = peek(p);
        if (ch == '#')
            break;
        query.push_back((XMLCh)ch);
        p++;
        }


    return p;
}

int URI::parseFragment(int p0)
{

    int p = p0;
    int ch = peek(p);
    if (ch != '#')
        return p0;

    p++;
    while (p < parselen)
        {
        ch = peek(p);
        if (ch == '?')
            break;
        fragment.push_back((XMLCh)ch);
        p++;
        }


    return p;
}


int URI::parse(int p0)
{

    int p = p0;

    int p2 = parseScheme(p);
    if (p2 < 0)
        {
        error("Scheme");
        return -1;
        }
    p = p2;


    p2 = parseHierarchicalPart(p);
    if (p2 < 0)
        {
        error("Hierarchical part");
        return -1;
        }
    p = p2;

    p2 = parseQuery(p);
    if (p2 < 0)
        {
        error("Query");
        return -1;
        }
    p = p2;


    p2 = parseFragment(p);
    if (p2 < 0)
        {
        error("Fragment");
        return -1;
        }
    p = p2;

    return p;

}



bool URI::parse(const String &str)
{
    init();
    
    parselen = str.size();

    String tmp;
    for (unsigned int i=0 ; i<str.size() ; i++)
        {
        XMLCh ch = (XMLCh) str[i];
        if (ch == '\\')
            tmp.push_back((XMLCh)'/');
        else
            tmp.push_back(ch);
        }
    parsebuf = (char *) tmp.c_str();


    int p = parse(0);
    normalize();

    if (p < 0)
        {
        error("Syntax error");
        return false;
        }

    //printf("uri:%s\n", toString().c_str());
    //printf("path:%s\n", path.c_str());

    return true;

}








//########################################################################
//########################################################################
//##  M A K E
//########################################################################
//########################################################################

//########################################################################
//# F I L E S E T
//########################################################################
/**
 * This is the descriptor for a <fileset> item
 */
class FileSet
{
public:

    /**
     *
     */
    FileSet()
        {}

    /**
     *
     */
    FileSet(const FileSet &other)
        { assign(other); }

    /**
     *
     */
    FileSet &operator=(const FileSet &other)
        { assign(other); return *this; }

    /**
     *
     */
    virtual ~FileSet()
        {}

    /**
     *
     */
    String getDirectory()
        { return directory; }
        
    /**
     *
     */
    void setDirectory(const String &val)
        { directory = val; }

    /**
     *
     */
    void setFiles(const std::vector<String> &val)
        { files = val; }

    /**
     *
     */
    std::vector<String> getFiles()
        { return files; }
        
    /**
     *
     */
    void setIncludes(const std::vector<String> &val)
        { includes = val; }

    /**
     *
     */
    std::vector<String> getIncludes()
        { return includes; }
        
    /**
     *
     */
    void setExcludes(const std::vector<String> &val)
        { excludes = val; }

    /**
     *
     */
    std::vector<String> getExcludes()
        { return excludes; }
        
    /**
     *
     */
    unsigned int size()
        { return files.size(); }
        
    /**
     *
     */
    String operator[](int index)
        { return files[index]; }
        
    /**
     *
     */
    void clear()
        {
        directory = "";
        files.clear();
        includes.clear();
        excludes.clear();
        }
        

private:

    void assign(const FileSet &other)
        {
        directory = other.directory;
        files     = other.files;
        includes  = other.includes;
        excludes  = other.excludes;
        }

    String directory;
    std::vector<String> files;
    std::vector<String> includes;
    std::vector<String> excludes;
};




//########################################################################
//# M A K E    B A S E
//########################################################################
/**
 * Base class for all classes in this file
 */
class MakeBase
{
public:
    MakeBase()
        {}
    virtual ~MakeBase()
        {}

    /**
     *     Return the URI of the file associated with this object 
     */     
    URI getURI()
        { return uri; }

    /**
     * Set the uri to the given string
     */
    void setURI(const String &uristr)
        { uri.parse(uristr); }

    /**
     *  Resolve another path relative to this one
     */
    String resolve(const String &otherPath);

    /**
     *  Get an element attribute, performing substitutions if necessary
     */
    bool getAttribute(Element *elem, const String &name, String &result);

    /**
     * Get an element value, performing substitutions if necessary
     */
    bool getValue(Element *elem, String &result);

protected:

    /**
     *    The path to the file associated with this object
     */     
    URI uri;


    /**
     *  Print a printf()-like formatted error message
     */
    void error(char *fmt, ...);

    /**
     *  Print a printf()-like formatted trace message
     */
    void status(char *fmt, ...);

    /**
     *  Print a printf()-like formatted trace message
     */
    void trace(char *fmt, ...);

    /**
     *  Check if a given string matches a given regex pattern
     */
    bool regexMatch(const String &str, const String &pattern);

    /**
     *
     */
    String getSuffix(const String &fname);

    /**
     * Break up a string into substrings delimited the characters
     * in delimiters.  Null-length substrings are ignored
     */  
    std::vector<String> tokenize(const String &val,
                          const String &delimiters);

    /**
     *  replace runs of whitespace with a space
     */
    String strip(const String &s);

    /**
     *  remove leading whitespace from each line
     */
    String leftJustify(const String &s);

    /**
     *  remove leading and trailing whitespace from string
     */
    String trim(const String &s);

    /**
     * Return the native format of the canonical
     * path which we store
     */
    String getNativePath(const String &path);

    /**
     * Execute a shell command.  Outbuf is a ref to a string
     * to catch the result.     
     */         
    bool executeCommand(const String &call,
                        const String &inbuf,
                        String &outbuf,
                        String &errbuf);
    /**
     * List all directories in a given base and starting directory
     * It is usually called like:
     *        bool ret = listDirectories("src", "", result);    
     */         
    bool listDirectories(const String &baseName,
                         const String &dirname,
                         std::vector<String> &res);

    /**
     * Find all files in the named directory 
     */         
    bool listFiles(const String &baseName,
                   const String &dirname,
                   std::vector<String> &result);

    /**
     * Perform a listing for a fileset 
     */         
    bool listFiles(MakeBase &propRef, FileSet &fileSet);

    /**
     * Parse a <patternset>
     */  
    bool parsePatternSet(Element *elem,
                       MakeBase &propRef,
                       std::vector<String> &includes,
                       std::vector<String> &excludes);

    /**
     * Parse a <fileset> entry, and determine which files
     * should be included
     */  
    bool parseFileSet(Element *elem,
                    MakeBase &propRef,
                    FileSet &fileSet);

    /**
     * Return this object's property list
     */
    virtual std::map<String, String> &getProperties()
        { return properties; }

    /**
     * Return a named property if found, else a null string
     */
    virtual String getProperty(const String &name)
        {
        String val;
        std::map<String, String>::iterator iter;
        iter = properties.find(name);
        if (iter != properties.end())
            val = iter->second;
        return val;
        }


    std::map<String, String> properties;

    /**
     * Turn 'true' and 'false' into boolean values
     */             
    bool getBool(const String &str, bool &val);

    /**
     * Create a directory, making intermediate dirs
     * if necessary
     */                  
    bool createDirectory(const String &dirname);

    /**
     * Delete a directory and its children if desired
     */
    bool removeDirectory(const String &dirName);

    /**
     * Copy a file from one name to another. Perform only if needed
     */ 
    bool copyFile(const String &srcFile, const String &destFile);

    /**
     * Tests if the file exists and is a regular file
     */ 
    bool isRegularFile(const String &fileName);

    /**
     * Tests if the file exists and is a directory
     */ 
    bool isDirectory(const String &fileName);

    /**
     * Tests is the modification date of fileA is newer than fileB
     */ 
    bool isNewerThan(const String &fileA, const String &fileB);

private:

    /**
     * replace variable refs like ${a} with their values
     */         
    bool getSubstitutions(const String &s, String &result);



};




/**
 *  Print a printf()-like formatted error message
 */
void MakeBase::error(char *fmt, ...)
{
    va_list args;
    va_start(args,fmt);
    fprintf(stderr, "Make error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args) ;
}



/**
 *  Print a printf()-like formatted trace message
 */
void MakeBase::status(char *fmt, ...)
{
    va_list args;
    va_start(args,fmt);
    //fprintf(stdout, " ");
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    va_end(args) ;
}



/**
 *  Resolve another path relative to this one
 */
String MakeBase::resolve(const String &otherPath)
{
    URI otherURI(otherPath);
    URI fullURI = uri.resolve(otherURI);
    String ret = fullURI.toString();
    return ret;
}


/**
 *  Print a printf()-like formatted trace message
 */
void MakeBase::trace(char *fmt, ...)
{
    va_list args;
    va_start(args,fmt);
    fprintf(stdout, "Make: ");
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    va_end(args) ;
}



/**
 *  Check if a given string matches a given regex pattern
 */
bool MakeBase::regexMatch(const String &str, const String &pattern)
{
    const TRexChar *terror = NULL;
    const TRexChar *cpat = pattern.c_str();
    TRex *expr = trex_compile(cpat, &terror);
    if (!expr)
        {
        if (!terror)
            terror = "undefined";
        error("compilation error [%s]!\n", terror);
        return false;
        } 

    bool ret = true;

    const TRexChar *cstr = str.c_str();
    if (trex_match(expr, cstr))
        {
        ret = true;
        }
    else
        {
        ret = false;
        }

    trex_free(expr);

    return ret;
}

/**
 *  Return the suffix, if any, of a file name
 */
String MakeBase::getSuffix(const String &fname)
{
    if (fname.size() < 2)
        return "";
    unsigned int pos = fname.find_last_of('.');
    if (pos == fname.npos)
        return "";
    pos++;
    String res = fname.substr(pos, fname.size()-pos);
    //trace("suffix:%s", res.c_str()); 
    return res;
}



/**
 * Break up a string into substrings delimited the characters
 * in delimiters.  Null-length substrings are ignored
 */  
std::vector<String> MakeBase::tokenize(const String &str,
                                const String &delimiters)
{

    std::vector<String> res;
    char *del = (char *)delimiters.c_str();
    String dmp;
    for (unsigned int i=0 ; i<str.size() ; i++)
        {
        char ch = str[i];
        char *p = (char *)0;
        for (p=del ; *p ; p++)
            if (*p == ch)
                break;
        if (*p)
            {
            if (dmp.size() > 0)
                {
                res.push_back(dmp);
                dmp.clear();
                }
            }
        else
            {
            dmp.push_back(ch);
            }
        }
    //Add tail
    if (dmp.size() > 0)
        {
        res.push_back(dmp);
        dmp.clear();
        }

    return res;
}



/**
 *  replace runs of whitespace with a single space
 */
String MakeBase::strip(const String &s)
{
    int len = s.size();
    String stripped;
    for (int i = 0 ; i<len ; i++)
        {
        char ch = s[i];
        if (isspace(ch))
            {
            stripped.push_back(' ');
            for ( ; i<len ; i++)
                {
                ch = s[i];
                if (!isspace(ch))
                    {
                    stripped.push_back(ch);
                    break;
                    }
                }
            }
        else
            {
            stripped.push_back(ch);
            }
        }
    return stripped;
}

/**
 *  remove leading whitespace from each line
 */
String MakeBase::leftJustify(const String &s)
{
    String out;
    int len = s.size();
    for (int i = 0 ; i<len ; )
        {
        char ch;
        //Skip to first visible character
        while (i<len)
            {
            ch = s[i];
            if (ch == '\n' || ch == '\r'
              || !isspace(ch))
                  break;
            i++;
            }
        //Copy the rest of the line
        while (i<len)
            {
            ch = s[i];
            if (ch == '\n' || ch == '\r')
                {
                if (ch != '\r')
                    out.push_back('\n');
                i++;
                break;
                }
            else
                {
                out.push_back(ch);
                }
            i++;
            }
        }
    return out;
}


/**
 *  Removes whitespace from beginning and end of a string
 */
String MakeBase::trim(const String &s)
{
    if (s.size() < 1)
        return s;
    
    //Find first non-ws char
    unsigned int begin = 0;
    for ( ; begin < s.size() ; begin++)
        {
        if (!isspace(s[begin]))
            break;
        }

    //Find first non-ws char, going in reverse
    unsigned int end = s.size() - 1;
    for ( ; end > begin ; end--)
        {
        if (!isspace(s[end]))
            break;
        }
    //trace("begin:%d  end:%d", begin, end);

    String res = s.substr(begin, end-begin+1);
    return res;
}

/**
 * Return the native format of the canonical
 * path which we store
 */
String MakeBase::getNativePath(const String &path)
{
#ifdef __WIN32__
    String npath;
    unsigned int firstChar = 0;
    if (path.size() >= 3)
        {
        if (path[0] == '/' &&
            isalpha(path[1]) &&
            path[2] == ':')
            firstChar++;
        }
    for (unsigned int i=firstChar ; i<path.size() ; i++)
        {
        char ch = path[i];
        if (ch == '/')
            npath.push_back('\\');
        else
            npath.push_back(ch);
        }
    return npath;
#else
    return path;
#endif
}


#ifdef __WIN32__
#include <tchar.h>

static String win32LastError()
{

    DWORD dw = GetLastError(); 

    LPVOID str;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        0,
        (LPTSTR) &str,
        0, NULL );
    LPTSTR p = _tcschr((const char *)str, _T('\r'));
    if(p != NULL)
        { // lose CRLF
        *p = _T('\0');
        }
    String ret = (char *)str;
    LocalFree(str);

    return ret;
}
#endif



/**
 * Execute a system call, using pipes to send data to the
 * program's stdin,  and reading stdout and stderr.
 */
bool MakeBase::executeCommand(const String &command,
                              const String &inbuf,
                              String &outbuf,
                              String &errbuf)
{

    status("============ cmd ============\n%s\n=============================",
                command.c_str());

    outbuf.clear();
    errbuf.clear();
    
#ifdef __WIN32__

    /*
    I really hate having win32 code in this program, but the
    read buffer in command.com and cmd.exe are just too small
    for the large commands we need for compiling and linking.
    */

    bool ret = true;

    //# Allocate a separate buffer for safety
    char *paramBuf = new char[command.size() + 1];
    if (!paramBuf)
       {
       error("executeCommand cannot allocate command buffer");
       return false;
       }
    strcpy(paramBuf, (char *)command.c_str());

    //# Create pipes
    SECURITY_ATTRIBUTES saAttr; 
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 
    HANDLE stdinRead,  stdinWrite;
    HANDLE stdoutRead, stdoutWrite;
    HANDLE stderrRead, stderrWrite;
    if (!CreatePipe(&stdinRead, &stdinWrite, &saAttr, 0))
        {
        error("executeProgram: could not create pipe");
        delete[] paramBuf;
        return false;
        } 
    SetHandleInformation(stdinWrite, HANDLE_FLAG_INHERIT, 0);
    if (!CreatePipe(&stdoutRead, &stdoutWrite, &saAttr, 0))
        {
        error("executeProgram: could not create pipe");
        delete[] paramBuf;
        return false;
        } 
    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);
    if (!CreatePipe(&stderrRead, &stderrWrite, &saAttr, 0))
        {
        error("executeProgram: could not create pipe");
        delete[] paramBuf;
        return false;
        } 
    SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0);

    // Create the process
    STARTUPINFO siStartupInfo;
    PROCESS_INFORMATION piProcessInfo;
    memset(&siStartupInfo, 0, sizeof(siStartupInfo));
    memset(&piProcessInfo, 0, sizeof(piProcessInfo));
    siStartupInfo.cb = sizeof(siStartupInfo);
    siStartupInfo.hStdError   =  stderrWrite;
    siStartupInfo.hStdOutput  =  stdoutWrite;
    siStartupInfo.hStdInput   =  stdinRead;
    siStartupInfo.dwFlags    |=  STARTF_USESTDHANDLES;
   
    if (!CreateProcess(NULL, paramBuf, NULL, NULL, true,
                0, NULL, NULL, &siStartupInfo,
                &piProcessInfo))
        {
        error("executeCommand : could not create process : %s",
                    win32LastError().c_str());
        ret = false;
        }

    DWORD bytesWritten;
    if (inbuf.size()>0 &&
        !WriteFile(stdinWrite, inbuf.c_str(), inbuf.size(), 
               &bytesWritten, NULL))
        {
        error("executeCommand: could not write to pipe");
        return false;
        }    
    if (!CloseHandle(stdinWrite))
        {          
        error("executeCommand: could not close write pipe");
        return false;
        }
    if (!CloseHandle(stdoutWrite))
        {
        error("executeCommand: could not close read pipe");
        return false;
        }
    if (!CloseHandle(stderrWrite))
        {
        error("executeCommand: could not close read pipe");
        return false;
        }
    while (true)
        {
        //trace("## stderr");
        DWORD avail;
        if (!PeekNamedPipe(stderrRead, NULL, 0, NULL, &avail, NULL))
            break;
        if (avail > 0)
            {
            DWORD bytesRead = 0;
            char readBuf[1025];
            if (avail>1024) avail = 1024;
            if (!ReadFile(stderrRead, readBuf, avail, &bytesRead, NULL)
                || bytesRead == 0)
                {
                break;
                }
            for (unsigned int i=0 ; i<bytesRead ; i++)
                errbuf.push_back(readBuf[i]);
            }
        }
    while (true)
        {
        //trace("## stdout");
        DWORD avail;
        if (!PeekNamedPipe(stdoutRead, NULL, 0, NULL, &avail, NULL))
            break;
        if (avail > 0)
            {
            DWORD bytesRead = 0;
            char readBuf[1025];
            if (avail>1024) avail = 1024;
            if (!ReadFile(stdoutRead, readBuf, avail, &bytesRead, NULL)
                || bytesRead==0)
                {
                break;
                }
            for (unsigned int i=0 ; i<bytesRead ; i++)
                outbuf.push_back(readBuf[i]);
            }
        DWORD exitCode;
        GetExitCodeProcess(piProcessInfo.hProcess, &exitCode);
        if (exitCode != STILL_ACTIVE)
            break;
        Sleep(100);
        }    
    //trace("outbuf:%s", outbuf.c_str());
    if (!CloseHandle(stdoutRead))
        {
        error("executeCommand: could not close read pipe");
        return false;
        }
    if (!CloseHandle(stderrRead))
        {
        error("executeCommand: could not close read pipe");
        return false;
        }

    DWORD exitCode;
    GetExitCodeProcess(piProcessInfo.hProcess, &exitCode);
    //trace("exit code:%d", exitCode);
    if (exitCode != 0)
        {
        ret = false;
        }
    
    // Clean up
    CloseHandle(piProcessInfo.hProcess);
    CloseHandle(piProcessInfo.hThread);


    return ret;

#else //do it unix-style

    String s;
    FILE *f = popen(command.c_str(), "r");
    int errnum = 0;
    if (f)
        {
        while (true)
            {
            int ch = fgetc(f);
            if (ch < 0)
                break;
            s.push_back((char)ch);
            }
        errnum = pclose(f);
        }
    outbuf = s;
    if (errnum != 0)
        {
        error("exec of command '%s' failed : %s",
             command.c_str(), strerror(errno));
        return false;
        }
    else
        return true;

#endif
} 




bool MakeBase::listDirectories(const String &baseName,
                              const String &dirName,
                              std::vector<String> &res)
{
    res.push_back(dirName);
    String fullPath = baseName;
    if (dirName.size()>0)
        {
        fullPath.append("/");
        fullPath.append(dirName);
        }
    DIR *dir = opendir(fullPath.c_str());
    while (true)
        {
        struct dirent *de = readdir(dir);
        if (!de)
            break;

        //Get the directory member name
        String s = de->d_name;
        if (s.size() == 0 || s[0] == '.')
            continue;
        String childName = dirName;
        childName.append("/");
        childName.append(s);

        String fullChildPath = baseName;
        fullChildPath.append("/");
        fullChildPath.append(childName);
        struct stat finfo;
        String childNative = getNativePath(fullChildPath);
        if (stat(childNative.c_str(), &finfo)<0)
            {
            error("cannot stat file:%s", childNative.c_str());
            }
        else if (S_ISDIR(finfo.st_mode))
            {
            //trace("directory: %s", childName.c_str());
            if (!listDirectories(baseName, childName, res))
                return false;
            }
        }
    closedir(dir);

    return true;
}


bool MakeBase::listFiles(const String &baseDir,
                         const String &dirName,
                         std::vector<String> &res)
{
    String fullDir = baseDir;
    if (dirName.size()>0)
        {
        fullDir.append("/");
        fullDir.append(dirName);
        }
    String dirNative = getNativePath(fullDir);

    std::vector<String> subdirs;
    DIR *dir = opendir(dirNative.c_str());
    if (!dir)
        {
        error("Could not open directory %s : %s",
              dirNative.c_str(), strerror(errno));
        return false;
        }
    while (true)
        {
        struct dirent *de = readdir(dir);
        if (!de)
            break;

        //Get the directory member name
        String s = de->d_name;
        if (s.size() == 0 || s[0] == '.')
            continue;
        String childName;
        if (dirName.size()>0)
            {
            childName.append(dirName);
            childName.append("/");
            }
        childName.append(s);
        String fullChild = baseDir;
        fullChild.append("/");
        fullChild.append(childName);
        
        if (isDirectory(fullChild))
            {
            //trace("directory: %s", childName.c_str());
            if (!listFiles(baseDir, childName, res))
                return false;
            continue;
            }
        else if (!isRegularFile(fullChild))
            {
            error("unknown file:%s", childName.c_str());
            return false;
            }

       //all done!
        res.push_back(childName);

        }
    closedir(dir);

    return true;
}


bool MakeBase::listFiles(MakeBase &propRef, FileSet &fileSet)
{
    String baseDir = propRef.resolve(fileSet.getDirectory());
    std::vector<String> fileList;
    if (!listFiles(baseDir, "", fileList))
        return false;

    std::vector<String> includes = fileSet.getIncludes();
    std::vector<String> excludes = fileSet.getExcludes();

    std::vector<String> incs;
    std::vector<String>::iterator iter;

    std::sort(fileList.begin(), fileList.end());

    //If there are <includes>, then add files to the output
    //in the order of the include list
    if (includes.size()==0)
        incs = fileList;
    else
        {
        for (iter = includes.begin() ; iter != includes.end() ; iter++)
            {
            String pattern = *iter;
            std::vector<String>::iterator siter;
            for (siter = fileList.begin() ; siter != fileList.end() ; siter++)
                {
                String s = *siter;
                if (regexMatch(s, pattern))
                    {
                    //trace("INCLUDED:%s", s.c_str());
                    incs.push_back(s);
                    }
                }
            }
        }

    //Now trim off the <excludes>
    std::vector<String> res;
    for (iter = incs.begin() ; iter != incs.end() ; iter++)
        {
        String s = *iter;
        bool skipme = false;
        std::vector<String>::iterator siter;
        for (siter = excludes.begin() ; siter != excludes.end() ; siter++)
            {
            String pattern = *siter;
            if (regexMatch(s, pattern))
                {
                //trace("EXCLUDED:%s", s.c_str());
                skipme = true;
                break;
                }
            }
        if (!skipme)
            res.push_back(s);
        }
        
    fileSet.setFiles(res);

    return true;
}





bool MakeBase::getSubstitutions(const String &str, String &result)
{
    String s = trim(str);
    int len = (int)s.size();
    String val;
    for (int i=0 ; i<len ; i++)
        {
        char ch = s[i];
        if (ch == '$' && s[i+1] == '{')
            {
            String varname;
            int j = i+2;
            for ( ; j<len ; j++)
                {
                ch = s[j];
                if (ch == '$' && s[j+1] == '{')
                    {
                    error("attribute %s cannot have nested variable references",
                           s.c_str());
                    return false;
                    }
                else if (ch == '}')
                    {
                    std::map<String, String>::iterator iter;
                    iter = properties.find(trim(varname));
                    if (iter != properties.end())
                        {
                        val.append(iter->second);
                        }
                    else
                        {
                        error("property ${%s} not found", varname.c_str());
                        return false;
                        }
                    break;
                    }
                else
                    {
                    varname.push_back(ch);
                    }
                }
            i = j;
            }
        else
            {
            val.push_back(ch);
            }
        }
    result = val;
    return true;
}


bool MakeBase::getAttribute(Element *elem, const String &name,
                                    String &result)
{
    String s = elem->getAttribute(name);
    return getSubstitutions(s, result);
}


bool MakeBase::getValue(Element *elem, String &result)
{
    String s = elem->getValue();
    //Replace all runs of whitespace with a single space
    return getSubstitutions(s, result);
}


/**
 * Turn 'true' and 'false' into boolean values
 */             
bool MakeBase::getBool(const String &str, bool &val)
{
    if (str == "true")
        val = true;
    else if (str == "false")
        val = false;
    else
        {
        error("expected 'true' or 'false'.  found '%s'", str.c_str());
        return false;
        }
    return true;
}




/**
 * Parse a <patternset> entry
 */  
bool MakeBase::parsePatternSet(Element *elem,
                          MakeBase &propRef,
                          std::vector<String> &includes,
                          std::vector<String> &excludes
                          )
{
    std::vector<Element *> children  = elem->getChildren();
    for (unsigned int i=0 ; i<children.size() ; i++)
        {
        Element *child = children[i];
        String tagName = child->getName();
        if (tagName == "exclude")
            {
            String fname;
            if (!propRef.getAttribute(child, "name", fname))
                return false;
            //trace("EXCLUDE: %s", fname.c_str());
            excludes.push_back(fname);
            }
        else if (tagName == "include")
            {
            String fname;
            if (!propRef.getAttribute(child, "name", fname))
                return false;
            //trace("INCLUDE: %s", fname.c_str());
            includes.push_back(fname);
            }
        }

    return true;
}




/**
 * Parse a <fileset> entry, and determine which files
 * should be included
 */  
bool MakeBase::parseFileSet(Element *elem,
                          MakeBase &propRef,
                          FileSet &fileSet)
{
    String name = elem->getName();
    if (name != "fileset")
        {
        error("expected <fileset>");
        return false;
        }


    std::vector<String> includes;
    std::vector<String> excludes;

    //A fileset has one implied patternset
    if (!parsePatternSet(elem, propRef, includes, excludes))
        {
        return false;
        }
    //Look for child tags, including more patternsets
    std::vector<Element *> children  = elem->getChildren();
    for (unsigned int i=0 ; i<children.size() ; i++)
        {
        Element *child = children[i];
        String tagName = child->getName();
        if (tagName == "patternset")
            {
            if (!parsePatternSet(child, propRef, includes, excludes))
                {
                return false;
                }
            }
        }

    String dir;
    //Now do the stuff
    //Get the base directory for reading file names
    if (!propRef.getAttribute(elem, "dir", dir))
        return false;

    fileSet.setDirectory(dir);
    fileSet.setIncludes(includes);
    fileSet.setExcludes(excludes);
    
    /*
    std::vector<String> fileList;
    if (dir.size() > 0)
        {
        String baseDir = propRef.resolve(dir);
        if (!listFiles(baseDir, "", includes, excludes, fileList))
            return false;
        }
    std::sort(fileList.begin(), fileList.end());
    result = fileList;
    */

    
    /*
    for (unsigned int i=0 ; i<result.size() ; i++)
        {
        trace("RES:%s", result[i].c_str());
        }
    */

    
    return true;
}



/**
 * Create a directory, making intermediate dirs
 * if necessary
 */                  
bool MakeBase::createDirectory(const String &dirname)
{
    //trace("## createDirectory: %s", dirname.c_str());
    //## first check if it exists
    struct stat finfo;
    String nativeDir = getNativePath(dirname);
    char *cnative = (char *) nativeDir.c_str();
#ifdef __WIN32__
    if (strlen(cnative)==2 && cnative[1]==':')
        return true;
#endif
    if (stat(cnative, &finfo)==0)
        {
        if (!S_ISDIR(finfo.st_mode))
            {
            error("mkdir: file %s exists but is not a directory",
                  cnative);
            return false;
            }
        else //exists
            {
            return true;
            }
        }

    //## 2: pull off the last path segment, if any,
    //## to make the dir 'above' this one, if necessary
    unsigned int pos = dirname.find_last_of('/');
    if (pos>0 && pos != dirname.npos)
        {
        String subpath = dirname.substr(0, pos);
        //A letter root (c:) ?
        if (!createDirectory(subpath))
            return false;
        }
        
    //## 3: now make
#ifdef __WIN32__
    if (mkdir(cnative)<0)
#else
    if (mkdir(cnative, S_IRWXU | S_IRWXG | S_IRWXO)<0)
#endif
        {
        error("cannot make directory '%s' : %s",
                 cnative, strerror(errno));
        return false;
        }
        
    return true;
}


/**
 * Remove a directory recursively
 */ 
bool MakeBase::removeDirectory(const String &dirName)
{
    char *dname = (char *)dirName.c_str();

    DIR *dir = opendir(dname);
    if (!dir)
        {
        //# Let this fail nicely.
        return true;
        //error("error opening directory %s : %s", dname, strerror(errno));
        //return false;
        }
    
    while (true)
        {
        struct dirent *de = readdir(dir);
        if (!de)
            break;

        //Get the directory member name
        String s = de->d_name;
        if (s.size() == 0 || s[0] == '.')
            continue;
        String childName;
        if (dirName.size() > 0)
            {
            childName.append(dirName);
            childName.append("/");
            }
        childName.append(s);


        struct stat finfo;
        String childNative = getNativePath(childName);
        char *cnative = (char *)childNative.c_str();
        if (stat(cnative, &finfo)<0)
            {
            error("cannot stat file:%s", cnative);
            }
        else if (S_ISDIR(finfo.st_mode))
            {
            //trace("DEL dir: %s", childName.c_str());
            if (!removeDirectory(childName))
                {
                return false;
                }
            }
        else if (!S_ISREG(finfo.st_mode))
            {
            //trace("not regular: %s", cnative);
            }
        else
            {
            //trace("DEL file: %s", childName.c_str());
            if (remove(cnative)<0)
                {
                error("error deleting %s : %s",
                     cnative, strerror(errno));
                return false;
                }
            }
        }
    closedir(dir);

    //Now delete the directory
    String native = getNativePath(dirName);
    if (rmdir(native.c_str())<0)
        {
        error("could not delete directory %s : %s",
            native.c_str() , strerror(errno));
        return false;
        }

    return true;
    
}


/**
 * Copy a file from one name to another. Perform only if needed
 */ 
bool MakeBase::copyFile(const String &srcFile, const String &destFile)
{
    //# 1 Check up-to-date times
    String srcNative = getNativePath(srcFile);
    struct stat srcinfo;
    if (stat(srcNative.c_str(), &srcinfo)<0)
        {
        error("source file %s for copy does not exist",
                 srcNative.c_str());
        return false;
        }

    String destNative = getNativePath(destFile);
    struct stat destinfo;
    if (stat(destNative.c_str(), &destinfo)==0)
        {
        if (destinfo.st_mtime >= srcinfo.st_mtime)
            return true;
        }
        
    //# 2 prepare a destination directory if necessary
    unsigned int pos = destFile.find_last_of('/');
    if (pos != destFile.npos)
        {
        String subpath = destFile.substr(0, pos);
        if (!createDirectory(subpath))
            return false;
        }

    //# 3 do the data copy
#ifndef __WIN32__

    FILE *srcf = fopen(srcNative.c_str(), "rb");
    if (!srcf)
        {
        error("copyFile cannot open '%s' for reading", srcNative.c_str());
        return false;
        }
    FILE *destf = fopen(destNative.c_str(), "wb");
    if (!destf)
        {
        error("copyFile cannot open %s for writing", srcNative.c_str());
        return false;
        }

    while (!feof(srcf))
        {
        int ch = fgetc(srcf);
        if (ch<0)
            break;
        fputc(ch, destf);
        }

    fclose(destf);
    fclose(srcf);

#else
    
    if (!CopyFile(srcNative.c_str(), destNative.c_str(), false))
        {
        error("copyFile from %s to %s failed",
             srcNative.c_str(), destNative.c_str());
        return false;
        }
        
#endif /* __WIN32__ */


    return true;
}



/**
 * Tests if the file exists and is a regular file
 */ 
bool MakeBase::isRegularFile(const String &fileName)
{
    String native = getNativePath(fileName);
    struct stat finfo;
    
    //Exists?
    if (stat(native.c_str(), &finfo)<0)
        return false;


    //check the file mode
    if (!S_ISREG(finfo.st_mode))
        return false;

    return true;
}

/**
 * Tests if the file exists and is a directory
 */ 
bool MakeBase::isDirectory(const String &fileName)
{
    String native = getNativePath(fileName);
    struct stat finfo;
    
    //Exists?
    if (stat(native.c_str(), &finfo)<0)
        return false;


    //check the file mode
    if (!S_ISDIR(finfo.st_mode))
        return false;

    return true;
}



/**
 * Tests is the modification of fileA is newer than fileB
 */ 
bool MakeBase::isNewerThan(const String &fileA, const String &fileB)
{
    //trace("isNewerThan:'%s' , '%s'", fileA.c_str(), fileB.c_str());
    String nativeA = getNativePath(fileA);
    struct stat infoA;
    //IF source does not exist, NOT newer
    if (stat(nativeA.c_str(), &infoA)<0)
        {
        return false;
        }

    String nativeB = getNativePath(fileB);
    struct stat infoB;
    //IF dest does not exist, YES, newer
    if (stat(nativeB.c_str(), &infoB)<0)
        {
        return true;
        }

    //check the actual times
    if (infoA.st_mtime > infoB.st_mtime)
        {
        return true;
        }

    return false;
}


//########################################################################
//# P K G    C O N F I G
//########################################################################

/**
 *
 */
class PkgConfig : public MakeBase
{

public:

    /**
     *
     */
    PkgConfig()
        { init(); }

    /**
     *
     */
    PkgConfig(const String &namearg)
        { init(); name = namearg; }

    /**
     *
     */
    PkgConfig(const PkgConfig &other)
        { assign(other); }

    /**
     *
     */
    PkgConfig &operator=(const PkgConfig &other)
        { assign(other); return *this; }

    /**
     *
     */
    virtual ~PkgConfig()
        { }

    /**
     *
     */
    virtual String getName()
        { return name; }

    /**
     *
     */
    virtual String getDescription()
        { return description; }

    /**
     *
     */
    virtual String getCflags()
        { return cflags; }

    /**
     *
     */
    virtual String getLibs()
        { return libs; }

    /**
     *
     */
    virtual String getVersion()
        { return version; }

    /**
     *
     */
    virtual int getMajorVersion()
        { return majorVersion; }

    /**
     *
     */
    virtual int getMinorVersion()
        { return minorVersion; }

    /**
     *
     */
    virtual int getMicroVersion()
        { return microVersion; }

    /**
     *
     */
    virtual std::map<String, String> &getAttributes()
        { return attrs; }

    /**
     *
     */
    virtual std::vector<String> &getRequireList()
        { return requireList; }

    virtual bool readFile(const String &fileName);

private:

    void init()
        {
        name         = "";
        description  = "";
        cflags       = "";
        libs         = "";
        requires     = "";
        version      = "";
        majorVersion = 0;
        minorVersion = 0;
        microVersion = 0;
        fileName     = "";
        attrs.clear();
        requireList.clear();
        }

    void assign(const PkgConfig &other)
        {
        name         = other.name;
        description  = other.description;
        cflags       = other.cflags;
        libs         = other.libs;
        requires     = other.requires;
        version      = other.version;
        majorVersion = other.majorVersion;
        minorVersion = other.minorVersion;
        microVersion = other.microVersion;
        fileName     = other.fileName;
        attrs        = other.attrs;
        requireList  = other.requireList;
        }



    int get(int pos);

    int skipwhite(int pos);

    int getword(int pos, String &ret);

    void parseRequires();

    void parseVersion();

    bool parse(const String &buf);

    void dumpAttrs();

    String name;

    String description;

    String cflags;

    String libs;

    String requires;

    String version;

    int majorVersion;

    int minorVersion;

    int microVersion;

    String fileName;

    std::map<String, String> attrs;

    std::vector<String> requireList;

    char *parsebuf;
    int parselen;
};


/**
 * Get a character from the buffer at pos.  If out of range,
 * return -1 for safety
 */
int PkgConfig::get(int pos)
{
    if (pos>parselen)
        return -1;
    return parsebuf[pos];
}



/**
 *  Skip over all whitespace characters beginning at pos.  Return
 *  the position of the first non-whitespace character.
 */
int PkgConfig::skipwhite(int pos)
{
    while (pos < parselen)
        {
        int ch = get(pos);
        if (ch < 0)
            break;
        if (!isspace(ch))
            break;
        pos++;
        }
    return pos;
}


/**
 *  Parse the buffer beginning at pos, for a word.  Fill
 *  'ret' with the result.  Return the position after the
 *  word.
 */
int PkgConfig::getword(int pos, String &ret)
{
    while (pos < parselen)
        {
        int ch = get(pos);
        if (ch < 0)
            break;
        if (!isalnum(ch) && ch != '_' && ch != '-'&& ch != '.')
            break;
        ret.push_back((char)ch);
        pos++;
        }
    return pos;
}

void PkgConfig::parseRequires()
{
    if (requires.size() == 0)
        return;
    parsebuf = (char *)requires.c_str();
    parselen = requires.size();
    int pos = 0;
    while (pos < parselen)
        {
        pos = skipwhite(pos);
        String val;
        int pos2 = getword(pos, val);
        if (pos2 == pos)
            break;
        pos = pos2;
        //trace("val %s", val.c_str());
        requireList.push_back(val);
        }
}

static int getint(const String str)
{
    char *s = (char *)str.c_str();
    char *ends = NULL;
    long val = strtol(s, &ends, 10);
    if (ends == s)
        return 0L;
    else
        return val;
}

void PkgConfig::parseVersion()
{
    if (version.size() == 0)
        return;
    String s1, s2, s3;
    unsigned int pos = 0;
    unsigned int pos2 = version.find('.', pos);
    if (pos2 == version.npos)
        {
        s1 = version;
        }
    else
        {
        s1 = version.substr(pos, pos2-pos);
        pos = pos2;
        pos++;
        if (pos < version.size())
            {
            pos2 = version.find('.', pos);
            if (pos2 == version.npos)
                {
                s2 = version.substr(pos, version.size()-pos);
                }
            else
                {
                s2 = version.substr(pos, pos2-pos);
                pos = pos2;
                pos++;
                if (pos < version.size())
                    s3 = version.substr(pos, pos2-pos);
                }
            }
        }

    majorVersion = getint(s1);
    minorVersion = getint(s2);
    microVersion = getint(s3);
    //trace("version:%d.%d.%d", majorVersion,
    //          minorVersion, microVersion );
}


bool PkgConfig::parse(const String &buf)
{
    init();

    parsebuf = (char *)buf.c_str();
    parselen = buf.size();
    int pos = 0;


    while (pos < parselen)
        {
        String attrName;
        pos = skipwhite(pos);
        int ch = get(pos);
        if (ch == '#')
            {
            //comment.  eat the rest of the line
            while (pos < parselen)
                {
                ch = get(pos);
                if (ch == '\n' || ch < 0)
                    break;
                pos++;
                }
            continue;
            }
        pos = getword(pos, attrName);
        if (attrName.size() == 0)
            continue;
        pos = skipwhite(pos);
        ch = get(pos);
        if (ch != ':' && ch != '=')
            {
            error("expected ':' or '='");
            return false;
            }
        pos++;
        pos = skipwhite(pos);
        String attrVal;
        while (pos < parselen)
            {
            ch = get(pos);
            if (ch == '\n' || ch < 0)
                break;
            else if (ch == '$' && get(pos+1) == '{')
                {
                //#  this is a ${substitution}
                pos += 2;
                String subName;
                while (pos < parselen)
                    {
                    ch = get(pos);
                    if (ch < 0)
                        {
                        error("unterminated substitution");
                        return false;
                        }
                    else if (ch == '}')
                        break;
                    else
                        subName.push_back((char)ch);
                    pos++;
                    }
                //trace("subName:%s", subName.c_str());
                String subVal = attrs[subName];
                //trace("subVal:%s", subVal.c_str());
                attrVal.append(subVal);
                }
            else
                attrVal.push_back((char)ch);
            pos++;
            }

        attrVal = trim(attrVal);
        attrs[attrName] = attrVal;

        if (attrName == "Name")
            name = attrVal;
        else if (attrName == "Description")
            description = attrVal;
        else if (attrName == "Cflags")
            cflags = attrVal;
        else if (attrName == "Libs")
            libs = attrVal;
        else if (attrName == "Requires")
            requires = attrVal;
        else if (attrName == "Version")
            version = attrVal;

        //trace("name:'%s'  value:'%s'",
        //      attrName.c_str(), attrVal.c_str());
        }


    parseRequires();
    parseVersion();

    return true;
}

void PkgConfig::dumpAttrs()
{
    //trace("### PkgConfig attributes for %s", fileName.c_str());
    std::map<String, String>::iterator iter;
    for (iter=attrs.begin() ; iter!=attrs.end() ; iter++)
        {
        trace("   %s = %s", iter->first.c_str(), iter->second.c_str());
        }
}


bool PkgConfig::readFile(const String &fileNameArg)
{
    fileName = fileNameArg;

    FILE *f = fopen(fileName.c_str(), "r");
    if (!f)
        {
        error("cannot open file '%s' for reading", fileName.c_str());
        return false;
        }
    String buf;
    while (true)
        {
        int ch = fgetc(f);
        if (ch < 0)
            break;
        buf.push_back((char)ch);
        }
    fclose(f);

    //trace("####### File:\n%s", buf.c_str());
    if (!parse(buf))
        {
        return false;
        }

    dumpAttrs();

    return true;
}





//########################################################################
//# D E P T O O L
//########################################################################



/**
 *  Class which holds information for each file.
 */
class FileRec
{
public:

    typedef enum
        {
        UNKNOWN,
        CFILE,
        HFILE,
        OFILE
        } FileType;

    /**
     *  Constructor
     */
    FileRec()
        {init(); type = UNKNOWN;}

    /**
     *  Copy constructor
     */
    FileRec(const FileRec &other)
        {init(); assign(other);}
    /**
     *  Constructor
     */
    FileRec(int typeVal)
        {init(); type = typeVal;}
    /**
     *  Assignment operator
     */
    FileRec &operator=(const FileRec &other)
        {init(); assign(other); return *this;}


    /**
     *  Destructor
     */
    ~FileRec()
        {}

    /**
     *  Directory part of the file name
     */
    String path;

    /**
     *  Base name, sans directory and suffix
     */
    String baseName;

    /**
     *  File extension, such as cpp or h
     */
    String suffix;

    /**
     *  Type of file: CFILE, HFILE, OFILE
     */
    int type;

    /**
     * Used to list files ref'd by this one
     */
    std::map<String, FileRec *> files;


private:

    void init()
        {
        }

    void assign(const FileRec &other)
        {
        type     = other.type;
        baseName = other.baseName;
        suffix   = other.suffix;
        files    = other.files;
        }

};



/**
 *  Simpler dependency record
 */
class DepRec
{
public:

    /**
     *  Constructor
     */
    DepRec()
        {init();}

    /**
     *  Copy constructor
     */
    DepRec(const DepRec &other)
        {init(); assign(other);}
    /**
     *  Constructor
     */
    DepRec(const String &fname)
        {init(); name = fname; }
    /**
     *  Assignment operator
     */
    DepRec &operator=(const DepRec &other)
        {init(); assign(other); return *this;}


    /**
     *  Destructor
     */
    ~DepRec()
        {}

    /**
     *  Directory part of the file name
     */
    String path;

    /**
     *  Base name, without the path and suffix
     */
    String name;

    /**
     *  Suffix of the source
     */
    String suffix;


    /**
     * Used to list files ref'd by this one
     */
    std::vector<String> files;


private:

    void init()
        {
        }

    void assign(const DepRec &other)
        {
        path     = other.path;
        name     = other.name;
        suffix   = other.suffix;
        files    = other.files;
        }

};


class DepTool : public MakeBase
{
public:

    /**
     *  Constructor
     */
    DepTool()
        {init();}

    /**
     *  Copy constructor
     */
    DepTool(const DepTool &other)
        {init(); assign(other);}

    /**
     *  Assignment operator
     */
    DepTool &operator=(const DepTool &other)
        {init(); assign(other); return *this;}


    /**
     *  Destructor
     */
    ~DepTool()
        {}


    /**
     *  Reset this section of code
     */
    virtual void init();
    
    /**
     *  Reset this section of code
     */
    virtual void assign(const DepTool &other)
        {
        }
    
    /**
     *  Sets the source directory which will be scanned
     */
    virtual void setSourceDirectory(const String &val)
        { sourceDir = val; }

    /**
     *  Returns the source directory which will be scanned
     */
    virtual String getSourceDirectory()
        { return sourceDir; }

    /**
     *  Sets the list of files within the directory to analyze
     */
    virtual void setFileList(const std::vector<String> &list)
        { fileList = list; }

    /**
     * Creates the list of all file names which will be
     * candidates for further processing.  Reads make.exclude
     * to see which files for directories to leave out.
     */
    virtual bool createFileList();


    /**
     *  Generates the forward dependency list
     */
    virtual bool generateDependencies();


    /**
     *  Generates the forward dependency list, saving the file
     */
    virtual bool generateDependencies(const String &);


    /**
     *  Load a dependency file
     */
    std::vector<DepRec> loadDepFile(const String &fileName);

    /**
     *  Load a dependency file, generating one if necessary
     */
    std::vector<DepRec> getDepFile(const String &fileName,
              bool forceRefresh);

    /**
     *  Save a dependency file
     */
    bool saveDepFile(const String &fileName);


private:


    /**
     *
     */
    void parseName(const String &fullname,
                   String &path,
                   String &basename,
                   String &suffix);

    /**
     *
     */
    int get(int pos);

    /**
     *
     */
    int skipwhite(int pos);

    /**
     *
     */
    int getword(int pos, String &ret);

    /**
     *
     */
    bool sequ(int pos, char *key);

    /**
     *
     */
    bool addIncludeFile(FileRec *frec, const String &fname);

    /**
     *
     */
    bool scanFile(const String &fname, FileRec *frec);

    /**
     *
     */
    bool processDependency(FileRec *ofile,
                           FileRec *include,
                           int depth);

    /**
     *
     */
    String sourceDir;

    /**
     *
     */
    std::vector<String> fileList;

    /**
     *
     */
    std::vector<String> directories;

    /**
     * A list of all files which will be processed for
     * dependencies.  This is the only list that has the actual
     * records.  All other lists have pointers to these records.     
     */
    std::map<String, FileRec *> allFiles;

    /**
     * The list of .o files, and the
     * dependencies upon them.
     */
    std::map<String, FileRec *> depFiles;

    int depFileSize;
    char *depFileBuf;

    static const int readBufSize = 8192;
    char readBuf[8193];//byte larger

};





/**
 *  Clean up after processing.  Called by the destructor, but should
 *  also be called before the object is reused.
 */
void DepTool::init()
{
    sourceDir = ".";

    fileList.clear();
    directories.clear();
    
    //clear refs
    depFiles.clear();
    //clear records
    std::map<String, FileRec *>::iterator iter;
    for (iter=allFiles.begin() ; iter!=allFiles.end() ; iter++)
         delete iter->second;

    allFiles.clear(); 

}




/**
 *  Parse a full path name into path, base name, and suffix
 */
void DepTool::parseName(const String &fullname,
                        String &path,
                        String &basename,
                        String &suffix)
{
    if (fullname.size() < 2)
        return;

    unsigned int pos = fullname.find_last_of('/');
    if (pos != fullname.npos && pos<fullname.size()-1)
        {
        path = fullname.substr(0, pos);
        pos++;
        basename = fullname.substr(pos, fullname.size()-pos);
        }
    else
        {
        path = "";
        basename = fullname;
        }

    pos = basename.find_last_of('.');
    if (pos != basename.npos && pos<basename.size()-1)
        {
        suffix   = basename.substr(pos+1, basename.size()-pos-1);
        basename = basename.substr(0, pos);
        }

    //trace("parsename:%s %s %s", path.c_str(),
    //        basename.c_str(), suffix.c_str()); 
}



/**
 *  Generate our internal file list.
 */
bool DepTool::createFileList()
{

    for (unsigned int i=0 ; i<fileList.size() ; i++)
        {
        String fileName = fileList[i];
        //trace("## FileName:%s", fileName.c_str());
        String path;
        String basename;
        String sfx;
        parseName(fileName, path, basename, sfx);
        if (sfx == "cpp" || sfx == "c" || sfx == "cxx"   ||
            sfx == "cc" || sfx == "CC")
            {
            FileRec *fe         = new FileRec(FileRec::CFILE);
            fe->path            = path;
            fe->baseName        = basename;
            fe->suffix          = sfx;
            allFiles[fileName]  = fe;
            }
        else if (sfx == "h"   ||  sfx == "hh"  ||
                 sfx == "hpp" ||  sfx == "hxx")
            {
            FileRec *fe         = new FileRec(FileRec::HFILE);
            fe->path            = path;
            fe->baseName        = basename;
            fe->suffix          = sfx;
            allFiles[fileName]  = fe;
            }
        }

    if (!listDirectories(sourceDir, "", directories))
        return false;
        
    return true;
}





/**
 * Get a character from the buffer at pos.  If out of range,
 * return -1 for safety
 */
int DepTool::get(int pos)
{
    if (pos>depFileSize)
        return -1;
    return depFileBuf[pos];
}



/**
 *  Skip over all whitespace characters beginning at pos.  Return
 *  the position of the first non-whitespace character.
 */
int DepTool::skipwhite(int pos)
{
    while (pos < depFileSize)
        {
        int ch = get(pos);
        if (ch < 0)
            break;
        if (!isspace(ch))
            break;
        pos++;
        }
    return pos;
}


/**
 *  Parse the buffer beginning at pos, for a word.  Fill
 *  'ret' with the result.  Return the position after the
 *  word.
 */
int DepTool::getword(int pos, String &ret)
{
    while (pos < depFileSize)
        {
        int ch = get(pos);
        if (ch < 0)
            break;
        if (isspace(ch))
            break;
        ret.push_back((char)ch);
        pos++;
        }
    return pos;
}

/**
 * Return whether the sequence of characters in the buffer
 * beginning at pos match the key,  for the length of the key
 */
bool DepTool::sequ(int pos, char *key)
{
    while (*key)
        {
        if (*key != get(pos))
            return false;
        key++; pos++;
        }
    return true;
}



/**
 *  Add an include file name to a file record.  If the name
 *  is not found in allFiles explicitly, try prepending include
 *  directory names to it and try again.
 */
bool DepTool::addIncludeFile(FileRec *frec, const String &iname)
{

    std::map<String, FileRec *>::iterator iter =
           allFiles.find(iname);
    if (iter != allFiles.end()) //already exists
        {
         //h file in same dir
        FileRec *other = iter->second;
        //trace("local: '%s'", iname.c_str());
        frec->files[iname] = other;
        return true;
        }
    else 
        {
        //look in other dirs
        std::vector<String>::iterator diter;
        for (diter=directories.begin() ;
             diter!=directories.end() ; diter++)
            {
            String dfname = *diter;
            dfname.append("/");
            dfname.append(iname);
            iter = allFiles.find(dfname);
            if (iter != allFiles.end())
                {
                FileRec *other = iter->second;
                //trace("other: '%s'", iname.c_str());
                frec->files[dfname] = other;
                return true;
                }
            }
        }
    return true;
}



/**
 *  Lightly parse a file to find the #include directives.  Do
 *  a bit of state machine stuff to make sure that the directive
 *  is valid.  (Like not in a comment).
 */
bool DepTool::scanFile(const String &fname, FileRec *frec)
{
    String fileName;
    if (sourceDir.size() > 0)
        {
        fileName.append(sourceDir);
        fileName.append("/");
        }
    fileName.append(fname);
    String nativeName = getNativePath(fileName);
    FILE *f = fopen(nativeName.c_str(), "r");
    if (!f)
        {
        error("Could not open '%s' for reading", fname.c_str());
        return false;
        }
    String buf;
    while (!feof(f))
        {
        int len = fread(readBuf, 1, readBufSize, f);
        readBuf[len] = '\0';
        buf.append(readBuf);
        }
    fclose(f);

    depFileSize = buf.size();
    depFileBuf  = (char *)buf.c_str();
    int pos = 0;


    while (pos < depFileSize)
        {
        //trace("p:%c", get(pos));

        //# Block comment
        if (get(pos) == '/' && get(pos+1) == '*')
            {
            pos += 2;
            while (pos < depFileSize)
                {
                if (get(pos) == '*' && get(pos+1) == '/')
                    {
                    pos += 2;
                    break;
                    }
                else
                    pos++;
                }
            }
        //# Line comment
        else if (get(pos) == '/' && get(pos+1) == '/')
            {
            pos += 2;
            while (pos < depFileSize)
                {
                if (get(pos) == '\n')
                    {
                    pos++;
                    break;
                    }
                else
                    pos++;
                }
            }
        //# #include! yaay
        else if (sequ(pos, "#include"))
            {
            pos += 8;
            pos = skipwhite(pos);
            String iname;
            pos = getword(pos, iname);
            if (iname.size()>2)
                {
                iname = iname.substr(1, iname.size()-2);
                addIncludeFile(frec, iname);
                }
            }
        else
            {
            pos++;
            }
        }

    return true;
}



/**
 *  Recursively check include lists to find all files in allFiles to which
 *  a given file is dependent.
 */
bool DepTool::processDependency(FileRec *ofile,
                             FileRec *include,
                             int depth)
{
    std::map<String, FileRec *>::iterator iter;
    for (iter=include->files.begin() ; iter!=include->files.end() ; iter++)
        {
        String fname  = iter->first;
        if (ofile->files.find(fname) != ofile->files.end())
            {
            //trace("file '%s' already seen", fname.c_str());
            continue;
            }
        FileRec *child  = iter->second;
        ofile->files[fname] = child;
      
        processDependency(ofile, child, depth+1);
        }


    return true;
}





/**
 *  Generate the file dependency list.
 */
bool DepTool::generateDependencies()
{
    std::map<String, FileRec *>::iterator iter;
    //# First pass.  Scan for all includes
    for (iter=allFiles.begin() ; iter!=allFiles.end() ; iter++)
        {
        FileRec *frec = iter->second;
        if (!scanFile(iter->first, frec))
            {
            //quit?
            }
        }

    //# Second pass.  Scan for all includes
    for (iter=allFiles.begin() ; iter!=allFiles.end() ; iter++)
        {
        FileRec *include = iter->second;
        if (include->type == FileRec::CFILE)
            {
            String cFileName = iter->first;
            FileRec *ofile      = new FileRec(FileRec::OFILE);
            ofile->path         = include->path;
            ofile->baseName     = include->baseName;
            ofile->suffix       = include->suffix;
            String fname     = include->path;
            if (fname.size()>0)
                fname.append("/");
            fname.append(include->baseName);
            fname.append(".o");
            depFiles[fname]    = ofile;
            //add the .c file first?   no, don't
            //ofile->files[cFileName] = include;
            
            //trace("ofile:%s", fname.c_str());

            processDependency(ofile, include, 0);
            }
        }

      
    return true;
}



/**
 *  High-level call to generate deps and optionally save them
 */
bool DepTool::generateDependencies(const String &fileName)
{
    if (!createFileList())
        return false;
    if (!generateDependencies())
        return false;
    if (!saveDepFile(fileName))
        return false;
    return true;
}


/**
 *   This saves the dependency cache.
 */
bool DepTool::saveDepFile(const String &fileName)
{
    time_t tim;
    time(&tim);

    FILE *f = fopen(fileName.c_str(), "w");
    if (!f)
        {
        trace("cannot open '%s' for writing", fileName.c_str());
        }
    fprintf(f, "<?xml version='1.0'?>\n");
    fprintf(f, "<!--\n");
    fprintf(f, "########################################################\n");
    fprintf(f, "## File: build.dep\n");
    fprintf(f, "## Generated by BuildTool at :%s", ctime(&tim));
    fprintf(f, "########################################################\n");
    fprintf(f, "-->\n");

    fprintf(f, "<dependencies source='%s'>\n\n", sourceDir.c_str());
    std::map<String, FileRec *>::iterator iter;
    for (iter=depFiles.begin() ; iter!=depFiles.end() ; iter++)
        {
        FileRec *frec = iter->second;
        if (frec->type == FileRec::OFILE)
            {
            fprintf(f, "<object path='%s' name='%s' suffix='%s'>\n",
                 frec->path.c_str(), frec->baseName.c_str(), frec->suffix.c_str());
            std::map<String, FileRec *>::iterator citer;
            for (citer=frec->files.begin() ; citer!=frec->files.end() ; citer++)
                {
                String cfname = citer->first;
                fprintf(f, "    <dep name='%s'/>\n", cfname.c_str());
                }
            fprintf(f, "</object>\n\n");
            }
        }

    fprintf(f, "</dependencies>\n");
    fprintf(f, "\n");
    fprintf(f, "<!--\n");
    fprintf(f, "########################################################\n");
    fprintf(f, "## E N D\n");
    fprintf(f, "########################################################\n");
    fprintf(f, "-->\n");

    fclose(f);

    return true;
}




/**
 *   This loads the dependency cache.
 */
std::vector<DepRec> DepTool::loadDepFile(const String &depFile)
{
    std::vector<DepRec> result;
    
    Parser parser;
    Element *root = parser.parseFile(depFile.c_str());
    if (!root)
        {
        //error("Could not open %s for reading", depFile.c_str());
        return result;
        }

    if (root->getChildren().size()==0 ||
        root->getChildren()[0]->getName()!="dependencies")
        {
        error("Main xml element should be <dependencies>");
        delete root;
        return result;
        }

    //########## Start parsing
    Element *depList = root->getChildren()[0];

    std::vector<Element *> objects = depList->getChildren();
    for (unsigned int i=0 ; i<objects.size() ; i++)
        {
        Element *objectElem = objects[i];
        String tagName = objectElem->getName();
        if (tagName == "object")
            {
            String objName   = objectElem->getAttribute("name");
             //trace("object:%s", objName.c_str());
            DepRec depObject(objName);
            depObject.path   = objectElem->getAttribute("path");
            depObject.suffix = objectElem->getAttribute("suffix");
            //########## DESCRIPTION
            std::vector<Element *> depElems = objectElem->getChildren();
            for (unsigned int i=0 ; i<depElems.size() ; i++)
                {
                Element *depElem = depElems[i];
                tagName = depElem->getName();
                if (tagName == "dep")
                    {
                    String depName = depElem->getAttribute("name");
                    //trace("    dep:%s", depName.c_str());
                    depObject.files.push_back(depName);
                    }
                }
            //Insert into the result list, in a sorted manner
            bool inserted = false;
            std::vector<DepRec>::iterator iter;
            for (iter = result.begin() ; iter != result.end() ; iter++)
                {
                String vpath = iter->path;
                vpath.append("/");
                vpath.append(iter->name);
                String opath = depObject.path;
                opath.append("/");
                opath.append(depObject.name);
                if (vpath > opath)
                    {
                    inserted = true;
                    iter = result.insert(iter, depObject);
                    break;
                    }
                }
            if (!inserted)
                result.push_back(depObject);
            }
        }

    delete root;

    return result;
}


/**
 *   This loads the dependency cache.
 */
std::vector<DepRec> DepTool::getDepFile(const String &depFile,
                   bool forceRefresh)
{
    std::vector<DepRec> result;
    if (forceRefresh)
        {
        generateDependencies(depFile);
        result = loadDepFile(depFile);
        }
    else
        {
        //try once
        result = loadDepFile(depFile);
        if (result.size() == 0)
            {
            //fail? try again
            generateDependencies(depFile);
            result = loadDepFile(depFile);
            }
        }
    return result;
}




//########################################################################
//# T A S K
//########################################################################
//forward decl
class Target;
class Make;

/**
 *
 */
class Task : public MakeBase
{

public:

    typedef enum
        {
        TASK_NONE,
        TASK_CC,
        TASK_COPY,
        TASK_DELETE,
        TASK_JAR,
        TASK_JAVAC,
        TASK_LINK,
        TASK_MAKEFILE,
        TASK_MKDIR,
        TASK_MSGFMT,
        TASK_RANLIB,
        TASK_RC,
        TASK_SHAREDLIB,
        TASK_STATICLIB,
        TASK_STRIP,
        TASK_TSTAMP
        } TaskType;
        

    /**
     *
     */
    Task(MakeBase &par) : parent(par)
        { init(); }

    /**
     *
     */
    Task(const Task &other) : parent(other.parent)
        { init(); assign(other); }

    /**
     *
     */
    Task &operator=(const Task &other)
        { assign(other); return *this; }

    /**
     *
     */
    virtual ~Task()
        { }


    /**
     *
     */
    virtual MakeBase &getParent()
        { return parent; }

     /**
     *
     */
    virtual int  getType()
        { return type; }

    /**
     *
     */
    virtual void setType(int val)
        { type = val; }

    /**
     *
     */
    virtual String getName()
        { return name; }

    /**
     *
     */
    virtual bool execute()
        { return true; }

    /**
     *
     */
    virtual bool parse(Element *elem)
        { return true; }

    /**
     *
     */
    Task *createTask(Element *elem);


protected:

    void init()
        {
        type = TASK_NONE;
        name = "none";
        }

    void assign(const Task &other)
        {
        type = other.type;
        name = other.name;
        }
        
    String getAttribute(Element *elem, const String &attrName)
        {
        String str;
        return str;
        }

    MakeBase &parent;

    int type;

    String name;
};



/**
 * This task runs the C/C++ compiler.  The compiler is invoked
 * for all .c or .cpp files which are newer than their correcsponding
 * .o files.  
 */
class TaskCC : public Task
{
public:

    TaskCC(MakeBase &par) : Task(par)
        {
        type = TASK_CC; name = "cc";
        ccCommand   = "gcc";
        cxxCommand  = "g++";
        source      = ".";
        dest        = ".";
        flags       = "";
        defines     = "";
        includes    = "";
        fileSet.clear();
        }

    virtual ~TaskCC()
        {}

    virtual bool needsCompiling(const DepRec &depRec,
              const String &src, const String &dest)
        {
        return false;
        }

    virtual bool execute()
        {
        if (!listFiles(parent, fileSet))
            return false;

        bool refreshCache = false;
        String fullName = parent.resolve("build.dep");
        if (isNewerThan(parent.getURI().getPath(), fullName))
            {
            status("          : regenerating C/C++ dependency cache");
            refreshCache = true;
            }

        DepTool depTool;
        depTool.setSourceDirectory(source);
        depTool.setFileList(fileSet.getFiles());
        std::vector<DepRec> deps =
             depTool.getDepFile("build.dep", refreshCache);
        
        String incs;
        incs.append("-I");
        incs.append(parent.resolve("."));
        incs.append(" ");
        if (includes.size()>0)
            {
            incs.append(includes);
            incs.append(" ");
            }
        std::set<String> paths;
        std::vector<DepRec>::iterator viter;
        for (viter=deps.begin() ; viter!=deps.end() ; viter++)
            {
            DepRec dep = *viter;
            if (dep.path.size()>0)
                paths.insert(dep.path);
            }
        if (source.size()>0)
            {
            incs.append(" -I");
            incs.append(parent.resolve(source));
            incs.append(" ");
            }
        std::set<String>::iterator setIter;
        for (setIter=paths.begin() ; setIter!=paths.end() ; setIter++)
            {
            incs.append(" -I");
            String dname;
            if (source.size()>0)
                {
                dname.append(source);
                dname.append("/");
                }
            dname.append(*setIter);
            incs.append(parent.resolve(dname));
            }
        std::vector<String> cfiles;
        for (viter=deps.begin() ; viter!=deps.end() ; viter++)
            {
            DepRec dep = *viter;

            //## Select command
            String sfx = dep.suffix;
            String command = ccCommand;
            if (sfx == "cpp" || sfx == "c++" || sfx == "cc"
                 || sfx == "CC")
                command = cxxCommand;
 
            //## Make paths
            String destPath = dest;
            String srcPath  = source;
            if (dep.path.size()>0)
                {
                destPath.append("/");
                destPath.append(dep.path);
                srcPath.append("/");
                srcPath.append(dep.path);
                }
            //## Make sure destination directory exists
            if (!createDirectory(destPath))
                return false;
                
            //## Check whether it needs to be done
            String destName;
            if (destPath.size()>0)
                {
                destName.append(destPath);
                destName.append("/");
                }
            destName.append(dep.name);
            destName.append(".o");
            String destFullName = parent.resolve(destName);
            String srcName;
            if (srcPath.size()>0)
                {
                srcName.append(srcPath);
                srcName.append("/");
                }
            srcName.append(dep.name);
            srcName.append(".");
            srcName.append(dep.suffix);
            String srcFullName = parent.resolve(srcName);
            bool compileMe = false;
            if (isNewerThan(srcFullName, destFullName))
                {
                status("          : compile of %s required by %s",
                        destFullName.c_str(), srcFullName.c_str());
                compileMe = true;
                }
            else
                {
                for (unsigned int i=0 ; i<dep.files.size() ; i++)
                    {
                    String depName;
                    if (srcPath.size()>0)
                        {
                        depName.append(srcPath);
                        depName.append("/");
                        }
                    depName.append(dep.files[i]);
                    String depFullName = parent.resolve(depName);
                    if (isNewerThan(depFullName, destFullName))
                        {
                        status("          : compile of %s required by %s",
                                destFullName.c_str(), depFullName.c_str());
                        compileMe = true;
                        break;
                        }
                    }
                }
            if (!compileMe)
                {
                continue;
                }

            //## Assemble the command
            String cmd = command;
            cmd.append(" -c ");
            cmd.append(flags);
            cmd.append(" ");
            cmd.append(defines);
            cmd.append(" ");
            cmd.append(incs);
            cmd.append(" ");
            cmd.append(srcFullName);
            cmd.append(" -o ");
            cmd.append(destFullName);

            //## Execute the command

            String outString, errString;
            if (!executeCommand(cmd.c_str(), "", outString, errString))
                {
                error("problem compiling: %s", errString.c_str());
                return false;
                }
            }
        
        return true;
        }

    virtual bool parse(Element *elem)
        {
        String s;
        if (!parent.getAttribute(elem, "command", s))
            return false;
        if (s.size()>0) { ccCommand = s; cxxCommand = s; }
        if (!parent.getAttribute(elem, "cc", s))
            return false;
        if (s.size()>0) ccCommand = s;
        if (!parent.getAttribute(elem, "cxx", s))
            return false;
        if (s.size()>0) cxxCommand = s;
        if (!parent.getAttribute(elem, "destdir", s))
            return false;
        if (s.size()>0) dest = s;

        std::vector<Element *> children = elem->getChildren();
        for (unsigned int i=0 ; i<children.size() ; i++)
            {
            Element *child = children[i];
            String tagName = child->getName();
            if (tagName == "flags")
                {
                if (!parent.getValue(child, flags))
                    return false;
                flags = strip(flags);
                }
            else if (tagName == "includes")
                {
                if (!parent.getValue(child, includes))
                    return false;
                includes = strip(includes);
                }
            else if (tagName == "defines")
                {
                if (!parent.getValue(child, defines))
                    return false;
                defines = strip(defines);
                }
            else if (tagName == "fileset")
                {
                if (!parseFileSet(child, parent, fileSet))
                    return false;
                source = fileSet.getDirectory();
                }
            }

        return true;
        }
        
protected:

    String ccCommand;
    String cxxCommand;
    String source;
    String dest;
    String flags;
    String defines;
    String includes;
    FileSet fileSet;
    
};



/**
 *
 */
class TaskCopy : public Task
{
public:

    typedef enum
        {
        CP_NONE,
        CP_TOFILE,
        CP_TODIR
        } CopyType;

    TaskCopy(MakeBase &par) : Task(par)
        {
        type = TASK_COPY; name = "copy";
        cptype = CP_NONE;
        verbose = false;
        haveFileSet = false;
        }

    virtual ~TaskCopy()
        {}

    virtual bool execute()
        {
        switch (cptype)
           {
           case CP_TOFILE:
               {
               if (fileName.size()>0)
                   {
                   status("          : %s to %s",
                        fileName.c_str(), toFileName.c_str());
                   String fullSource = parent.resolve(fileName);
                   String fullDest = parent.resolve(toFileName);
                   //trace("copy %s to file %s", fullSource.c_str(),
                   //                       fullDest.c_str());
                   if (!isRegularFile(fullSource))
                       {
                       error("copy : file %s does not exist", fullSource.c_str());
                       return false;
                       }
                   if (!isNewerThan(fullSource, fullDest))
                       {
                       return true;
                       }
                   if (!copyFile(fullSource, fullDest))
                       return false;
                   status("          : 1 file copied");
                   }
               return true;
               }
           case CP_TODIR:
               {
               if (haveFileSet)
                   {
                   if (!listFiles(parent, fileSet))
                       return false;
                   String fileSetDir = fileSet.getDirectory();

                   status("          : %s to %s",
                       fileSetDir.c_str(), toDirName.c_str());

                   int nrFiles = 0;
                   for (unsigned int i=0 ; i<fileSet.size() ; i++)
                       {
                       String fileName = fileSet[i];

                       String sourcePath;
                       if (fileSetDir.size()>0)
                           {
                           sourcePath.append(fileSetDir);
                           sourcePath.append("/");
                           }
                       sourcePath.append(fileName);
                       String fullSource = parent.resolve(sourcePath);
                       
                       //Get the immediate parent directory's base name
                       String baseFileSetDir = fileSetDir;
                       unsigned int pos = baseFileSetDir.find_last_of('/');
                       if (pos!=baseFileSetDir.npos &&
                                  pos < baseFileSetDir.size()-1)
                           baseFileSetDir =
                              baseFileSetDir.substr(pos+1,
                                   baseFileSetDir.size());
                       //Now make the new path
                       String destPath;
                       if (toDirName.size()>0)
                           {
                           destPath.append(toDirName);
                           destPath.append("/");
                           }
                       if (baseFileSetDir.size()>0)
                           {
                           destPath.append(baseFileSetDir);
                           destPath.append("/");
                           }
                       destPath.append(fileName);
                       String fullDest = parent.resolve(destPath);
                       //trace("fileName:%s", fileName.c_str());
                       //trace("copy %s to new dir : %s", fullSource.c_str(),
                       //                   fullDest.c_str());
                       if (!isNewerThan(fullSource, fullDest))
                           {
                           //trace("copy skipping %s", fullSource.c_str());
                           continue;
                           }
                       if (!copyFile(fullSource, fullDest))
                           return false;
                       nrFiles++;
                       }
                   status("          : %d file(s) copied", nrFiles);
                   }
               else //file source
                   {
                   //For file->dir we want only the basename of
                   //the source appended to the dest dir
                   status("          : %s to %s", 
                       fileName.c_str(), toDirName.c_str());
                   String baseName = fileName;
                   unsigned int pos = baseName.find_last_of('/');
                   if (pos!=baseName.npos && pos<baseName.size()-1)
                       baseName = baseName.substr(pos+1, baseName.size());
                   String fullSource = parent.resolve(fileName);
                   String destPath;
                   if (toDirName.size()>0)
                       {
                       destPath.append(toDirName);
                       destPath.append("/");
                       }
                   destPath.append(baseName);
                   String fullDest = parent.resolve(destPath);
                   //trace("copy %s to new dir : %s", fullSource.c_str(),
                   //                       fullDest.c_str());
                   if (!isRegularFile(fullSource))
                       {
                       error("copy : file %s does not exist", fullSource.c_str());
                       return false;
                       }
                   if (!isNewerThan(fullSource, fullDest))
                       {
                       return true;
                       }
                   if (!copyFile(fullSource, fullDest))
                       return false;
                   status("          : 1 file copied");
                   }
               return true;
               }
           }
        return true;
        }


    virtual bool parse(Element *elem)
        {
        if (!parent.getAttribute(elem, "file", fileName))
            return false;
        if (!parent.getAttribute(elem, "tofile", toFileName))
            return false;
        if (toFileName.size() > 0)
            cptype = CP_TOFILE;
        if (!parent.getAttribute(elem, "todir", toDirName))
            return false;
        if (toDirName.size() > 0)
            cptype = CP_TODIR;
        String ret;
        if (!parent.getAttribute(elem, "verbose", ret))
            return false;
        if (ret.size()>0 && !getBool(ret, verbose))
            return false;
            
        haveFileSet = false;
        
        std::vector<Element *> children = elem->getChildren();
        for (unsigned int i=0 ; i<children.size() ; i++)
            {
            Element *child = children[i];
            String tagName = child->getName();
            if (tagName == "fileset")
                {
                if (!parseFileSet(child, parent, fileSet))
                    {
                    error("problem getting fileset");
                    return false;
                    }
                haveFileSet = true;
                }
            }

        //Perform validity checks
        if (fileName.size()>0 && fileSet.size()>0)
            {
            error("<copy> can only have one of : file= and <fileset>");
            return false;
            }
        if (toFileName.size()>0 && toDirName.size()>0)
            {
            error("<copy> can only have one of : tofile= or todir=");
            return false;
            }
        if (haveFileSet && toDirName.size()==0)
            {
            error("a <copy> task with a <fileset> must have : todir=");
            return false;
            }
        if (cptype == CP_TOFILE && fileName.size()==0)
            {
            error("<copy> tofile= must be associated with : file=");
            return false;
            }
        if (cptype == CP_TODIR && fileName.size()==0 && !haveFileSet)
            {
            error("<copy> todir= must be associated with : file= or <fileset>");
            return false;
            }

        return true;
        }
        
private:

    int cptype;
    String fileName;
    FileSet fileSet;
    String toFileName;
    String toDirName;
    bool verbose;
    bool haveFileSet;
};


/**
 *
 */
class TaskDelete : public Task
{
public:

    typedef enum
        {
        DEL_FILE,
        DEL_DIR,
        DEL_FILESET
        } DeleteType;

    TaskDelete(MakeBase &par) : Task(par)
        { 
          type        = TASK_DELETE;
          name        = "delete";
          delType     = DEL_FILE;
          verbose     = false;
          quiet       = false;
          failOnError = true;
        }

    virtual ~TaskDelete()
        {}

    virtual bool execute()
        {
        struct stat finfo;
        switch (delType)
            {
            case DEL_FILE:
                {
                status("          : %s", fileName.c_str());
                String fullName = parent.resolve(fileName);
                char *fname = (char *)fullName.c_str();
                //does not exist
                if (stat(fname, &finfo)<0)
                    return true;
                //exists but is not a regular file
                if (!S_ISREG(finfo.st_mode))
                    {
                    error("<delete> failed. '%s' exists and is not a regular file",
                          fname);
                    return false;
                    }
                if (remove(fname)<0)
                    {
                    error("<delete> failed: %s", strerror(errno));
                    return false;
                    }
                return true;
                }
            case DEL_DIR:
                {
                status("          : %s", dirName.c_str());
                String fullDir = parent.resolve(dirName);
                if (!removeDirectory(fullDir))
                    return false;
                return true;
                }
            }
        return true;
        }

    virtual bool parse(Element *elem)
        {
        if (!parent.getAttribute(elem, "file", fileName))
            return false;
        if (fileName.size() > 0)
            delType = DEL_FILE;
        if (!parent.getAttribute(elem, "dir", dirName))
            return false;
        if (dirName.size() > 0)
            delType = DEL_DIR;
        if (fileName.size()>0 && dirName.size()>0)
            {
            error("<delete> can only have one attribute of file= or dir=");
            return false;
            }
        String ret;
        if (!parent.getAttribute(elem, "verbose", ret))
            return false;
        if (ret.size()>0 && !getBool(ret, verbose))
            return false;
        if (!parent.getAttribute(elem, "quiet", ret))
            return false;
        if (ret.size()>0 && !getBool(ret, quiet))
            return false;
        if (!parent.getAttribute(elem, "failonerror", ret))
            return false;
        if (ret.size()>0 && !getBool(ret, failOnError))
            return false;
        return true;
        }

private:

    int delType;
    String dirName;
    String fileName;
    bool verbose;
    bool quiet;
    bool failOnError;
};


/**
 *
 */
class TaskJar : public Task
{
public:

    TaskJar(MakeBase &par) : Task(par)
        { type = TASK_JAR; name = "jar"; }

    virtual ~TaskJar()
        {}

    virtual bool execute()
        {
        return true;
        }

    virtual bool parse(Element *elem)
        {
        return true;
        }
};


/**
 *
 */
class TaskJavac : public Task
{
public:

    TaskJavac(MakeBase &par) : Task(par)
        { type = TASK_JAVAC; name = "javac"; }

    virtual ~TaskJavac()
        {}

    virtual bool execute()
        {
        return true;
        }

    virtual bool parse(Element *elem)
        {
        return true;
        }
};


/**
 *
 */
class TaskLink : public Task
{
public:

    TaskLink(MakeBase &par) : Task(par)
        {
        type = TASK_LINK; name = "link";
        command = "g++";
        doStrip = false;
                stripCommand = "strip";
                objcopyCommand = "objcopy";
        }

    virtual ~TaskLink()
        {}

    virtual bool execute()
        {
        if (!listFiles(parent, fileSet))
            return false;
        String fileSetDir = fileSet.getDirectory();
        //trace("%d files in %s", fileSet.size(), fileSetDir.c_str());
        bool doit = false;
        String fullTarget = parent.resolve(fileName);
        String cmd = command;
        cmd.append(" -o ");
        cmd.append(fullTarget);
        cmd.append(" ");
        cmd.append(flags);
        for (unsigned int i=0 ; i<fileSet.size() ; i++)
            {
            cmd.append(" ");
            String obj;
            if (fileSetDir.size()>0)
                {
                obj.append(fileSetDir);
                obj.append("/");
                }
            obj.append(fileSet[i]);
            String fullObj = parent.resolve(obj);
            cmd.append(fullObj);
            //trace("link: tgt:%s obj:%s", fullTarget.c_str(),
            //          fullObj.c_str());
            if (isNewerThan(fullObj, fullTarget))
                doit = true;
            }
        cmd.append(" ");
        cmd.append(libs);
        if (!doit)
            {
            //trace("link not needed");
            return true;
            }
        //trace("LINK cmd:%s", cmd.c_str());


        String outbuf, errbuf;
        if (!executeCommand(cmd.c_str(), "", outbuf, errbuf))
            {
            error("LINK problem: %s", errbuf.c_str());
            return false;
            }

        if (symFileName.size()>0)
            {
            String symFullName = parent.resolve(symFileName);
            cmd = objcopyCommand;
            cmd.append(" --only-keep-debug ");
            cmd.append(getNativePath(fullTarget));
            cmd.append(" ");
            cmd.append(getNativePath(symFullName));
            if (!executeCommand(cmd, "", outbuf, errbuf))
                {
                error("<strip> symbol file failed : %s", errbuf.c_str());
                return false;
                }
            }
            
        if (doStrip)
            {
            cmd = stripCommand;
            cmd.append(" ");
            cmd.append(getNativePath(fullTarget));
            if (!executeCommand(cmd, "", outbuf, errbuf))
               {
               error("<strip> failed : %s", errbuf.c_str());
               return false;
               }
            }

        return true;
        }

    virtual bool parse(Element *elem)
        {
        String s;
        if (!parent.getAttribute(elem, "command", s))
            return false;
        if (s.size()>0)
            command = s;
        if (!parent.getAttribute(elem, "objcopycommand", s))
            return false;
        if (s.size()>0)
            objcopyCommand = s;
        if (!parent.getAttribute(elem, "stripcommand", s))
            return false;
        if (s.size()>0)
            stripCommand = s;
        if (!parent.getAttribute(elem, "out", fileName))
            return false;
        if (!parent.getAttribute(elem, "strip", s))
            return false;
        if (s.size()>0 && !getBool(s, doStrip))
            return false;
        if (!parent.getAttribute(elem, "symfile", symFileName))
            return false;
            
        std::vector<Element *> children = elem->getChildren();
        for (unsigned int i=0 ; i<children.size() ; i++)
            {
            Element *child = children[i];
            String tagName = child->getName();
            if (tagName == "fileset")
                {
                if (!parseFileSet(child, parent, fileSet))
                    return false;
                }
            else if (tagName == "flags")
                {
                if (!parent.getValue(child, flags))
                    return false;
                flags = strip(flags);
                }
            else if (tagName == "libs")
                {
                if (!parent.getValue(child, libs))
                    return false;
                libs = strip(libs);
                }
            }
        return true;
        }

private:

    String  command;
    String  fileName;
    String  flags;
    String  libs;
    FileSet fileSet;
    bool    doStrip;
    String  symFileName;
    String  stripCommand;
    String  objcopyCommand;

};



/**
 * Create a named directory
 */
class TaskMakeFile : public Task
{
public:

    TaskMakeFile(MakeBase &par) : Task(par)
        { type = TASK_MAKEFILE; name = "makefile"; }

    virtual ~TaskMakeFile()
        {}

    virtual bool execute()
        {
        status("          : %s", fileName.c_str());
        String fullName = parent.resolve(fileName);
        if (!isNewerThan(parent.getURI().getPath(), fullName))
            {
            //trace("skipped <makefile>");
            return true;
            }
        //trace("fullName:%s", fullName.c_str());
        FILE *f = fopen(fullName.c_str(), "w");
        if (!f)
            {
            error("<makefile> could not open %s for writing : %s",
                fullName.c_str(), strerror(errno));
            return false;
            }
        for (unsigned int i=0 ; i<text.size() ; i++)
            fputc(text[i], f);
        fputc('\n', f);
        fclose(f);
        return true;
        }

    virtual bool parse(Element *elem)
        {
        if (!parent.getAttribute(elem, "file", fileName))
            return false;
        if (fileName.size() == 0)
            {
            error("<makefile> requires 'file=\"filename\"' attribute");
            return false;
            }
        if (!parent.getValue(elem, text))
            return false;
        text = leftJustify(text);
        //trace("dirname:%s", dirName.c_str());
        return true;
        }

private:

    String fileName;
    String text;
};



/**
 * Create a named directory
 */
class TaskMkDir : public Task
{
public:

    TaskMkDir(MakeBase &par) : Task(par)
        { type = TASK_MKDIR; name = "mkdir"; }

    virtual ~TaskMkDir()
        {}

    virtual bool execute()
        {
        status("          : %s", dirName.c_str());
        String fullDir = parent.resolve(dirName);
        //trace("fullDir:%s", fullDir.c_str());
        if (!createDirectory(fullDir))
            return false;
        return true;
        }

    virtual bool parse(Element *elem)
        {
        if (!parent.getAttribute(elem, "dir", dirName))
            return false;
        if (dirName.size() == 0)
            {
            error("<mkdir> requires 'dir=\"dirname\"' attribute");
            return false;
            }
        return true;
        }

private:

    String dirName;
};



/**
 * Create a named directory
 */
class TaskMsgFmt: public Task
{
public:

    TaskMsgFmt(MakeBase &par) : Task(par)
         {
         type    = TASK_MSGFMT;
         name    = "msgfmt";
         command = "msgfmt";
         owndir  = false;
         outName = "";
         }

    virtual ~TaskMsgFmt()
        {}

    virtual bool execute()
        {
        if (!listFiles(parent, fileSet))
            return false;
        String fileSetDir = fileSet.getDirectory();

        //trace("msgfmt: %d", fileSet.size());
        for (unsigned int i=0 ; i<fileSet.size() ; i++)
            {
            String fileName = fileSet[i];
            if (getSuffix(fileName) != "po")
                continue;
            String sourcePath;
            if (fileSetDir.size()>0)
                {
                sourcePath.append(fileSetDir);
                sourcePath.append("/");
                }
            sourcePath.append(fileName);
            String fullSource = parent.resolve(sourcePath);

            String destPath;
            if (toDirName.size()>0)
                {
                destPath.append(toDirName);
                destPath.append("/");
                }
            if (owndir)
                {
                String subdir = fileName;
                unsigned int pos = subdir.find_last_of('.');
                if (pos != subdir.npos)
                    subdir = subdir.substr(0, pos);
                destPath.append(subdir);
                destPath.append("/");
                }
            //Pick the output file name
            if (outName.size() > 0)
                {
                destPath.append(outName);
                }
            else
                {
                destPath.append(fileName);
                destPath[destPath.size()-2] = 'm';
                }

            String fullDest = parent.resolve(destPath);

            if (!isNewerThan(fullSource, fullDest))
                {
                //trace("skip %s", fullSource.c_str());
                continue;
                }
                
            String cmd = command;
            cmd.append(" ");
            cmd.append(fullSource);
            cmd.append(" -o ");
            cmd.append(fullDest);
            
            int pos = fullDest.find_last_of('/');
            if (pos>0)
                {
                String fullDestPath = fullDest.substr(0, pos);
                if (!createDirectory(fullDestPath))
                    return false;
                }



            String outString, errString;
            if (!executeCommand(cmd.c_str(), "", outString, errString))
                {
                error("<msgfmt> problem: %s", errString.c_str());
                return false;
                }
            }

        return true;
        }

    virtual bool parse(Element *elem)
        {
        String s;
        if (!parent.getAttribute(elem, "command", s))
            return false;
        if (s.size()>0)
            command = s;
        if (!parent.getAttribute(elem, "todir", toDirName))
            return false;
        if (!parent.getAttribute(elem, "out", outName))
            return false;
        if (!parent.getAttribute(elem, "owndir", s))
            return false;
        if (s.size()>0 && !getBool(s, owndir))
            return false;
            
        std::vector<Element *> children = elem->getChildren();
        for (unsigned int i=0 ; i<children.size() ; i++)
            {
            Element *child = children[i];
            String tagName = child->getName();
            if (tagName == "fileset")
                {
                if (!parseFileSet(child, parent, fileSet))
                    return false;
                }
            }
        return true;
        }

private:

    String  command;
    String  toDirName;
    String  outName;
    FileSet fileSet;
    bool    owndir;

};





/**
 *  Process an archive to allow random access
 */
class TaskRanlib : public Task
{
public:

    TaskRanlib(MakeBase &par) : Task(par)
        {
        type = TASK_RANLIB; name = "ranlib";
        command = "ranlib";
        }

    virtual ~TaskRanlib()
        {}

    virtual bool execute()
        {
        String fullName = parent.resolve(fileName);
        //trace("fullDir:%s", fullDir.c_str());
        String cmd = command;
        cmd.append(" ");
        cmd.append(fullName);
        String outbuf, errbuf;
        if (!executeCommand(cmd, "", outbuf, errbuf))
            return false;
        return true;
        }

    virtual bool parse(Element *elem)
        {
        String s;
        if (!parent.getAttribute(elem, "command", s))
            return false;
        if (s.size()>0)
           command = s;
        if (!parent.getAttribute(elem, "file", fileName))
            return false;
        if (fileName.size() == 0)
            {
            error("<ranlib> requires 'file=\"fileNname\"' attribute");
            return false;
            }
        return true;
        }

private:

    String fileName;
    String command;
};



/**
 * Run the "ar" command to archive .o's into a .a
 */
class TaskRC : public Task
{
public:

    TaskRC(MakeBase &par) : Task(par)
        {
        type = TASK_RC; name = "rc";
        command = "windres";
        }

    virtual ~TaskRC()
        {}

    virtual bool execute()
        {
        String fullFile = parent.resolve(fileName);
        String fullOut  = parent.resolve(outName);
        if (!isNewerThan(fullFile, fullOut))
            return true;
        String cmd = command;
        cmd.append(" -o ");
        cmd.append(fullOut);
        cmd.append(" ");
        cmd.append(flags);
        cmd.append(" ");
        cmd.append(fullFile);

        String outString, errString;
        if (!executeCommand(cmd.c_str(), "", outString, errString))
            {
            error("RC problem: %s", errString.c_str());
            return false;
            }
        return true;
        }

    virtual bool parse(Element *elem)
        {
        if (!parent.getAttribute(elem, "command", command))
            return false;
        if (!parent.getAttribute(elem, "file", fileName))
            return false;
        if (!parent.getAttribute(elem, "out", outName))
            return false;
        std::vector<Element *> children = elem->getChildren();
        for (unsigned int i=0 ; i<children.size() ; i++)
            {
            Element *child = children[i];
            String tagName = child->getName();
            if (tagName == "flags")
                {
                if (!parent.getValue(child, flags))
                    return false;
                }
            }
        return true;
        }

private:

    String command;
    String flags;
    String fileName;
    String outName;

};



/**
 *  Collect .o's into a .so or DLL
 */
class TaskSharedLib : public Task
{
public:

    TaskSharedLib(MakeBase &par) : Task(par)
        {
        type = TASK_SHAREDLIB; name = "dll";
        command = "ar crv";
        }

    virtual ~TaskSharedLib()
        {}

    virtual bool execute()
        {
        //trace("###########HERE %d", fileSet.size());
        bool doit = false;
        
        String fullOut = parent.resolve(fileName);
        //trace("ar fullout: %s", fullOut.c_str());
        
        if (!listFiles(parent, fileSet))
            return false;
        String fileSetDir = fileSet.getDirectory();

        for (unsigned int i=0 ; i<fileSet.size() ; i++)
            {
            String fname;
            if (fileSetDir.size()>0)
                {
                fname.append(fileSetDir);
                fname.append("/");
                }
            fname.append(fileSet[i]);
            String fullName = parent.resolve(fname);
            //trace("ar : %s/%s", fullOut.c_str(), fullName.c_str());
            if (isNewerThan(fullName, fullOut))
                doit = true;
            }
        //trace("Needs it:%d", doit);
        if (!doit)
            {
            return true;
            }

        String cmd = "dllwrap";
        cmd.append(" -o ");
        cmd.append(fullOut);
        if (defFileName.size()>0)
            {
            cmd.append(" --def ");
            cmd.append(defFileName);
            cmd.append(" ");
            }
        if (impFileName.size()>0)
            {
            cmd.append(" --implib ");
            cmd.append(impFileName);
            cmd.append(" ");
            }
        for (unsigned int i=0 ; i<fileSet.size() ; i++)
            {
            String fname;
            if (fileSetDir.size()>0)
                {
                fname.append(fileSetDir);
                fname.append("/");
                }
            fname.append(fileSet[i]);
            String fullName = parent.resolve(fname);

            cmd.append(" ");
            cmd.append(fullName);
            }
        cmd.append(" ");
        cmd.append(libs);

        String outString, errString;
        if (!executeCommand(cmd.c_str(), "", outString, errString))
            {
            error("<sharedlib> problem: %s", errString.c_str());
            return false;
            }

        return true;
        }

    virtual bool parse(Element *elem)
        {
        if (!parent.getAttribute(elem, "file", fileName))
            return false;
        if (!parent.getAttribute(elem, "import", impFileName))
            return false;
        if (!parent.getAttribute(elem, "def", defFileName))
            return false;
            
        std::vector<Element *> children = elem->getChildren();
        for (unsigned int i=0 ; i<children.size() ; i++)
            {
            Element *child = children[i];
            String tagName = child->getName();
            if (tagName == "fileset")
                {
                if (!parseFileSet(child, parent, fileSet))
                    return false;
                }
            else if (tagName == "libs")
                {
                if (!parent.getValue(child, libs))
                    return false;
                libs = strip(libs);
                }
            }
        return true;
        }

private:

    String command;
    String fileName;
    String defFileName;
    String impFileName;
    FileSet fileSet;
    String libs;

};


/**
 * Run the "ar" command to archive .o's into a .a
 */
class TaskStaticLib : public Task
{
public:

    TaskStaticLib(MakeBase &par) : Task(par)
        {
        type = TASK_STATICLIB; name = "staticlib";
        command = "ar crv";
        }

    virtual ~TaskStaticLib()
        {}

    virtual bool execute()
        {
        //trace("###########HERE %d", fileSet.size());
        bool doit = false;
        
        String fullOut = parent.resolve(fileName);
        //trace("ar fullout: %s", fullOut.c_str());
        
        if (!listFiles(parent, fileSet))
            return false;
        String fileSetDir = fileSet.getDirectory();

        for (unsigned int i=0 ; i<fileSet.size() ; i++)
            {
            String fname;
            if (fileSetDir.size()>0)
                {
                fname.append(fileSetDir);
                fname.append("/");
                }
            fname.append(fileSet[i]);
            String fullName = parent.resolve(fname);
            //trace("ar : %s/%s", fullOut.c_str(), fullName.c_str());
            if (isNewerThan(fullName, fullOut))
                doit = true;
            }
        //trace("Needs it:%d", doit);
        if (!doit)
            {
            return true;
            }

        String cmd = command;
        cmd.append(" ");
        cmd.append(fullOut);
        for (unsigned int i=0 ; i<fileSet.size() ; i++)
            {
            String fname;
            if (fileSetDir.size()>0)
                {
                fname.append(fileSetDir);
                fname.append("/");
                }
            fname.append(fileSet[i]);
            String fullName = parent.resolve(fname);

            cmd.append(" ");
            cmd.append(fullName);
            }

        String outString, errString;
        if (!executeCommand(cmd.c_str(), "", outString, errString))
            {
            error("<staticlib> problem: %s", errString.c_str());
            return false;
            }

        return true;
        }

    virtual bool parse(Element *elem)
        {
        String s;
        if (!parent.getAttribute(elem, "command", s))
            return false;
        if (s.size()>0)
            command = s;
        if (!parent.getAttribute(elem, "file", fileName))
            return false;
            
        std::vector<Element *> children = elem->getChildren();
        for (unsigned int i=0 ; i<children.size() ; i++)
            {
            Element *child = children[i];
            String tagName = child->getName();
            if (tagName == "fileset")
                {
                if (!parseFileSet(child, parent, fileSet))
                    return false;
                }
            }
        return true;
        }

private:

    String command;
    String fileName;
    FileSet fileSet;

};


/**
 * Strip an executable
 */
class TaskStrip : public Task
{
public:

    TaskStrip(MakeBase &par) : Task(par)
        { type = TASK_STRIP; name = "strip"; }

    virtual ~TaskStrip()
        {}

    virtual bool execute()
        {
        String fullName = parent.resolve(fileName);
        //trace("fullDir:%s", fullDir.c_str());
        String cmd;
        String outbuf, errbuf;

        if (symFileName.size()>0)
            {
            String symFullName = parent.resolve(symFileName);
            cmd = "objcopy --only-keep-debug ";
            cmd.append(getNativePath(fullName));
            cmd.append(" ");
            cmd.append(getNativePath(symFullName));
            if (!executeCommand(cmd, "", outbuf, errbuf))
                {
                error("<strip> symbol file failed : %s", errbuf.c_str());
                return false;
                }
            }
            
        cmd = "strip ";
        cmd.append(getNativePath(fullName));
        if (!executeCommand(cmd, "", outbuf, errbuf))
            {
            error("<strip> failed : %s", errbuf.c_str());
            return false;
            }
        return true;
        }

    virtual bool parse(Element *elem)
        {
        if (!parent.getAttribute(elem, "file", fileName))
            return false;
        if (!parent.getAttribute(elem, "symfile", symFileName))
            return false;
        if (fileName.size() == 0)
            {
            error("<strip> requires 'file=\"fileName\"' attribute");
            return false;
            }
        return true;
        }

private:

    String fileName;
    String symFileName;
};


/**
 *
 */
class TaskTstamp : public Task
{
public:

    TaskTstamp(MakeBase &par) : Task(par)
        { type = TASK_TSTAMP; name = "tstamp"; }

    virtual ~TaskTstamp()
        {}

    virtual bool execute()
        {
        return true;
        }

    virtual bool parse(Element *elem)
        {
        //trace("tstamp parse");
        return true;
        }
};



/**
 *
 */
Task *Task::createTask(Element *elem)
{
    String tagName = elem->getName();
    //trace("task:%s", tagName.c_str());
    Task *task = NULL;
    if (tagName == "cc")
        task = new TaskCC(parent);
    else if (tagName == "copy")
        task = new TaskCopy(parent);
    else if (tagName == "delete")
        task = new TaskDelete(parent);
    else if (tagName == "jar")
        task = new TaskJar(parent);
    else if (tagName == "javac")
        task = new TaskJavac(parent);
    else if (tagName == "link")
        task = new TaskLink(parent);
    else if (tagName == "makefile")
        task = new TaskMakeFile(parent);
    else if (tagName == "mkdir")
        task = new TaskMkDir(parent);
    else if (tagName == "msgfmt")
        task = new TaskMsgFmt(parent);
    else if (tagName == "ranlib")
        task = new TaskRanlib(parent);
    else if (tagName == "rc")
        task = new TaskRC(parent);
    else if (tagName == "sharedlib")
        task = new TaskSharedLib(parent);
    else if (tagName == "staticlib")
        task = new TaskStaticLib(parent);
    else if (tagName == "strip")
        task = new TaskStrip(parent);
    else if (tagName == "tstamp")
        task = new TaskTstamp(parent);
    else
        {
        error("Unknown task '%s'", tagName.c_str());
        return NULL;
        }

    if (!task->parse(elem))
        {
        delete task;
        return NULL;
        }
    return task;
}



//########################################################################
//# T A R G E T
//########################################################################

/**
 *
 */
class Target : public MakeBase
{

public:

    /**
     *
     */
    Target(Make &par) : parent(par)
        { init(); }

    /**
     *
     */
    Target(const Target &other) : parent(other.parent)
        { init(); assign(other); }

    /**
     *
     */
    Target &operator=(const Target &other)
        { init(); assign(other); return *this; }

    /**
     *
     */
    virtual ~Target()
        { cleanup() ; }


    /**
     *
     */
    virtual Make &getParent()
        { return parent; }

    /**
     *
     */
    virtual String getName()
        { return name; }

    /**
     *
     */
    virtual void setName(const String &val)
        { name = val; }

    /**
     *
     */
    virtual String getDescription()
        { return description; }

    /**
     *
     */
    virtual void setDescription(const String &val)
        { description = val; }

    /**
     *
     */
    virtual void addDependency(const String &val)
        { deps.push_back(val); }

    /**
     *
     */
    virtual void parseDependencies(const String &val)
        { deps = tokenize(val, ", "); }

    /**
     *
     */
    virtual std::vector<String> &getDependencies()
        { return deps; }

    /**
     *
     */
    virtual String getIf()
        { return ifVar; }

    /**
     *
     */
    virtual void setIf(const String &val)
        { ifVar = val; }

    /**
     *
     */
    virtual String getUnless()
        { return unlessVar; }

    /**
     *
     */
    virtual void setUnless(const String &val)
        { unlessVar = val; }

    /**
     *
     */
    virtual void addTask(Task *val)
        { tasks.push_back(val); }

    /**
     *
     */
    virtual std::vector<Task *> &getTasks()
        { return tasks; }

private:

    void init()
        {
        }

    void cleanup()
        {
        tasks.clear();
        }

    void assign(const Target &other)
        {
        //parent      = other.parent;
        name        = other.name;
        description = other.description;
        ifVar       = other.ifVar;
        unlessVar   = other.unlessVar;
        deps        = other.deps;
        tasks       = other.tasks;
        }

    Make &parent;

    String name;

    String description;

    String ifVar;

    String unlessVar;

    std::vector<String> deps;

    std::vector<Task *> tasks;

};








//########################################################################
//# M A K E
//########################################################################


/**
 *
 */
class Make : public MakeBase
{

public:

    /**
     *
     */
    Make()
        { init(); }

    /**
     *
     */
    Make(const Make &other)
        { assign(other); }

    /**
     *
     */
    Make &operator=(const Make &other)
        { assign(other); return *this; }

    /**
     *
     */
    virtual ~Make()
        { cleanup(); }

    /**
     *
     */
    virtual std::map<String, Target> &getTargets()
        { return targets; }


    /**
     *
     */
    virtual String version()
        { return BUILDTOOL_VERSION; }

    /**
     * Overload a <property>
     */
    virtual bool specifyProperty(const String &name,
                                 const String &value);

    /**
     *
     */
    virtual bool run();

    /**
     *
     */
    virtual bool run(const String &target);



private:

    /**
     *
     */
    void init();

    /**
     *
     */
    void cleanup();

    /**
     *
     */
    void assign(const Make &other);

    /**
     *
     */
    bool executeTask(Task &task);


    /**
     *
     */
    bool executeTarget(Target &target,
             std::set<String> &targetsCompleted);


    /**
     *
     */
    bool execute();

    /**
     *
     */
    bool checkTargetDependencies(Target &prop,
                    std::vector<String> &depList);

    /**
     *
     */
    bool parsePropertyFile(const String &fileName,
                           const String &prefix);

    /**
     *
     */
    bool parseProperty(Element *elem);

    /**
     *
     */
    bool parseTask(Task &task, Element *elem);

    /**
     *
     */
    bool parseFile();

    /**
     *
     */
    std::vector<String> glob(const String &pattern);


    //###############
    //# Fields
    //###############

    String projectName;

    String currentTarget;

    String defaultTarget;

    String specifiedTarget;

    String baseDir;

    String description;
    
    String envAlias;

    //std::vector<Property> properties;
    
    std::map<String, Target> targets;

    std::vector<Task *> allTasks;
    
    std::map<String, String> specifiedProperties;

};


//########################################################################
//# C L A S S  M A I N T E N A N C E
//########################################################################

/**
 *
 */
void Make::init()
{
    uri             = "build.xml";
    projectName     = "";
    currentTarget   = "";
    defaultTarget   = "";
    specifiedTarget = "";
    baseDir         = "";
    description     = "";
    envAlias        = "";
    properties.clear();
    for (unsigned int i = 0 ; i < allTasks.size() ; i++)
        delete allTasks[i];
    allTasks.clear();
}



/**
 *
 */
void Make::cleanup()
{
    for (unsigned int i = 0 ; i < allTasks.size() ; i++)
        delete allTasks[i];
    allTasks.clear();
}



/**
 *
 */
void Make::assign(const Make &other)
{
    uri              = other.uri;
    projectName      = other.projectName;
    currentTarget    = other.currentTarget;
    defaultTarget    = other.defaultTarget;
    specifiedTarget  = other.specifiedTarget;
    baseDir          = other.baseDir;
    description      = other.description;
    properties       = other.properties;
}



//########################################################################
//# U T I L I T Y    T A S K S
//########################################################################

/**
 *  Perform a file globbing
 */
std::vector<String> Make::glob(const String &pattern)
{
    std::vector<String> res;
    return res;
}


//########################################################################
//# P U B L I C    A P I
//########################################################################



/**
 *
 */
bool Make::executeTarget(Target &target,
             std::set<String> &targetsCompleted)
{

    String name = target.getName();

    //First get any dependencies for this target
    std::vector<String> deps = target.getDependencies();
    for (unsigned int i=0 ; i<deps.size() ; i++)
        {
        String dep = deps[i];
        //Did we do it already?  Skip
        if (targetsCompleted.find(dep)!=targetsCompleted.end())
            continue;
            
        std::map<String, Target> &tgts =
               target.getParent().getTargets();
        std::map<String, Target>::iterator iter =
               tgts.find(dep);
        if (iter == tgts.end())
            {
            error("Target '%s' dependency '%s' not found",
                      name.c_str(),  dep.c_str());
            return false;
            }
        Target depTarget = iter->second;
        if (!executeTarget(depTarget, targetsCompleted))
            {
            return false;
            }
        }

    status("## Target : %s", name.c_str());

    //Now let's do the tasks
    std::vector<Task *> &tasks = target.getTasks();
    for (unsigned int i=0 ; i<tasks.size() ; i++)
        {
        Task *task = tasks[i];
        status("---- task : %s", task->getName().c_str());
        if (!task->execute())
            {
            return false;
            }
        }
        
    targetsCompleted.insert(name);
    
    return true;
}



/**
 *  Main execute() method.  Start here and work
 *  up the dependency tree 
 */
bool Make::execute()
{
    status("######## EXECUTE");

    //Determine initial target
    if (specifiedTarget.size()>0)
        {
        currentTarget = specifiedTarget;
        }
    else if (defaultTarget.size()>0)
        {
        currentTarget = defaultTarget;
        }
    else
        {
        error("execute: no specified or default target requested");
        return false;
        }

    std::map<String, Target>::iterator iter =
               targets.find(currentTarget);
    if (iter == targets.end())
        {
        error("Initial target '%s' not found",
                 currentTarget.c_str());
        return false;
        }
        
    //Now run
    Target target = iter->second;
    std::set<String> targetsCompleted;
    if (!executeTarget(target, targetsCompleted))
        {
        return false;
        }

    status("######## EXECUTE COMPLETE");
    return true;
}




/**
 *
 */
bool Make::checkTargetDependencies(Target &target, 
                            std::vector<String> &depList)
{
    String tgtName = target.getName().c_str();
    depList.push_back(tgtName);

    std::vector<String> deps = target.getDependencies();
    for (unsigned int i=0 ; i<deps.size() ; i++)
        {
        String dep = deps[i];
        //First thing entered was the starting Target
        if (dep == depList[0])
            {
            error("Circular dependency '%s' found at '%s'",
                      dep.c_str(), tgtName.c_str());
            std::vector<String>::iterator diter;
            for (diter=depList.begin() ; diter!=depList.end() ; diter++)
                {
                error("  %s", diter->c_str());
                }
            return false;
            }

        std::map<String, Target> &tgts =
                  target.getParent().getTargets();
        std::map<String, Target>::iterator titer = tgts.find(dep);
        if (titer == tgts.end())
            {
            error("Target '%s' dependency '%s' not found",
                      tgtName.c_str(), dep.c_str());
            return false;
            }
        if (!checkTargetDependencies(titer->second, depList))
            {
            return false;
            }
        }
    return true;
}





static int getword(int pos, const String &inbuf, String &result)
{
    int p = pos;
    int len = (int)inbuf.size();
    String val;
    while (p < len)
        {
        char ch = inbuf[p];
        if (!isalnum(ch) && ch!='.' && ch!='_')
            break;
        val.push_back(ch);
        p++;
        }
    result = val;
    return p;
}




/**
 *
 */
bool Make::parsePropertyFile(const String &fileName,
                             const String &prefix)
{
    FILE *f = fopen(fileName.c_str(), "r");
    if (!f)
        {
        error("could not open property file %s", fileName.c_str());
        return false;
        }
    int linenr = 0;
    while (!feof(f))
        {
        char buf[256];
        if (!fgets(buf, 255, f))
            break;
        linenr++;
        String s = buf;
        s = trim(s);
        int len = s.size();
        if (len == 0)
            continue;
        if (s[0] == '#')
            continue;
        String key;
        String val;
        int p = 0;
        int p2 = getword(p, s, key);
        if (p2 <= p)
            {
            error("property file %s, line %d: expected keyword",
                    fileName.c_str(), linenr);
            return false;
            }
        if (prefix.size() > 0)
            {
            key.insert(0, prefix);
            }

        //skip whitespace
        for (p=p2 ; p<len ; p++)
            if (!isspace(s[p]))
                break;

        if (p>=len || s[p]!='=')
            {
            error("property file %s, line %d: expected '='",
                    fileName.c_str(), linenr);
            return false;
            }
        p++;

        //skip whitespace
        for ( ; p<len ; p++)
            if (!isspace(s[p]))
                break;

        /* This way expects a word after the =
        p2 = getword(p, s, val);
        if (p2 <= p)
            {
            error("property file %s, line %d: expected value",
                    fileName.c_str(), linenr);
            return false;
            }
        */
        // This way gets the rest of the line after the =
        if (p>=len)
            {
            error("property file %s, line %d: expected value",
                    fileName.c_str(), linenr);
            return false;
            }
        val = s.substr(p);
        if (key.size()==0)
            continue;
        //allow property to be set, even if val=""

        //trace("key:'%s' val:'%s'", key.c_str(), val.c_str());
        //See if we wanted to overload this property
        std::map<String, String>::iterator iter =
            specifiedProperties.find(key);
        if (iter!=specifiedProperties.end())
            {
            val = iter->second;
            status("overloading property '%s' = '%s'",
                   key.c_str(), val.c_str());
            }
        properties[key] = val;
        }
    fclose(f);
    return true;
}




/**
 *
 */
bool Make::parseProperty(Element *elem)
{
    std::vector<Attribute> &attrs = elem->getAttributes();
    for (unsigned int i=0 ; i<attrs.size() ; i++)
        {
        String attrName = attrs[i].getName();
        String attrVal  = attrs[i].getValue();

        if (attrName == "name")
            {
            String val;
            if (!getAttribute(elem, "value", val))
                return false;
            if (val.size() > 0)
                {
                properties[attrVal] = val;
                }
            else
                {
                if (!getAttribute(elem, "location", val))
                    return false;
                //let the property exist, even if not defined
                properties[attrVal] = val;
                }
            //See if we wanted to overload this property
            std::map<String, String>::iterator iter =
                specifiedProperties.find(attrVal);
            if (iter != specifiedProperties.end())
                {
                val = iter->second;
                status("overloading property '%s' = '%s'",
                    attrVal.c_str(), val.c_str());
                properties[attrVal] = val;
                }
            }
        else if (attrName == "file")
            {
            String prefix;
            if (!getAttribute(elem, "prefix", prefix))
                return false;
            if (prefix.size() > 0)
                {
                if (prefix[prefix.size()-1] != '.')
                    prefix.push_back('.');
                }
            if (!parsePropertyFile(attrName, prefix))
                return false;
            }
        else if (attrName == "environment")
            {
            if (envAlias.size() > 0)
                {
                error("environment property can only be set once");
                return false;
                }
            envAlias = attrVal;
            }
        }

    return true;
}




/**
 *
 */
bool Make::parseFile()
{
    status("######## PARSE : %s", uri.getPath().c_str());

    Parser parser;
    Element *root = parser.parseFile(uri.getNativePath());
    if (!root)
        {
        error("Could not open %s for reading",
              uri.getNativePath().c_str());
        return false;
        }

    if (root->getChildren().size()==0 ||
        root->getChildren()[0]->getName()!="project")
        {
        error("Main xml element should be <project>");
        delete root;
        return false;
        }

    //########## Project attributes
    Element *project = root->getChildren()[0];
    String s = project->getAttribute("name");
    if (s.size() > 0)
        projectName = s;
    s = project->getAttribute("default");
    if (s.size() > 0)
        defaultTarget = s;
    s = project->getAttribute("basedir");
    if (s.size() > 0)
        baseDir = s;

    //######### PARSE MEMBERS
    std::vector<Element *> children = project->getChildren();
    for (unsigned int i=0 ; i<children.size() ; i++)
        {
        Element *elem = children[i];
        String tagName = elem->getName();

        //########## DESCRIPTION
        if (tagName == "description")
            {
            description = parser.trim(elem->getValue());
            }

        //######### PROPERTY
        else if (tagName == "property")
            {
            if (!parseProperty(elem))
                return false;
            }

        //######### TARGET
        else if (tagName == "target")
            {
            String tname   = elem->getAttribute("name");
            String tdesc   = elem->getAttribute("description");
            String tdeps   = elem->getAttribute("depends");
            String tif     = elem->getAttribute("if");
            String tunless = elem->getAttribute("unless");
            Target target(*this);
            target.setName(tname);
            target.setDescription(tdesc);
            target.parseDependencies(tdeps);
            target.setIf(tif);
            target.setUnless(tunless);
            std::vector<Element *> telems = elem->getChildren();
            for (unsigned int i=0 ; i<telems.size() ; i++)
                {
                Element *telem = telems[i];
                Task breeder(*this);
                Task *task = breeder.createTask(telem);
                if (!task)
                    return false;
                allTasks.push_back(task);
                target.addTask(task);
                }

            //Check name
            if (tname.size() == 0)
                {
                error("no name for target");
                return false;
                }
            //Check for duplicate name
            if (targets.find(tname) != targets.end())
                {
                error("target '%s' already defined", tname.c_str());
                return false;
                }
            //more work than targets[tname]=target, but avoids default allocator
            targets.insert(std::make_pair<String, Target>(tname, target));
            }

        }

    std::map<String, Target>::iterator iter;
    for (iter = targets.begin() ; iter!= targets.end() ; iter++)
        {
        Target tgt = iter->second;
        std::vector<String> depList;
        if (!checkTargetDependencies(tgt, depList))
            {
            return false;
            }
        }


    delete root;
    status("######## PARSE COMPLETE");
    return true;
}


/**
 * Overload a <property>
 */
bool Make::specifyProperty(const String &name, const String &value)
{
    if (specifiedProperties.find(name) != specifiedProperties.end())
        {
        error("Property %s already specified", name.c_str());
        return false;
        }
    specifiedProperties[name] = value;
    return true;
}



/**
 *
 */
bool Make::run()
{
    if (!parseFile())
        return false;
        
    if (!execute())
        return false;

    return true;
}




/**
 * Get a formatted MM:SS.sss time elapsed string
 */ 
static String
timeDiffString(struct timeval &x, struct timeval &y)
{
    long microsX  = x.tv_usec;
    long secondsX = x.tv_sec;
    long microsY  = y.tv_usec;
    long secondsY = y.tv_sec;
    if (microsX < microsY)
        {
        microsX += 1000000;
        secondsX -= 1;
        }

    int seconds = (int)(secondsX - secondsY);
    int millis  = (int)((microsX - microsY)/1000);

    int minutes = seconds/60;
    seconds -= minutes*60;
    char buf[80];
    snprintf(buf, 79, "%dm %d.%03ds", minutes, seconds, millis);
    String ret = buf;
    return ret;
    
}

/**
 *
 */
bool Make::run(const String &target)
{
    status("####################################################");
    status("#   %s", version().c_str());
    status("####################################################");
    struct timeval timeStart, timeEnd;
    ::gettimeofday(&timeStart, NULL);
    specifiedTarget = target;
    if (!run())
        return false;
    ::gettimeofday(&timeEnd, NULL);
    String timeStr = timeDiffString(timeEnd, timeStart);
    status("####################################################");
    status("#   BuildTool Completed : %s", timeStr.c_str());
    status("####################################################");
    return true;
}







}// namespace buildtool
//########################################################################
//# M A I N
//########################################################################

typedef buildtool::String String;

/**
 *  Format an error message in printf() style
 */
static void error(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "BuildTool error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}


static bool parseProperty(const String &s, String &name, String &val)
{
    int len = s.size();
    int i;
    for (i=0 ; i<len ; i++)
        {
        char ch = s[i];
        if (ch == '=')
            break;
        name.push_back(ch);
        }
    if (i>=len || s[i]!='=')
        {
        error("property requires -Dname=value");
        return false;
        }
    i++;
    for ( ; i<len ; i++)
        {
        char ch = s[i];
        val.push_back(ch);
        }
    return true;
}


/**
 * Compare a buffer with a key, for the length of the key
 */
static bool sequ(const String &buf, char *key)
{
    int len = buf.size();
    for (int i=0 ; key[i] && i<len ; i++)
        {
        if (key[i] != buf[i])
            return false;
        }        
    return true;
}

static void usage(int argc, char **argv)
{
    printf("usage:\n");
    printf("   %s [options] [target]\n", argv[0]);
    printf("Options:\n");
    printf("  -help, -h              print this message\n");
    printf("  -version               print the version information and exit\n");
    printf("  -file <file>           use given buildfile\n");
    printf("  -f <file>                 ''\n");
    printf("  -D<property>=<value>   use value for given property\n");
}




/**
 * Parse the command-line args, get our options,
 * and run this thing
 */   
static bool parseOptions(int argc, char **argv)
{
    if (argc < 1)
        {
        error("Cannot parse arguments");
        return false;
        }

    buildtool::Make make;

    String target;

    //char *progName = argv[0];
    for (int i=1 ; i<argc ; i++)
        {
        String arg = argv[i];
        if (arg.size()>1 && arg[0]=='-')
            {
            if (arg == "-h" || arg == "-help")
                {
                usage(argc,argv);
                return true;
                }
            else if (arg == "-version")
                {
                printf("%s", make.version().c_str());
                return true;
                }
            else if (arg == "-f" || arg == "-file")
                {
                if (i>=argc)
                   {
                   usage(argc, argv);
                   return false;
                   }
                i++; //eat option
                make.setURI(argv[i]);
                }
            else if (arg.size()>2 && sequ(arg, "-D"))
                {
                String s = arg.substr(2, s.size());
                String name, value;
                if (!parseProperty(s, name, value))
                   {
                   usage(argc, argv);
                   return false;
                   }
                if (!make.specifyProperty(name, value))
                    return false;
                }
            else
                {
                error("Unknown option:%s", arg.c_str());
                return false;
                }
            }
        else
            {
            if (target.size()>0)
                {
                error("only one initial target");
                usage(argc, argv);
                return false;
                }
            target = arg;
            }
        }

    //We have the options.  Now execute them
    if (!make.run(target))
        return false;

    return true;
}




/*
static bool runMake()
{
    buildtool::Make make;
    if (!make.run())
        return false;
    return true;
}


static bool pkgConfigTest()
{
    buildtool::PkgConfig pkgConfig;
    if (!pkgConfig.readFile("gtk+-2.0.pc"))
        return false;
    return true;
}



static bool depTest()
{
    buildtool::DepTool deptool;
    deptool.setSourceDirectory("/dev/ink/inkscape/src");
    if (!deptool.generateDependencies("build.dep"))
        return false;
    std::vector<buildtool::DepRec> res =
           deptool.loadDepFile("build.dep");
    if (res.size() == 0)
        return false;
    return true;
}

static bool popenTest()
{
    buildtool::Make make;
    buildtool::String out, err;
    bool ret = make.executeCommand("gcc xx.cpp", "", out, err);
    printf("Popen test:%d '%s' '%s'\n", ret, out.c_str(), err.c_str());
    return true;
}


static bool propFileTest()
{
    buildtool::Make make;
    make.parsePropertyFile("test.prop", "test.");
    return true;
}
*/

int main(int argc, char **argv)
{

    if (!parseOptions(argc, argv))
        return 1;
    /*
    if (!popenTest())
        return 1;

    if (!depTest())
        return 1;
    if (!propFileTest())
        return 1;
    if (runMake())
        return 1;
    */
    return 0;
}


//########################################################################
//# E N D 
//########################################################################


