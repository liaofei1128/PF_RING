/*
 *  Copyright (C) 2016 ntop.org
 *
 *      http://www.ntop.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesses General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 */

#include <stdio.h>

#include "fast_bpf.h"

/* *********************************************************** */

int _is_emptyv6(struct fast_bpf_in6_addr *a) {
  int i;

  for(i=0; i<4; i++)
    if(a->u6_addr.u6_addr32[i] != 0)
      return(1);

  return(0);
}

/* *********************************************************** */

void rule_to_napatech(u_int8_t stream_id, u_int8_t port_id,
		      char *cmd, u_int cmd_len,
		      fast_bpf_rule_core_fields_t *c) {
  char *proto = "", buf[256];
  int num_cmds = 0;

  bpf_append_str(cmd, cmd_len, 0, "Assign[StreamId = 1] = Port == 0 AND ");

  if(c->vlan_id) bpf_append_str(cmd, cmd_len, num_cmds++, "((Encapsulation == VLAN)");

  switch(c->proto) {
  case 1:  bpf_append_str(cmd, cmd_len, num_cmds++, "(Layer4Protocol == ICMP)"); break;
  case 6:  bpf_append_str(cmd, cmd_len, num_cmds++, "(Layer4Protocol == TCP)"), proto = "Tcp";  break;
  case 17: bpf_append_str(cmd, cmd_len, num_cmds++, "(Layer4Protocol == UDP)"), proto = "Udp";  break;
  }

  if(c->ip_version == 4) {
    char a[32];

    if(c->shost.v4) { 
      snprintf(buf, sizeof(buf), "mIPv4%sAddr == [%s]", "Src", bpf_intoaV4(ntohl(c->shost.v4), a, sizeof(a))); 
      bpf_append_str(cmd, cmd_len, num_cmds++,  buf); 
    }
    
    if(c->dhost.v4) { 
      snprintf(buf, sizeof(buf), "mIPv4%sAddr == [%s]", "Dest", bpf_intoaV4(ntohl(c->dhost.v4), a, sizeof(a))); 
      bpf_append_str(cmd, cmd_len, num_cmds++,  buf); 
    }
  } else if(c->ip_version == 6) {
    char a[64];

    if(!_is_emptyv6(&c->shost.v6)) {    
      snprintf(buf, sizeof(buf), "mIPv6%sAddr == [%s]", "Src", bpf_intoaV6(&c->shost.v6, a, sizeof(a))); 
      bpf_append_str(cmd, cmd_len, num_cmds++,  buf); 
    }
    
    if(!_is_emptyv6(&c->dhost.v6)) { 
      snprintf(buf, sizeof(buf), "mIPv6%sAddr == [%s]", "Dest", bpf_intoaV6(&c->dhost.v6, a, sizeof(a))); 
      bpf_append_str(cmd, cmd_len, num_cmds++,  buf); 
    }
  }

  if(c->sport_low > 0) { 
    snprintf(buf, sizeof(buf), "m%s%sPort == %u", proto, "Src",  ntohs(c->sport_low)); 
    bpf_append_str(cmd, cmd_len, num_cmds++,  buf); 
  }

  if(c->dport_low > 0) { 
    snprintf(buf, sizeof(buf), "m%s%sPort == %u", proto, "Dest", ntohs(c->dport_low)); 
    bpf_append_str(cmd, cmd_len, num_cmds++,  buf);
  }

  if(c->vlan_id) bpf_append_str(cmd, cmd_len, num_cmds++, ")");
}