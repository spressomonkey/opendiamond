/*
 * These file handles a lot of the device specific code.  For the current
 * version we have state for each of the devices.
 */
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include "ring.h"
#include "obj_attr.h"
#include "lib_od.h"
#include "lib_odisk.h"
#include "lib_searchlet.h"
#include "socket_trans.h"
#include "lib_dctl.h"
#include "lib_sstub.h"
#include "sstub_impl.h"



void
sstub_write_data(listener_state_t *lstate, cstate_t *cstate)
{
	obj_data_t	*obj;
	int		sent;
	int		vnum;
	int		err;
	int		header_remain, header_offset;
	int		attr_remain, attr_offset;
	int		data_remain, data_offset;
	char *		data;


	if (cstate->data_tx_state == DATA_TX_NO_PENDING) {
		pthread_mutex_lock(&cstate->cmutex);
		err = ring_2deq(cstate->obj_ring, (void **)&obj  , 
				(void **)&vnum);

		/*
		 * if there is no other data, then clear the obj data flag
		 */
		if (err) {
			cstate->flags &= ~CSTATE_OBJ_DATA;
			pthread_mutex_unlock(&cstate->cmutex);
			return;
		}
		pthread_mutex_unlock(&cstate->cmutex);

		cstate->data_tx_obj = obj;

		/* 
		 * Construct the header for the object we are going
		 * to send out.
		 */
		cstate->data_tx_oheader.obj_magic = htonl(OBJ_MAGIC_HEADER);
		cstate->data_tx_oheader.attr_len  = 
			htonl((int)obj->attr_info.attr_len);
		cstate->data_tx_oheader.data_len  = 
			htonl((int)obj->data_len);
		cstate->data_tx_oheader.version_num  = htonl((int)vnum);



		/* setup the remain and offset counters */
		header_offset = 0;
		header_remain = sizeof(cstate->data_tx_oheader);
		attr_offset = 0;
		attr_remain = obj->attr_info.attr_len;
		data_offset = 0;
		data_remain = obj->data_len;
		

	} else if (cstate->data_tx_state == DATA_TX_HEADER) {
		obj = cstate->data_tx_obj;

		header_offset = cstate->data_tx_offset;
		header_remain = sizeof(cstate->data_tx_oheader) - 
			header_offset;
		attr_offset = 0;
		attr_remain = obj->attr_info.attr_len;
		data_offset = 0;
		data_remain = obj->data_len;

	} else if (cstate->data_tx_state == DATA_TX_ATTR) {
		obj = cstate->data_tx_obj;

		header_offset = 0;
		header_remain = 0;
		attr_offset = cstate->data_tx_offset;
		attr_remain = obj->attr_info.attr_len - attr_offset;
		data_offset = 0;
		data_remain = obj->data_len;
	} else {
		assert(cstate->data_tx_state == DATA_TX_DATA);

		obj = cstate->data_tx_obj;

		header_offset = 0;
		header_remain = 0;
		attr_offset = 0;
		attr_remain = 0;
		data_offset = cstate->data_tx_offset;
		data_remain = obj->data_len - data_offset;
	}

	/*
	 * If we haven't sent all the header yet, then go ahead
	 * and send it.
	 */

	if (header_remain > 0) {
		data = (char *)&cstate->data_tx_oheader;
		sent = send(cstate->data_fd, &data[header_offset],
			       	header_remain, 0);

		if (sent < 0) {
			if (errno == EAGAIN) {
				cstate->data_tx_state = DATA_TX_HEADER;
				cstate->data_tx_offset = header_offset;
				return;
			} else {
				/* XXX what errors should we handles ?? */
				perror("send oheader ");
			       	printf("XXX error while sending oheader\n");
				exit(1);
			}

		}
		if (sent != header_remain) {
			cstate->data_tx_state = DATA_TX_HEADER;
			cstate->data_tx_offset = header_offset + sent;
			return;
		}
	}

	/*
	 * If there is still some attributes to send, then go ahead and
	 * send it.
	 */

	if (attr_remain) {
		data = (char *)cstate->data_tx_obj->attr_info.attr_data;
		sent = send(cstate->data_fd, &data[attr_offset],
			       	attr_remain, 0);

		if (sent < 0) {
			if (errno == EAGAIN) {
				cstate->data_tx_state = DATA_TX_ATTR;
				cstate->data_tx_offset = attr_offset;
				return;
			} else {
				/* XXX what errors should we handles ?? */
				perror("send attr ");
			       	printf("XXX error while sending attr\n");
				exit(1);
			}

		}
		if (sent != attr_remain) {
			cstate->data_tx_state = DATA_TX_ATTR;
			cstate->data_tx_offset = attr_offset + sent;
			return;
		}
	}


	/*
	 * If there is still data to be sent, then go ahead and
	 * send it.
	 */
	
	if (data_remain) {
		data = (char *)cstate->data_tx_obj->data;

		sent = send(cstate->data_fd, &data[data_offset],
			       	data_remain, 0);

		if (sent < 0) {
			if (errno == EAGAIN) {
				cstate->data_tx_state = DATA_TX_DATA;
				cstate->data_tx_offset = data_offset;
				return;
			} else {
				/* XXX what errors should we handles ?? */
				perror("send data ");
			       	printf("XXX error while sending data\n");
				exit(1);
			}

		}
		if (sent != data_remain) {
			cstate->data_tx_state = DATA_TX_DATA;
			cstate->data_tx_offset = data_offset + sent;
			return;
		}
	}

    /* some stats */
    cstate->stats_objs_tx++;
    cstate->stats_objs_attr_bytes_tx += obj->attr_info.attr_len;
    cstate->stats_objs_data_bytes_tx += obj->data_len;
    cstate->stats_objs_hdr_bytes_tx += sizeof(cstate->data_tx_oheader);
    cstate->stats_objs_total_bytes_tx += sizeof(cstate->data_tx_oheader) +
            obj->attr_info.attr_len + obj->data_len;

	/*
	 * If we make it here, then we have sucessfully sent
	 * the object so we need to make sure our state is set
	 * to no data pending, and we will call the callback the frees
	 * the object.
	 */

	cstate->data_tx_state = DATA_TX_NO_PENDING;
	(*lstate->release_obj_cb)(cstate->app_cookie, cstate->data_tx_obj);

	return;
}

void
sstub_except_data(listener_state_t *lstate, cstate_t *cstate)
{
	/* Handle the case where we are shutting down */
	if (cstate->flags & CSTATE_SHUTTING_DOWN) {
		return;
	}

	return;
}



void
sstub_read_data(listener_state_t *lstate, cstate_t *cstate)
{

	/* Handle the case where we are shutting down */
	if (cstate->flags & CSTATE_SHUTTING_DOWN) {
		return;
	}

	return;
}

