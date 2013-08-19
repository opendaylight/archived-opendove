/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *
 *  Source File:
 *      dps_client_ctrl.c
 *      This file has functions that deals with network communication between the
 *      DPS client and DPS Server.
 *
 *  Author:
 *      Sushma Anantharam
 *
 */
#include "dps_client_common.h"
#include "dps_pkt.h"
#include "dps_log.h"
#include "raw_proto_timer.h"

uint32_t last_sent_time = 0;                 // The last time a request for a DSP node was sent tot the DMC
uint32_t DpsReqNewNodeInterval = 132;       // Min interval after which a req for a DPS node is sent to DMC
int32_t DpsProtocolLogLevel = DPS_LOGLEVEL_NOTICE;

void dps_retransmit_callback(raw_proto_retransmit_status_t status, char *pRawPkt, 
                             void* context, rpt_owner_t owner);


/*
 ******************************************************************************
 * DPS Client Server Protocol - The Client Part                           *//**
 *
 * \addtogroup DPSClientServerProtocol
 * @{
 * \defgroup DPClientProtocol DPS Client Server Protocol - Client Handling
 * @{
 *
 * This module handles the Client Part of the DPS Client Server Protocol
 *
 */

/**
 *
 * \brief The list of DPS servers this client can commmunicate with
 */
dps_svr_addr_info_t dps_svr_addr_list[1];

/**
 *
 * \brief The VNID->DPS Node mapping table
 */
dps_vnid_node_mapping_table_t dps_vnid_node_mapping_table;

/*
 ***********************************************************************
 * dps_vnid_node_mapping_hash --                                   *//**
 *
 * \brief A hash function to compute the hash value given an array of
 * uint8_t. Currently using the djb2 hash function
 *
 * \param[in]  data	Pointer to the array of uint8_t
 * \param[in]  size	Number of elements in the array
 *
 * \retval An integer value representing the hash.
 *
 ***********************************************************************
 */

uint32_t dps_vnid_node_mapping_hash(uint8_t *data, uint32_t size)
{
	uint32_t hash = 5381;
	uint32_t i;

	for (i = 0; i < size; i++)
	{
		hash = ((hash << 5) + hash) + data[i]; /*hash * 33 + data[i]*/
	}

	return hash;
}

/*
 ***********************************************************************
 * dps_vnid_node_mapping_table_init --                             *//**
 *
 * \brief Initialize the hash list heads of the hash table
 *
 * \param[in]  table	Pointer to the VNID->Node table
 *
 * \retval DPS_SUCCESS or DPS_ERROR_NO_RESOURCES
 *
 ***********************************************************************
 */

int dps_vnid_node_mapping_table_init(dps_vnid_node_mapping_table_t *table)
{
	int ret = DPS_SUCCESS;
	int i;

	table->hash_bits = VNID_NODE_MAPPING_HASH_BITS;
	table->hash_size = 1 << VNID_NODE_MAPPING_HASH_BITS;

	for (i = 0; i < table->hash_size; i++)
	{
		dps_hlist_head_init(&table->hash[i]);
	}

	if (sem_init(&table->lock, 0, 1))
	{
		ret = DPS_ERROR_NO_RESOURCES;
	}

	return ret;
}

/*
 ***********************************************************************
 * dps_vnid_node_mapping_table_clean --                            *//**
 *
 * \brief Removes the entries from the VNID->Node table and cleans up the
 * 	  lock.
 *
 * \param[in]  table	Pointer to the VNID->Node table
 *
 * \retval none.
 *
 ***********************************************************************
 */

void dps_vnid_node_mapping_table_clean(dps_vnid_node_mapping_table_t *table)
{
	dps_vnid_node_mapping_entry_t *entry = NULL;
	dps_hlist_head *hhead;
	dps_hlist_node *hnode;
	int i;

	sem_wait(&table->lock);
	for (i = 0; i < table->hash_size; i++)
	{
		hhead = &table->hash[i];

		while(hhead->first)
		{
			hnode = hhead->first;
			dps_hlist_node_remove(hnode);
			entry = dps_hlist_entry(hnode, dps_vnid_node_mapping_entry_t, hlist_entry);
			free(entry->svr_node);
			free(entry);
		}

		dps_hlist_head_init(&table->hash[i]);
	}
	sem_post(&table->lock);

	sem_destroy(&table->lock);

	return;
}

/*
 ***********************************************************************
 * dps_vnid_node_mapping_table_get_available_entry --              *//**
 *
 * \brief Get first available entry in the mapping table
 *
 *
 * \retval The found entry or NULL
 *
 * \remark - The Table Lock is acquired.
 *
 ***********************************************************************
 */

int
dps_vnid_node_mapping_table_get_available_entry(ip_addr_t *addr)
{
	dps_vnid_node_mapping_table_t *table = &dps_vnid_node_mapping_table;
	dps_vnid_node_mapping_entry_t *entry = NULL;
	dps_hlist_head *hhead;
	dps_hlist_node *hnode;
	int status = DPS_ERROR;
	int i;

	sem_wait(&table->lock);
	for (i = 0; i < table->hash_size; i++)
	{
		hhead = &table->hash[i];

		if (hhead->first)
		{
			hnode = hhead->first;
			entry = dps_hlist_entry(hnode, dps_vnid_node_mapping_entry_t, hlist_entry);
			*addr = *(entry->svr_node);
			status = DPS_SUCCESS;
			dps_log_debug(DpsProtocolLogLevel, "Found dps in vnid %d list dps %x port %d", entry->vn_id, addr->ip4, addr->port);
			break;
		}
	}
	sem_post(&table->lock);

	return status;
}

static inline dps_hlist_head *get_hlist_head(dps_vnid_node_mapping_table_t *table, uint32_t vn_id)
{
	uint32_t hash_bucket;
	
	hash_bucket = dps_vnid_node_mapping_hash((uint8_t *)&vn_id, sizeof(vn_id));
	hash_bucket = hash_bucket & (table->hash_size - 1);
	return(&table->hash[hash_bucket]);
}

static dps_vnid_node_mapping_entry_t *
vnid_node_search_in_bkt(uint32_t vn_id, dps_hlist_head *head)
{
	dps_hlist_node *hnode;
	dps_vnid_node_mapping_entry_t *entry = NULL;
	dps_hlist_for_each_entry(dps_vnid_node_mapping_entry_t, entry, hnode, head, hlist_entry)
	{
		if (entry->vn_id == vn_id)
		{
			break;
		}
		entry = NULL;
	}
	return entry;
}

/*
 ***********************************************************************
 * dps_vnid_node_mapping_entry_search --                           *//**
 *
 * \brief Find the matching entry in the mapping table
 *
 * \param[in]	vn_id	VN Identity
 *
 * \retval The found entry or NULL
 *
 * \remark - The Table Lock is acquired.
 *
 ****************************************************************************
 */

int
dps_vnid_node_mapping_entry_search(uint32_t vn_id, ip_addr_t *addr)
{
	dps_vnid_node_mapping_table_t *table = &dps_vnid_node_mapping_table;
	dps_vnid_node_mapping_entry_t *entry = NULL;
	int status = DPS_SUCCESS;
	dps_hlist_head *hhead;

	hhead  = get_hlist_head(table, vn_id);

	sem_wait(&table->lock);

	entry = vnid_node_search_in_bkt(vn_id, hhead);

	if (entry != NULL)
	{
		*addr = *(entry->svr_node);
		dps_log_debug(DpsProtocolLogLevel, "Found dps in vnid %d list dps %x port %d", entry->vn_id, addr->ip4, addr->port);
	}
	else
	{
		status = DPS_ERROR;
	}
	
	sem_post(&table->lock);

	return status;
}

void print_ip_addr_struct(uint32_t vn_id, ip_addr_t *svr_node)
{
	dps_log_debug(DpsProtocolLogLevel,
	              "DPS ADDRESS: vnid %d family %d ip 0x%x "
	              "port %d xport %d hhtp %d",
	              vn_id, svr_node->family, svr_node->ip4,
	              svr_node->port, svr_node->xport_type, svr_node->port_http);
	return;
}

/*
 ***********************************************************************
 * dps_vnid_node_mapping_entry_add --                              *//**
 *
 * \brief Add a VNID->Node entry to the mapping table. If entry exists,
 *        replace old node with new node.
 *
 * \param[in]	vn_id		VN Identity
 * \param[in]	svr_node	Pointer to DPS node which service for the VNID
 *
 * \retval	DPS_SUCCESS		Entry was added successfully
 * \retval	DPS_ERROR_NO_RESOURCES	The entry could not be allocated
 *
 * \remark The Table Lock is acquired in this routine.
 *
 ***********************************************************************
 */

int dps_vnid_node_mapping_entry_add(uint32_t vn_id, ip_addr_t *svr_node)
{
	dps_vnid_node_mapping_table_t *table = &dps_vnid_node_mapping_table;
	dps_vnid_node_mapping_entry_t *entry;
	dps_hlist_head *hhead;
	int ret = DPS_SUCCESS;

	hhead  = get_hlist_head(table, vn_id);
	sem_wait(&table->lock);
	do
	{
		entry = vnid_node_search_in_bkt(vn_id, hhead);
		dps_log_info(DpsProtocolLogLevel, "Adding ip for vnid: %d new ip: %x entry: %p\r\n",vn_id, svr_node->ip4,entry); 
		if (entry)
		{
			/* If existed, replace it */
			*(entry->svr_node) = *svr_node;
			break;
		}

		entry = (dps_vnid_node_mapping_entry_t *)malloc(sizeof(dps_vnid_node_mapping_entry_t));
		if (entry == NULL)
		{
			ret = DPS_ERROR_NO_RESOURCES;
			break;
		}
		if ((entry->svr_node = (ip_addr_t *)malloc(sizeof(ip_addr_t))) == NULL)
		{
            free(entry);
			ret = DPS_ERROR_NO_RESOURCES;
			break;
		}

		entry->vn_id = vn_id;
		*(entry->svr_node) = *svr_node;
		dps_log_info(DpsProtocolLogLevel,"Added to hash list vnid %d ip %x port %d", 
		              entry->vn_id, entry->svr_node->ip4,  entry->svr_node->port);
		dps_hlist_head_add(hhead, &entry->hlist_entry);
	} while (0);

	sem_post(&table->lock);
	return ret;
}

/*
 ***********************************************************************
 * dps_vnid_entries_eq --                                         *//**
 *
 * \brief Compares 2 ip_addr_t strucutres.If they are equal returns 
 *        DPS_SUCCESS and if they are not equal returns DPS_ERROR
 *
 * \param[in]	node1	DPS server address
 * \param[in]	node2	DPS server address
 *
 * \retval	DPS_SUCCESS	Returns 1 if both structures are equal
 * \retval	0	Returns 0 if they are not
 *
 *
 ***********************************************************************
 */

int dps_vnid_entries_eq(ip_addr_t *node1, ip_addr_t *node2)
{
	if((node1->family == node2->family) && (node1->port == node2->port) &&
	   (node1->xport_type == node2->xport_type))
	{
		if (node1->family == AF_INET )
		{
			if(node1->ip4 == node2->ip4)
				return DPS_SUCCESS;
			else 
				return DPS_ERROR;
		}
		else
		{
			if (memcmp(node1->ip6, node2->ip6, 16))
				return DPS_ERROR;
			else
				return DPS_SUCCESS;
		}
	}
	return DPS_ERROR;
}

/*
 ***********************************************************************
 * dps_vnid_node_mapping_entry_del --                              *//**
 *
 * \brief Deletes the matching DCS node in the VNID->Node entry from 
 *        the mapping table.
 *
 * \param[in]	vn_id		VN Identity
 * \param[in]	svr_node	Pointer to DPS node
 *
 * \retval	DPS_SUCCESS	Entry was deleted successfully if matched
 * \retval	DPS_ERROR	Entry can't be deleted if not matched
 *
 * \remark - The Table Lock is acquired.
 *
 ***********************************************************************
 */

int dps_vnid_node_mapping_entry_del(uint32_t vn_id, ip_addr_t *svr_node)
{
	dps_vnid_node_mapping_table_t *table = &dps_vnid_node_mapping_table;
	dps_vnid_node_mapping_entry_t *entry = NULL;
	dps_hlist_head *hhead;
	int ret = DPS_ERROR;

	hhead  = get_hlist_head(table, vn_id);
	sem_wait(&table->lock);
	do
	{
		entry = vnid_node_search_in_bkt(vn_id, hhead);
		if (entry)
		{
			dps_log_debug(DpsProtocolLogLevel, "DPS to be deleted");
			print_ip_addr_struct(vn_id,svr_node);
			if (dps_vnid_entries_eq(entry->svr_node, svr_node) == DPS_SUCCESS)
			{
				dps_log_debug(DpsProtocolLogLevel, "Matching entry found");
				print_ip_addr_struct(vn_id, entry->svr_node);
				dps_hlist_node_remove(&entry->hlist_entry);
				free(entry->svr_node);
				free(entry);
				ret = DPS_SUCCESS;
			}
		}
		
	} while (0);
	sem_post(&table->lock);
	return ret;
}

/*
 ***********************************************************************
 * dps_vnid_node_mapping_del_dup_node --                           *//**
 *
 * \brief Look for a specific svr ip address in the entire table and if
 *        there are duplicate entries delete it. When the Controller
 *        first gives the seed server address it is added in vnid 0, 
 *        subsequently the server address is added to different vnids as
 *        the mapping is learnt. The Controller subsequently might realize 
 *        server is gone down and it will send a new server address. The
 *        old server address must now be purged from all entries in the
 *        hash list.
 *
 * \param[in]	vn_id		VN Identity
 * \param[in]	svr_node	Pointer to DPS node
 *
 * \retval	DPS_SUCCESS	Entry was deleted successfully if matched
 * \retval	DPS_ERROR	Entry can't be deleted if not matched
 *
 * \remark - The Table Lock is acquired.
 *
 ***********************************************************************
 */
void dps_vnid_node_mapping_del_dup_node(ip_addr_t *svr_node)
{
	int ret = DPS_SUCCESS;
	int i;

	for (i = 0; i < dps_vnid_node_mapping_table.hash_size; i++)
	{
		ret = dps_vnid_node_mapping_entry_del(i, svr_node);
		if (ret == DPS_SUCCESS)
			dps_log_debug(DpsProtocolLogLevel, "Found entry in vnid %d", i);
	}
}

/*
 ***********************************************************************
 * dps_vnid_node_mapping_entry_del2 --                             *//**
 *
 * \brief Delete a VNID->Node entry from the mapping table.
 *
 * \param[in]	vn_id		VN Identity
 *
 * \retval	DPS_SUCCESS	Entry was deleted successfully
 *
 * \remark - The Table Lock is acquired.
 *
 ***********************************************************************
 */

int dps_vnid_node_mapping_entry_del2(uint32_t vn_id)
{
	dps_vnid_node_mapping_table_t *table = &dps_vnid_node_mapping_table;
	dps_vnid_node_mapping_entry_t *entry = NULL;
	dps_hlist_head *hhead;
	int ret = DPS_ERROR;

	hhead  = get_hlist_head(table, vn_id);
	sem_wait(&table->lock);
	entry = vnid_node_search_in_bkt(vn_id, hhead);
	if (entry)
	{
		dps_hlist_node_remove(&entry->hlist_entry);
		free(entry->svr_node);
		free(entry);
		ret = DPS_SUCCESS;
	}
	sem_post(&table->lock);
	return ret;
}

/*
 ***********************************************************************
 * dps_vnid_node_mapping_entry_find --                             *//**
 *
 * \brief Find the matching entry in the mapping table, and copy the 
 *        information into addr. If there is no entry for the vnid, then
 *        return an entry for vnid 0.
 *
 * \param[in]	vn_id	VN Identity
 * \param[out]  addr    return the address of the server
 * \retval The DPS_SUCCESS if entry found or DPS_ERROR if entry not found
 *
 ****************************************************************************
 */

uint32_t
dps_vnid_node_mapping_entry_find(uint32_t vn_id, ip_addr_t *addr)
{

	if (dps_vnid_node_mapping_entry_search(vn_id, addr) == DPS_SUCCESS)
	{
		return DPS_SUCCESS;
	}

	return(dps_vnid_node_mapping_table_get_available_entry(addr));

}

/*
 ***********************************************************************
 * dps_vnid_node_mapping_entry_show --                             *//**
 *
 * \brief Show the specific entry in the mapping table. If VN_ID is 
 *        0xFFFFFFFF, show all entries in the table.
 *
 * \param[in]	vn_id	VN Identity
 *
 * \retval None
 *
 ****************************************************************************
 */

void dps_vnid_node_mapping_entry_show(uint32_t vn_id)
{
	dps_vnid_node_mapping_table_t *table = &dps_vnid_node_mapping_table;
	dps_vnid_node_mapping_entry_t *entry = NULL;
	dps_hlist_head *hhead;
	dps_hlist_node *hnode;
	char ip_str[INET6_ADDRSTRLEN];
	int i;
	uint32_t addr;

    dps_log_notice(DpsProtocolLogLevel, "%12s%50s%10s\n", "VN_ID", "IP Address", "IP Port");

	sem_wait(&table->lock);
	for (i = 0; i < table->hash_size; i++)
	{
		hhead = &table->hash[i];

		dps_hlist_for_each_entry(dps_vnid_node_mapping_entry_t, entry, hnode, hhead, hlist_entry)
		{
			if ((vn_id == 0xFFFFFFFF) || (vn_id == entry->vn_id))
			{
				addr = (entry->svr_node->family == AF_INET ? htonl(entry->svr_node->ip4) : 0);
				inet_ntop(entry->svr_node->family, (entry->svr_node->family == AF_INET ? (void *)&addr : entry->svr_node->ip6), ip_str, INET6_ADDRSTRLEN);
				//printf("%12u%50s%10u\n", entry->vn_id, ip_str, entry->svr_node->port);
                dps_log_notice(DpsProtocolLogLevel, "%12u%50s%10u\n", entry->vn_id, ip_str, entry->svr_node->port);

				if (vn_id == entry->vn_id) break;
			}
			entry = NULL;
		}
		if (entry) break;
	}
	sem_post(&table->lock);

	return;
}

/*
 ******************************************************************************
 * dps_copy_laddr_saddr                                                   *//**
 *
 * \brief - This routine copies the destination address and port number present
 *          in src to a sockaddr structure in network order.
 *
 * \param[in] src - The ip address and port of the dps server
 * \param[out] dst - An destination to which the info is copied
 *
 * \retval DPS_SUCCESS
 *
 * \todo Return "int" instead of iswitch_status
 *
 ******************************************************************************
 */
uint32_t dps_copy_laddr_saddr(ip_addr_t *src, struct sockaddr_storage *dst)
{
	uint32_t status = DPS_SUCCESS;

	switch(src->family)
	{
	    case AF_INET:
	    {
		    struct sockaddr_in *sin =  (struct sockaddr_in *)dst;
		    sin->sin_family = AF_INET;
		    sin->sin_addr.s_addr = htonl(src->ip4);
		    sin->sin_port = htons(src->port);
		    break;
	    }
	    case AF_INET6:
	    {
		    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)dst;
		    sin6->sin6_family = AF_INET6;
		    memcpy(sin6->sin6_addr.s6_addr, src->ip6, 16);
		    sin6->sin6_port = htons(src->port);
		    break;
	    }
	    default:
	    {
		    dps_log_debug(DpsProtocolLogLevel, "Invalid address format");
		    status = DPS_ERROR;
		    break;
	    }
	}		
	return status;
}

/*
 ******************************************************************************
 * dps_copy_saddr_laddr                                                   *//**
 *
 * \brief - This routine copies the senders address and port number present in
 *          src to a ip_addr_t structure. The information in the src is in 
 *          network order and is copied to the dst in host order
 *
 * \param[in] src - The source of the senders addr and port
 * \param[out] dst - An destination to which the info is copied
 *
 * \retval DPS_SUCCESS
 *
 * \todo Return "int" instead of iswitch_status
 *
 ******************************************************************************
 */
void dps_copy_saddr_laddr(struct sockaddr_storage *src, ip_addr_t *dst)
{
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;
	
	if (src->ss_family == AF_INET)
	{
		dst->family = AF_INET;
		sin = (struct sockaddr_in *)src;
		dst->ip4 = ntohl(sin->sin_addr.s_addr);
		dst->port = ntohs(sin->sin_port);
	}
	else
	{
		dst->family = AF_INET6;
		sin6 = (struct sockaddr_in6 *)src;
		memcpy(dst->ip6, sin6->sin6_addr.s6_addr, 16);
		dst->port = ntohs(sin6->sin6_port);
	}
	dst->xport_type = SOCK_DGRAM;
	dst->port_http = 0;
}

/*
 ******************************************************************************
 * dps_process_data_rcvd                                                  *//**
 *
 * \brief - Called by the polling thread that gets an indication that the data
 *          has arrived. The function calls the appropriate dps protocol
 *          function to parse the packet, by looking at the type of the pkt
 *          in the header
 *
 * \param[in] socket - The socket on which the information arrived
 * \param[in] context - An index value used to access the dps server information
 *
 * \retval DPS_SUCCESS
 *
 * \todo Return "int" instead of iswitch_status
 *
 ******************************************************************************
 */

static int dps_process_data_rcvd(int socket, void *context)
{
	struct sockaddr_storage client_addr;
	ip_addr_t sender_addr = {AF_INET, 0};
	int bytes_read, addr_len = sizeof(client_addr);
	int loops = 0;
	size_t index = (size_t)context;
	dps_client_hdr_t hdr;
#if defined (NDEBUG) || defined (VMX86_DEBUG)
	char str_ip[INET6_ADDRSTRLEN];
	struct sockaddr_in *sin =  (struct sockaddr_in *)&client_addr;
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&client_addr;
#endif
	do
	{
		bytes_read = recvfrom(socket, dps_svr_addr_list[index].buff, DPS_MAX_BUFF_SZ,
		                      0, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);

		if(bytes_read <= 0)
		{
			if (loops == 0)
			{
				//Why did we get waken up???
				dps_log_error(DpsProtocolLogLevel, "[loop %d] recvfrom() error %s",
				              loops, strerror(errno));
			}
			else
			{
				//Normal
				dps_log_info(DpsProtocolLogLevel, "[loop %d] recvfrom() error %s",
				             loops, strerror(errno));
			}
			break;
		}
		else
		{
			dps_copy_saddr_laddr(&client_addr, &sender_addr);
			dps_get_pkt_hdr((dps_pkt_hdr_t *)(dps_svr_addr_list[index].buff), &hdr);			
			dps_vnid_node_mapping_entry_add(hdr.vnid, &sender_addr);


			dps_log_debug(DpsProtocolLogLevel, "Msg from [%s:%d] bytes read %d",
			              inet_ntop(client_addr.ss_family, 
			                        ((client_addr.ss_family == AF_INET) ? &(sin->sin_addr.s_addr) : (void *)sin6->sin6_addr.s6_addr), 
			                        str_ip, sizeof(str_ip)), 
			              (client_addr.ss_family == AF_INET) ? ntohs(sin->sin_port) : ntohs(sin6->sin6_port), bytes_read);
		
			dps_process_rcvd_pkt((void *)dps_svr_addr_list[index].buff, (void *)&sender_addr);



		}
		loops++;
	} while (loops < 10000); // Process Max 10000 replies at a time before giving up CPU

	//TODO: Should this be 0 or ISWITCH_STATUS_OK
	return DPS_SUCCESS;
}

void dps_update_svr_addr_list(ip_addr_t *dps_svr_node)
{
	if (dps_svr_node->family == AF_INET)
	{
		dps_svr_addr_list[0].family = AF_INET;
		dps_svr_addr_list[0].svr_ip4.sin_family = AF_INET;
		dps_svr_addr_list[0].svr_ip4.sin_addr.s_addr = dps_svr_node->ip4;

	}
	else
	{
		dps_svr_addr_list[0].family = AF_INET6;
		dps_svr_addr_list[0].svr_ip6.sin6_family = AF_INET6;
		memcpy(dps_svr_addr_list[0].svr_ip6.sin6_addr.s6_addr, dps_svr_node->ip6, 16);
	}
	dps_svr_addr_list[0].svr_ip4.sin_port = dps_svr_node->port;
	dps_svr_addr_list[0].xport_type = dps_svr_node->xport_type;
}
/*
 ******************************************************************************
 * dps_server_add                                                         *//**
 *
 * \brief - This function adds DPS Server information to be used by the DPS
 *          protocol client. The DPS protocol uses one of the DPS Servers to
 *          send the information to, when requested by the DPS client
 *
 * \param[in] domain - The Domain associated with the DPS Server Node. A
 *                     value of 0 indicates that this dps node is not associated
 *                     with any particular domain.
 * \param[in] dps_svr_node - Address of the DPS Server (Network Byte Order)
 *
 * \retval ISWITCH_STATUS_OK
 *
 * \todo Return "int" instead of iswitch_status
 *
 ******************************************************************************
 */

dps_return_status dps_server_add(uint32_t domain, ip_addr_t *dps_svr_node)
{
	dps_return_status status = DPS_SUCCESS;
	int comm_status = 0;
	ip_addr_t old_svr_addr = {0};
	char str[INET6_ADDRSTRLEN];
	uint32_t nip4 = 0;

	do
	{
		if (dps_svr_node->family == AF_INET)
		{
			nip4 = htonl(dps_svr_node->ip4);
		}
		dps_log_alert(DpsProtocolLogLevel, "Adding DCS Node: %s \r\n", inet_ntop(dps_svr_node->family, 
			   ((dps_svr_node->family == AF_INET) ? (void *)&nip4 : (void *)dps_svr_node->ip6), str, INET6_ADDRSTRLEN));

		if (dps_svr_addr_list[0].inuse)
		{
			if (dps_vnid_node_mapping_entry_search(0, &old_svr_addr) == DPS_SUCCESS)
			{
				dps_log_debug(DpsProtocolLogLevel, "Found dps in vnid 0 list dps %x port %d", 
				              old_svr_addr.ip4, old_svr_addr.port);

				if ((dps_svr_node->family == old_svr_addr.family) &&
				    (dps_svr_node->ip4 == old_svr_addr.ip4) &&
				    (dps_svr_node->port == old_svr_addr.port))
				{
					dps_log_notice(DpsProtocolLogLevel, "Old and New  DPS the same -> IP: %x port: %d family: %d\r\n", 
					               dps_svr_node->ip4, dps_svr_node->port, dps_svr_node->family);
				}
				else
				{
					dps_vnid_node_mapping_del_dup_node(&old_svr_addr);
					dps_update_svr_addr_list(dps_svr_node);
					dps_vnid_node_mapping_entry_add(0, dps_svr_node);
				}
			}
			else
			{
				dps_vnid_node_mapping_entry_add(0, dps_svr_node);
			}

		}
		else
		{
			
			dps_update_svr_addr_list(dps_svr_node);
				
			dps_svr_addr_list[0].inuse = 1;
		
			// Open socket for datagram service
			if (dps_svr_addr_list[0].xport_type == SOCK_DGRAM)
			{
				if ((dps_svr_addr_list[0].sock_fd = socket(dps_svr_addr_list[0].svr_ip4.sin_family, SOCK_DGRAM, 0)) < 0)
				{
					dps_svr_addr_list[0].inuse = 0;
					status = DPS_ERROR_NO_RESOURCES;
					dps_log_error(DpsProtocolLogLevel,"Cannot create socket");
					break;
				}
				if (fcntl(dps_svr_addr_list[0].sock_fd, F_SETFL, O_NONBLOCK) == -1)
				{
					dps_log_emergency(DpsProtocolLogLevel,
					                  "Cannot set socket %d to non-blocking",
					                  dps_svr_addr_list[0].sock_fd);
					break;
				}
				comm_status = fd_process_add_fd(dps_svr_addr_list[0].sock_fd,
				                                dps_process_data_rcvd, (void *)0);
				// add to the socket fd list
				if (comm_status == 0)
				{
					dps_vnid_node_mapping_entry_add(0, dps_svr_node);
					// Success
					status = DPS_SUCCESS;
					break;
				}
				status = DPS_ERROR;
				dps_svr_addr_list[0].inuse = 0;
				close(dps_svr_addr_list[0].sock_fd);
				dps_log_error(DpsProtocolLogLevel,"Cannot add UDP socket fd to poll list");
			}
		}
		
	}while(0);

	return status;
}

/*
 ******************************************************************************
 * dps_server_del                                                         *//**
 *
 * \brief - This function deletes DPS Server information. The message to delete
 *          comes from from the Dove Controller
 *
 * \param[in] dps_svr_node - Address of the DPS Server (Network Byte Order)
 *
 * \retval None
 *
 ******************************************************************************
 */

void dps_server_del(ip_addr_t *dps_svr_node)
{
	dps_svr_addr_list[0].svr_ip4.sin_addr.s_addr = 0;
	dps_svr_addr_list[0].svr_ip4.sin_port = 0;
	dps_vnid_node_mapping_del_dup_node(dps_svr_node);
	return;
}

/*
 ******************************************************************************
 * dps_find_server                                                        *//**
 *
 * \brief - This function finds the dps server location for the specific vnid
 *
 * \param[in] vnid - The vnid id for which a dps server needs to be located
 * \param[out] svr_addr - The ip address and port of the dps in network order
 *
 * \retval None
 *
 ******************************************************************************
 */
uint32_t dps_find_server(uint32_t vnid, struct sockaddr_storage *svr_addr)
{
	ip_addr_t addr;
	uint32_t status = DPS_SUCCESS;

	if ((status = dps_vnid_node_mapping_entry_find(vnid, &addr)) == DPS_SUCCESS)
	{
		status = dps_copy_laddr_saddr(&addr, svr_addr);
		dps_log_debug(DpsProtocolLogLevel,"Return Status %d", status);
	}
	return status;
}

/*
 ******************************************************************************
 * dps_req_new_dps_node                                                    *//**
 *
 * \brief - This function finds the node that was not reachable and deletes
 *          the node from the vnid mapping table and sends a request to the 
 *          DPS client to fetch a new DPS node
 *
 * \param[in] vnid - The vnid id for which was used to located the dcs node
 *
 * \retval None
 *
 ******************************************************************************
 */
void dps_req_new_dps_node(uint32_t vnid)
{
	dps_client_data_t data;

	// Delete from the corresponding vnid mapping table
	dps_vnid_node_mapping_entry_del2(vnid);
	// Don't send another req to get seed dps node if you have "recently" sent the msg
	if (difftime(time(NULL), last_sent_time) >= DpsReqNewNodeInterval)
	{ 		
		last_sent_time = time(NULL);
		dps_log_notice(DpsProtocolLogLevel,"Sending a DPS new node request to DMC");
		// Request for a new DPS node
		memset((void *)&data, 0, sizeof(dps_client_hdr_t));
		data.hdr.type = DPS_GET_DCS_NODE;
		dps_protocol_send_to_client(&data);
	}
}

inline size_t dps_get_sockaddr_len(struct sockaddr_storage *svr_addr) 
{
	return(svr_addr->ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));
}

/*
 ******************************************************************************
 * dps_protocol_xmit                                                      *//**
 *
 * \brief - This routine should to be called to send a Buffer/Packet to a
 *          Remote destination
 *
 * \param[in] buff - Pointer to a char buffer
 * \param[in] buff_len - Size of the Buffer
 * \param[in] addr - The Remote (DPS Server) End to send the Buffer To
 *
 * \retval DPS_SUCCESS
 *
 ******************************************************************************
 */

uint32_t dps_protocol_xmit(uint8_t *buff, uint32_t buff_len, ip_addr_t *addr, void *context)
{
	int32_t bytes_sent, status = DPS_SUCCESS;
	dps_client_hdr_t hdr;
	int32_t rc;
	struct sockaddr_storage dst_addr;
	char str_ip[INET6_ADDRSTRLEN];
	struct sockaddr_in *sin =  (struct sockaddr_in *)&dst_addr;
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&dst_addr;

	if (!dps_svr_addr_list[0].inuse)
	{
		dps_log_debug(DpsProtocolLogLevel,"Socket not initialized");
		return(DPS_XMIT_ERROR);
	}

	dps_get_pkt_hdr((dps_pkt_hdr_t *)buff, &hdr);
	//Check to see if valid ip address present. If present use it to send the msg
	//This is needed for unsolicited msgs from the DPS which needs to be acked.
	if ((addr->family == AF_INET) || (addr->family == AF_INET6))
	{
		if ((status = dps_copy_laddr_saddr(addr, &dst_addr)) != DPS_SUCCESS)
		{
 			dps_log_debug(DpsProtocolLogLevel,"Address not valid");
			return(DPS_XMIT_ERROR);
		}
	}
	else
	{
		if ((status = dps_find_server(hdr.vnid, &dst_addr)) != DPS_SUCCESS)
		{
			dps_log_info(DpsProtocolLogLevel,"Could not find any servers");
			return(DPS_XMIT_ERROR);
		}
	}
	
	dps_log_info(DpsProtocolLogLevel, "DPS Server Address: %s port %d\n",
	              inet_ntop(dst_addr.ss_family, 
	                        ((dst_addr.ss_family == AF_INET) ? &(sin->sin_addr.s_addr) : (void *)sin6->sin6_addr.s6_addr), 
	                        str_ip, sizeof(str_ip)), 
	              (dst_addr.ss_family == AF_INET) ? ntohs(sin->sin_port) : ntohs(sin6->sin6_port));


	if (context)
	{

		dps_log_debug(DpsProtocolLogLevel,"Starting Rexmit: Context x%x, Pkt Type %d, QID %d",
		              context, hdr.type, hdr.query_id);
#if 1 
		rc = raw_proto_timer_start((char *)buff, buff_len, hdr.query_id,
		                           RAW_PKT_DEF_RETRANSMIT_INTERVAL, MAX_NUM_DEF_RETRANSMIT,
		                           dps_svr_addr_list[0].sock_fd, (struct sockaddr *)&dst_addr,
		                           context, RPT_OWNER_DPSA);
		if (rc)
		{
			dps_log_error(DpsProtocolLogLevel, "raw_proto_timer_start returned FAIL ...");
			status = DPS_ERROR_NO_RESOURCES;
		}
#endif
	}

	bytes_sent = sendto(dps_svr_addr_list[0].sock_fd,
	                    (void *)buff, buff_len, 0,
	                    (struct sockaddr *)&(dst_addr),
	                    dps_get_sockaddr_len(&dst_addr));

	if(bytes_sent <= 0)
	{
		dps_log_error(DpsProtocolLogLevel, "sendto() error %s", strerror(errno));
		status = DPS_XMIT_ERROR;
	}
	else
	{
		dps_log_info(DpsProtocolLogLevel, "sendto() bytes_sent %d", bytes_sent);
	}

	return status;
}

/*
 ******************************************************************************
 * dps_client_init --                                                     *//**
 *
 * \brief This routine is called to initialize the dps client.
 *
 * \retval None
 *
 ******************************************************************************
 */

dps_return_status dps_client_init(void)
{
	dps_return_status ret = DPS_SUCCESS;
	int32_t rc;
//	ip_addr_t dps_coordinator_node;

	dps_log_debug(DpsProtocolLogLevel, "Enter");

	// Register the callback that the controller will call with a list of DPS nodes
	//dps_server_reg_dereg_callback(dps_server_add, dps_server_del);

//	dps_coordinator_node.family = AF_INET;
//	dps_coordinator_node.ip4 = inet_addr("172.31.46.40");
//	dps_coordinator_node.port = 12345;
//	dps_coordinator_node.xport_type = SOCK_DGRAM;
//
//	ret = dps_server_add(0, &dps_coordinator_node);
	dps_vnid_node_mapping_table_init(&dps_vnid_node_mapping_table);

	rc = raw_proto_timer_init(&dps_retransmit_callback, RPT_OWNER_DPSA);
	if (rc != RAW_PROTO_TIMER_RETURN_OK)
	{
		ret = DPS_ERROR_NO_RESOURCES;
	}
	dps_log_debug(DpsProtocolLogLevel, "Exit");

	return ret;
}

/** @} */
/** @} */

