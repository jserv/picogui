/* $Id$
 * 
 * wt.c - Loading and instantiation of PicoGUI's Widget Templates
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/handle.h>
#include <pgserver/widget.h>
#include <pgserver/svrwt.h>
#include <pgserver/requests.h>
#include <picogui/wt.h>

g_error wt_load(handle *wt, int owner, const u8 *template, u32 template_len) {
  struct pgwt_header *hdr;
  u32 file_crc32, valid_crc32;
  struct pgmemwt *memwt;
  g_error e;

  /* Get the WT header */
  if (template_len < sizeof(struct pgwt_header))
    return mkerror(PG_ERRT_FILEFMT,111);         /* No header in WT */
  hdr = (struct pgwt_header *) template;

  /* Validate magic number */
  if (hdr->magic[0] != 'P' ||
      hdr->magic[1] != 'G' ||
      hdr->magic[2] != 'w' ||
      hdr->magic[3] != 't')
    return mkerror(PG_ERRT_FILEFMT,112);    /* Bad magic number */

  if (hdr->file_len) {
    /* Zero out the checksum field, and validate the file */
    file_crc32 = ntohl(hdr->file_crc32);
    hdr->file_crc32 = 0;
    valid_crc32 = crc32(crc32(0,NULL,0),template,template_len);
    if (file_crc32 != valid_crc32)
      return mkerror(PG_ERRT_FILEFMT,113);    /* Bad checksum */
  }    

  /* Now convert byte order in the header */
  hdr->file_len     = ntohl(hdr->file_len); 
  hdr->file_ver     = ntohs(hdr->file_ver);
  hdr->num_global   = ntohs(hdr->num_global); 
  hdr->num_instance = ntohs(hdr->num_instance);
  hdr->num_handles  = ntohs(hdr->num_handles);

  /* Validate length */
  if (hdr->file_len && hdr->file_len != template_len)
    return mkerror(PG_ERRT_FILEFMT,114);    /* Length mismatch */

  /* Now skip past the header */
  template += sizeof(struct pgwt_header);
  template_len -= sizeof(struct pgwt_header);

  /* Allocate the widget template and handle table */
  e = g_malloc((void**) &memwt, sizeof(struct pgmemwt));
  errorcheck;
  if (hdr->num_handles) {
    e = g_malloc((void**) &memwt->htable, sizeof(handle) * hdr->num_handles);
    if (iserror(e)) {
      g_free(memwt);
      return e;
    }
  }
  else
    memwt->htable = NULL;
  memset(memwt->htable,0,sizeof(handle) * hdr->num_handles);
  memwt->num_handles = hdr->num_handles;
  memwt->num_instance = hdr->num_instance;
  memwt->requests = NULL;

  /* Allocate us a handle now */
  e = mkhandle(wt,PG_TYPE_WT, owner, memwt);
  errorcheck;

  /* Now run num_global requests */
  e = wt_run_requests(*wt, owner, &template, &template_len, memwt->htable,
		      memwt->num_handles, hdr->num_global);
  if (iserror(e)) {
    g_free(memwt->htable);
    g_free(memwt);
    return e;
  }

  /* Point the loaded widget template at the per-instance requests
   */
  e = g_malloc((void**) &memwt->requests, template_len);
  if (iserror(e)) {
    handle_free(owner,*wt);
    return e;
  }
  memcpy(memwt->requests,template,template_len);
  memwt->requests_length = template_len;

  return success;
}

g_error wt_instantiate(handle *instance, struct pgmemwt *wt, handle wt_h, int owner) {
  int requests_len;
  const u8 *requests;
  g_error e;

  /* This is just a matter of running the per-instance requests.
   * The handle table is used as-is. Most widget templates will define extra scratch
   * space for per-instance requests, but this could be used to do some interesting
   * linking between instances i suppose.
   */
  
  requests = wt->requests;
  requests_len = wt->requests_length;
  
  e = wt_run_requests(wt_h, owner, &requests, &requests_len, 
		      wt->htable,wt->num_handles,wt->num_instance);
  errorcheck;

  /* If possible, return the first handle in the table as our result */
  if (wt->num_handles > 0)
    *instance = wt->htable[0];
  else
    *instance = 0;

  return success;
}

void wt_free(struct pgmemwt *wt) {
  if (wt->htable)
    g_free(wt->htable);
  if (wt->requests)
    g_free(wt->requests);
  g_free(wt);
}

g_error wt_run_requests(handle group, int owner, const u8 **requests, int *requests_len, 
			handle *htable, int num_handles, int num_requests) {
  struct pgrequest *preq;
  struct pgrequest req;
  struct request_data r;
  int padding;
  u32 reqsize;
  g_error e;

  while (num_requests--) {
    memset(&r,0,sizeof(r));

    /* Extract a request header */
    preq = (struct pgrequest *) *requests;
    *requests += sizeof(struct pgrequest);
    *requests_len -= sizeof(struct pgrequest);
    if (*requests_len < 0)
      return mkerror(PG_ERRT_FILEFMT, 115);    /* Incomplete request */

    /* We need to copy the request header, since request_exec will
     * swap its byte order in-place and we need to keep this copy
     * preserved for future instantiations
     */
    req = *preq;
    r.in.req = &req;
    r.in.owner = owner;
    reqsize = ntohl(req.size);

    /* Get the associated data */
    r.in.data = (void*) *requests;
    *requests += reqsize;
    *requests_len -= reqsize;
    if (*requests_len < 0)
      return mkerror(PG_ERRT_FILEFMT, 115);    /* Incomplete request */
    
    /* Set our handle mapping table */
    handle_setmapping(htable, num_handles);
    e = request_exec(&r);
    errorcheck;
    handle_setmapping(NULL, 0);

    /* Group the resulting handle so it's freed when we are */
    handle_group(owner,group,r.out.ret,owner);

    /* Now stick the return value in the handle table if we need to */
    if (req.id < num_handles)
      htable[req.id] = r.out.ret;

    /* The next packet is padded to a 32-bit boundary */
    if (requests_len) {
      padding = (req.size + sizeof(struct pgrequest)) & 3;
      if (padding) {
	padding = 4-padding;
	*requests += padding;
	*requests_len -= padding;
	if (requests_len<0)
	  requests_len = 0;
      }
    }
  }
  return success;
}

/* The End */
