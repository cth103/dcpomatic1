/*
 * $Id: auxtrans_manager.c 1269 2011-12-23 08:29:09Z mathieu.malaterre $
 *
 * Copyright (c) 2002-2011, Communications and Remote Sensing Laboratory, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2011, Professor Benoit Macq
 * Copyright (c) 2010-2011, Kaori Hagihara
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "auxtrans_manager.h"

#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#endif

#ifdef SERVER
#include "fcgi_stdio.h"
#define logstream FCGI_stdout
#else
#define FCGI_stdout stdout
#define FCGI_stderr stderr
#define logstream stderr
#endif //SERVER

auxtrans_param_t init_aux_transport( int tcp_auxport, int udp_auxport)
{
  auxtrans_param_t auxtrans;

  auxtrans.tcpauxport = tcp_auxport;
  auxtrans.udpauxport = udp_auxport;

  if( 49152 <= tcp_auxport && tcp_auxport <= 65535)
    auxtrans.tcplistensock = open_listeningsocket( tcp_auxport);
  else
    auxtrans.tcplistensock = -1;

  auxtrans.udplistensock = -1;
  // open listening socket for udp later

  return auxtrans;
}

void close_aux_transport( auxtrans_param_t auxtrans)
{
  if( auxtrans.tcplistensock != -1)
    if( close_socket( auxtrans.tcplistensock) != 0)
      perror("close");

  if( auxtrans.udplistensock != -1)
    if( close_socket( auxtrans.udplistensock) != 0)
      perror("close");
}


//!< auxiliary response parameters
typedef struct aux_response_param{
  char *cid;            //!< channel ID
  unsigned char *data;  //!< sending data
  int datalen;          //!< length of data
  int maxlenPerFrame;   //!< maximum data length to send per frame
  SOCKET listensock;    //!< listeing socket
#ifdef _WIN32
  HANDLE hTh;           //!< thread handle
#endif
} aux_response_param_t;

aux_response_param_t * gene_auxresponse( bool istcp, auxtrans_param_t auxtrans, char cid[], void *data, int datalen, int maxlenPerFrame);

void delete_auxresponse( aux_response_param_t **auxresponse);


#ifdef _WIN32
unsigned __stdcall aux_streaming( void *arg);
#else
void * aux_streaming( void *arg);
#endif

void send_responsedata_on_aux( bool istcp, auxtrans_param_t auxtrans, char cid[], void *data, int datalen, int maxlenPerFrame)
{
  aux_response_param_t *auxresponse;
#ifdef _WIN32
  unsigned int threadId;
#else
  pthread_t thread;
  int status;
#endif
  
  if( istcp){
    if( auxtrans.tcplistensock == -1){
      fprintf( FCGI_stderr, "Error: error in send_responsedata_on_aux(), tcp listening socket no open\n");
      return;
    }

    auxresponse = gene_auxresponse( istcp, auxtrans, cid, data, datalen, maxlenPerFrame);

#ifdef _WIN32
    auxresponse->hTh = (HANDLE)_beginthreadex( NULL, 0, &aux_streaming, auxresponse, 0, &threadId);
    if( auxresponse->hTh == 0)
      fprintf( FCGI_stderr,"ERRO: pthread_create() %s", strerror( (int)auxresponse->hTh));
#else
    status = pthread_create( &thread, NULL, &aux_streaming, auxresponse);
    if( status != 0)
      fprintf( FCGI_stderr,"ERROR: pthread_create() %s",strerror(status));
#endif   
  }
  else
    fprintf( FCGI_stderr, "Error: error in send_responsedata_on_aux(), udp not implemented\n");
}

aux_response_param_t * gene_auxresponse( bool istcp, auxtrans_param_t auxtrans, char cid[], void *data, int datalen, int maxlenPerFrame)
{
  aux_response_param_t *auxresponse;

  auxresponse = (aux_response_param_t *)malloc( sizeof(aux_response_param_t));

  auxresponse->cid = strdup( cid);
  auxresponse->data = data;
  auxresponse->datalen = datalen;
  auxresponse->maxlenPerFrame = maxlenPerFrame;
  auxresponse->listensock = istcp ? auxtrans.tcplistensock : auxtrans.udplistensock;

  return auxresponse;
}

void delete_auxresponse( aux_response_param_t **auxresponse)
{
  free( (*auxresponse)->cid);
  free( (*auxresponse)->data);
  free( *auxresponse);
}

/**
 * Identify cid sent from client
 *
 * @param [in] connected_socket file descriptor of the connected socket
 * @param [in] refcid           refenrece channel ID
 * @param [in] fp               file pointer for log of aux stream
 * @return                      true if identified, false otherwise
 */
bool identify_cid( SOCKET connected_socket, char refcid[], FILE *fp);

bool recv_ack( SOCKET connected_socket, void *data);

#ifdef _WIN32
unsigned __stdcall aux_streaming( void *arg)
#else
void * aux_streaming( void *arg)
#endif
{
  SOCKET connected_socket;
  unsigned char *chunk, *ptr;
  int maxLenOfBody, remlen, chunklen;
  const int headlen = 8;
  
  aux_response_param_t *auxresponse = (aux_response_param_t *)arg;

#ifdef _WIN32
  CloseHandle( auxresponse->hTh);
#else
  pthread_detach( pthread_self());
#endif

  chunk = (unsigned char *)malloc( auxresponse->maxlenPerFrame);
  maxLenOfBody = auxresponse->maxlenPerFrame - headlen;
  remlen = auxresponse->datalen;

  while((connected_socket = accept_socket( auxresponse->listensock)) != -1){
    if( identify_cid( connected_socket, auxresponse->cid, FCGI_stderr)){
      ptr = auxresponse->data;
      while( 0 < remlen){
	memset( chunk, 0, auxresponse->maxlenPerFrame);

	chunklen = remlen<maxLenOfBody?remlen:maxLenOfBody;
	chunklen += headlen;
	
	chunk[0] = (chunklen >> 8) & 0xff;
	chunk[1] = chunklen & 0xff;

	memcpy( chunk+headlen, ptr, chunklen-headlen);

	do{
	  send_stream( connected_socket, chunk, chunklen);
	}while( !recv_ack( connected_socket, chunk));

	remlen -= maxLenOfBody;
	ptr += maxLenOfBody;
      }
      if( close_socket( connected_socket) != 0)
	perror("close");
      break;
    }
  }
  free( chunk);

  delete_auxresponse( &auxresponse);
  
#ifdef _WIN32
  _endthreadex(0);
#else
  pthread_exit(0);
#endif

  return 0;
}


bool identify_cid( SOCKET connected_socket, char refcid[], FILE *fp)
{
  char *cid;
  bool succeed;

  if(!(cid = receive_string( connected_socket))){
    fprintf( fp, "Error: error in identify_cid(), while receiving cid from client\n");
    return false;
  }
  
  succeed = false;
  if( strncmp( refcid, cid, strlen( refcid)) == 0)
    succeed = true;
  
  free( cid);

  return succeed;
}

bool recv_ack( SOCKET connected_socket, void *data)
{
  char *header;
  bool succeed;
  
  header = receive_stream( connected_socket, 8);
  
  if( memcmp( header, data, 8) != 0)
    succeed = false;
  else
    succeed = true;
  
  free( header);

  return succeed;
}
