/*
 * $Id: openjpip.c 1364 2012-01-30 09:55:05Z mathieu.malaterre $
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

#include <stdlib.h>
#include "openjpip.h"
#include "jpip_parser.h"
#include "channel_manager.h"
#include "byte_manager.h"
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef SERVER
#include "auxtrans_manager.h"
#endif

#include <stdio.h>
#include "dec_clientmsg_handler.h"
#include "jpipstream_manager.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "jp2k_encoder.h"

#ifdef SERVER

server_record_t * init_JPIPserver( int tcp_auxport, int udp_auxport)
{
  server_record_t *record = (server_record_t *)malloc( sizeof(server_record_t));
  
  record->sessionlist = gene_sessionlist();
  record->targetlist  = gene_targetlist();
  record->auxtrans = init_aux_transport( tcp_auxport, udp_auxport);
   
  return record;
}

void terminate_JPIPserver( server_record_t **rec)
{
  delete_sessionlist( &(*rec)->sessionlist);
  delete_targetlist( &(*rec)->targetlist); 
  close_aux_transport( (*rec)->auxtrans);
   
  free( *rec);
}

QR_t * parse_querystring( char *query_string)
{
  QR_t *qr;

  qr = (QR_t *)malloc( sizeof(QR_t));
    
  qr->query = parse_query( query_string);
  qr->msgqueue = NULL;
  qr->channel = NULL;

  return qr;
}

bool process_JPIPrequest( server_record_t *rec, QR_t *qr)
{
  target_param_t *target = NULL;
  session_param_t *cursession = NULL;
  channel_param_t *curchannel = NULL;

  if( qr->query->target || qr->query->tid){
    if( !identify_target( *(qr->query), rec->targetlist, &target))
      return false;
  }

  if( qr->query->cid){
    if( !associate_channel( *(qr->query), rec->sessionlist, &cursession, &curchannel))
      return false;
    qr->channel = curchannel;
  }
  
  if( qr->query->cnew != non){
    if( !open_channel( *(qr->query), rec->sessionlist, rec->auxtrans, target, &cursession, &curchannel))
      return false;
    qr->channel = curchannel;
  }
  
  if( qr->query->cclose)
    if( !close_channel( *(qr->query), rec->sessionlist, &cursession, &curchannel))
      return false;
  
  if( (qr->query->fx > 0 && qr->query->fy > 0) || qr->query->box_type[0][0] != 0)
    if( !gene_JPIPstream( *(qr->query), target, cursession, curchannel, &qr->msgqueue))
      return false;

  return true;
}

void add_EORmsg( int fd, QR_t *qr);

void send_responsedata( server_record_t *rec, QR_t *qr)
{
  int fd;
  char tmpfname[] = "tmpjpipstream.jpp";
  Byte_t *jpipstream;
  Byte8_t len_of_jpipstream;

  if( (fd = open( tmpfname, O_RDWR|O_CREAT|O_EXCL, S_IRWXU)) == -1){
    fprintf( FCGI_stderr, "file open error %s", tmpfname);
    fprintf( FCGI_stdout, "Status: 503\r\n");
    fprintf( FCGI_stdout, "Reason: Implementation failed\r\n");
    return;
  }
  
  recons_stream_from_msgqueue( qr->msgqueue, fd);
  
  add_EORmsg( fd, qr); /* needed at least for tcp and udp */

  len_of_jpipstream = get_filesize( fd);
  jpipstream = fetch_bytes( fd, 0, len_of_jpipstream);

  close( fd);
  remove( tmpfname);

  fprintf( FCGI_stdout, "\r\n");

  if( qr->channel)
    if( qr->channel->aux == tcp || qr->channel->aux == udp){
      send_responsedata_on_aux( qr->channel->aux==tcp, rec->auxtrans, qr->channel->cid, jpipstream, len_of_jpipstream, 1000); /* 1KB per frame*/
      return;
    }
  
  if( fwrite( jpipstream, len_of_jpipstream, 1, FCGI_stdout) != 1)
    fprintf( FCGI_stderr, "Error: failed to write jpipstream\n");

  free( jpipstream);
  return;
}

void add_EORmsg( int fd, QR_t *qr)
{
  unsigned char EOR[3];

  if( qr->channel){
    EOR[0] = 0x00;   
    EOR[1] = is_allsent( *(qr->channel->cachemodel)) ? 0x01 : 0x02;
    EOR[2] = 0x00;
    if( write( fd, EOR, 3) != 3)
      fprintf( FCGI_stderr, "Error: failed to write EOR message\n");
  }
}

void end_QRprocess( server_record_t *rec, QR_t **qr)
{
  /* TODO: record client preferences if necessary*/
  
  delete_query( &((*qr)->query));
  delete_msgqueue( &((*qr)->msgqueue));
  free( *qr);
}


void local_log( bool query, bool messages, bool sessions, bool targets, QR_t *qr, server_record_t *rec)
{
  if( query)
    print_queryparam( *qr->query);

  if( messages)
    print_msgqueue( qr->msgqueue);

  if( sessions)
    print_allsession( rec->sessionlist);
  
  if( targets)
    print_alltarget( rec->targetlist);
}

#endif /*SERVER*/

#ifndef SERVER

dec_server_record_t * init_dec_server( int port)
{
  dec_server_record_t *record = (dec_server_record_t *)malloc( sizeof(dec_server_record_t));

  record->cachelist = gene_cachelist();
  record->jpipstream = NULL;
  record->jpipstreamlen = 0;
  record->msgqueue = gene_msgqueue( true, NULL);
  record->listening_socket = open_listeningsocket( port);

  return record;
}

void terminate_dec_server( dec_server_record_t **rec)
{
  delete_cachelist( &(*rec)->cachelist);  
  free( (*rec)->jpipstream);
  
  if( (*rec)->msgqueue)
    delete_msgqueue( &((*rec)->msgqueue));

  if( close_socket( (*rec)->listening_socket) != 0)
    perror("close");
  
  free( *rec);
}

client_t accept_connection( dec_server_record_t *rec)
{
  client_t client;
  
  client = accept_socket( rec->listening_socket);
  if( client == -1)
    fprintf( stderr, "error: failed to connect to client\n");
  
  return client;
}

bool handle_clientreq( client_t client, dec_server_record_t *rec)
{
  bool quit = false;
  msgtype_t msgtype = identify_clientmsg( client);
  
  switch( msgtype){
  case JPIPSTREAM:
    handle_JPIPstreamMSG( client, rec->cachelist, &rec->jpipstream, &rec->jpipstreamlen, rec->msgqueue);
    break;
      
  case PNMREQ:
    handle_PNMreqMSG( client, rec->jpipstream, rec->msgqueue, rec->cachelist);
    break;
    
  case XMLREQ:
    handle_XMLreqMSG( client, rec->jpipstream, rec->cachelist);
    break;

  case TIDREQ:
    handle_TIDreqMSG( client, rec->cachelist);
    break;
						
  case CIDREQ:
    handle_CIDreqMSG( client, rec->cachelist);
    break;

  case CIDDST:
    handle_dstCIDreqMSG( client, rec->cachelist);
    break;
    
  case SIZREQ:
    handle_SIZreqMSG( client, rec->jpipstream, rec->msgqueue, rec->cachelist);
    break;

  case JP2SAVE:
    handle_JP2saveMSG( client, rec->cachelist, rec->msgqueue, rec->jpipstream);
    break;

  case QUIT:
    quit = true;
    save_codestream( rec->jpipstream, rec->jpipstreamlen, "jpt");
    break;
  case MSGERROR:
    break;
  }

  fprintf( stderr, "\t end of the connection\n\n");
  if( close_socket(client) != 0){
    perror("close");
    return false;
  }

  if( quit)
    return false;

  return true;
}


jpip_dec_param_t * init_jpipdecoder( bool jp2)
{
  jpip_dec_param_t *dec;
  
  dec = (jpip_dec_param_t *)calloc( 1, sizeof(jpip_dec_param_t));

  dec->msgqueue = gene_msgqueue( true, NULL);
  
  if( jp2)
    dec->metadatalist = gene_metadatalist();

  return dec;
}


bool fread_jpip( char fname[], jpip_dec_param_t *dec)
{
  int infd;

  if(( infd = open( fname, O_RDONLY)) == -1){
    fprintf( stderr, "file %s not exist\n", fname);
    return false;
  }
  
  if(!(dec->jpiplen = get_filesize(infd)))
    return false;
  
  dec->jpipstream = (Byte_t *)malloc( dec->jpiplen);

  if( read( infd, dec->jpipstream, dec->jpiplen) != (int)dec->jpiplen){
    fprintf( stderr, "file reading error\n");
    free( dec->jpipstream);
    return false;
  }

  close(infd);

  return true;
}

void decode_jpip( jpip_dec_param_t *dec)
{
  parse_JPIPstream( dec->jpipstream, dec->jpiplen, 0, dec->msgqueue);

  if( dec->metadatalist){ /* JP2 encoding*/
    parse_metamsg( dec->msgqueue, dec->jpipstream, dec->jpiplen, dec->metadatalist);
    dec->ihdrbox = gene_ihdrbox( dec->metadatalist, dec->jpipstream);
    
    dec->jp2kstream = recons_jp2( dec->msgqueue, dec->jpipstream, dec->msgqueue->first->csn, &dec->jp2klen);
  }
  else /* J2k encoding  */
    /* Notice: arguments fw, fh need to be set for LRCP, PCRL, CPRL*/
    dec->jp2kstream = recons_j2k( dec->msgqueue, dec->jpipstream, dec->msgqueue->first->csn, 0, 0, &dec->jp2klen);  
}

bool fwrite_jp2k( char fname[], jpip_dec_param_t *dec)
{
  int outfd;
  
#ifdef _WIN32
  if(( outfd = open( fname, O_WRONLY|O_CREAT, _S_IREAD | _S_IWRITE)) == -1){
#else
 if(( outfd = open( fname, O_WRONLY|O_CREAT, S_IRWXU|S_IRWXG)) == -1){
#endif
   fprintf( stderr, "file %s open error\n", fname);
   return false;
 }
  
 if( write( outfd, dec->jp2kstream, dec->jp2klen) != (int)dec->jp2klen)
   fprintf( stderr, "j2k file write error\n");

 close(outfd);

 return true;
}

void output_log( bool messages, bool metadata, bool ihdrbox, jpip_dec_param_t *dec)
{
  if( messages)
    print_msgqueue( dec->msgqueue);

  if( metadata)
    print_allmetadata( dec->metadatalist);

  if( ihdrbox){
    printf("W*H: %d*%d\n", dec->ihdrbox->height, dec->ihdrbox->width);
    printf("NC: %d, bpc: %d\n", dec->ihdrbox->nc, dec->ihdrbox->bpc);
  }
}

void destroy_jpipdecoder( jpip_dec_param_t **dec)
{
  free( (*dec)->jpipstream);
  delete_msgqueue( &(*dec)->msgqueue);
  if( (*dec)->metadatalist){
    delete_metadatalist( &(*dec)->metadatalist);
    free( (*dec)->ihdrbox);
  }

  free( (*dec)->jp2kstream);
  free( *dec);
}

index_t * get_index_from_JP2file( int fd)
{
  char *data;
 
  /* Check resource is a JP family file.*/
  if( lseek( fd, 0, SEEK_SET)==-1){
    fprintf( stderr, "Error: File broken (lseek error)\n");
    return NULL;
  }
  
  data = (char *)malloc( 12); /* size of header*/
  if( read( fd, data, 12) != 12){
    free( data);
    fprintf( stderr, "Error: File broken (read error)\n");
    return NULL;
  }
    
  if( *data || *(data + 1) || *(data + 2) ||
      *(data + 3) != 12 || strncmp (data + 4, "jP  \r\n\x87\n", 8)){
    free( data);
    fprintf( stderr, "Error: No JPEG 2000 Signature box in this file\n");
    return NULL;
  }
  free( data);
  
  return parse_jp2file( fd);
}

void destroy_index( index_t **idx)
{
  delete_index( idx);
}

void output_index( index_t *index)
{
  print_index( *index);
}

#endif /*SERVER*/
