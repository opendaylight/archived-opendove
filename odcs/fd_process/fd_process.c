/******************************************************************************
** File Main Owner:   DOVE Development Team
** File Description:  The CORE (Communication) APIs used to exchange Messages
**                    via UDP/TCP. Phase 1
**/
/*
{
*
*  HISTORY
*
 *
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*  $Log: fd_process_phase1.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/

#include "fd_process_internal.h"

/**
 * \brief The Events the poll should wait on
 */
const int poll_event = POLLIN | POLLPRI | POLLERR | POLLNVAL | POLLHUP;

/**
 * \brief The number of Sockets supported
 */

#define CORE_API_P1_MAX_SOCKETS 4

/**
 * \brief Max file descriptors allowed for polling
 *        1 for the Dummy FDs
 */
#define CORE_API_P1_POLL_FD_MAX (1 + CORE_API_P1_MAX_SOCKETS)

/*
 ******************************************************************************
 * DPSServerCoreAPI                                                       *//**
 *
 * \addtogroup DPSServer
 * @{
 * \defgroup DPSServerCoreAPI CORE APIs
 * @{
 * This module implements the CORE API
 */

/**
 * \brief A pair of file descriptors,  fd_poll_dummy[0] is for
 * reading, fd_poll_dummy[1] is for writing. fd_poll_dummy[0]
 * will be added to fd_process_pfd anytime, and used to wake up poll thread
 * when poll_update_fds is called
 */
static int fd_poll_dummy[2] = {-1, -1};

/**
 * \brief The Global Polling Structure
 */
static struct pollfd pfd[CORE_API_P1_POLL_FD_MAX];

/*
 * \brief A Structure that is utilized to store function and data
 * related to a File Descriptor. This data will enable the poll
 * processing routine to issue a callback to the correct routine
 * to process the events on the file descriptor
 */
typedef struct fd_process_poll_callback_s{
	/**
	 * \brief The Callback Routine
	 */
	poll_event_callback poll_callback_routine;
	/**
	 * \brief The Callback Context
	 */
	void *context;
} fd_process_poll_callback_t;

/**
 * \brief A List of Routines that is associated with every File
 * Descriptor. When the File Descriptor is activated this routine
 * is called.
 */
static fd_process_poll_callback_t poll_callback_array[CORE_API_P1_POLL_FD_MAX];

/**
 * \brief The Total File Descriptor count needed for Polling
 */
static int fd_count_total = 0;

/**
 * \brief A Global mutex for interacting with File Descriptors
 */

static pthread_mutex_t base_mutex = PTHREAD_MUTEX_INITIALIZER;

static int LogLevel = DPS_SERVER_LOGLEVEL_WARNING;

/*
 ******************************************************************************
 * poll_dummy_info_process --                                                  *//**
 *
 * \brief Process for dummy information, which is only used for waking up
 *  poll thread
 *
 * \param[in] fd The File Descriptor for Events, Log and Statistics
 * \param[in] context Not Used
 *
 * \retval 0 No more event processing currently
 * \retval 1 If there are event processing to be done
 *
 *****************************************************************************/
static int poll_dummy_info_process(int fd, void *context)
{
	char dummy;
	if(fd >= 0)
	{
		if(1 != read(fd, &dummy, 1))
		{
			log_debug(LogLevel, "No data to read in dummy");
		}
	}
	return 0;
}


/*
 ******************************************************************************
 * poll_array_init --                                                     *//**
 *
 * \brief This routine initializes the polling array
 *
 * \retval Count The number of file descriptors which can be polled
 *
 *****************************************************************************/

static dove_status poll_array_init(void)
{
	dove_status status = DOVE_STATUS_OK;
	int index;

	for (index = 0; index < CORE_API_P1_POLL_FD_MAX ; index++)
	{
		pfd[index].fd = -1;
		pfd[index].events = poll_event;
		pfd[index].revents = 0;
		poll_callback_array[index].poll_callback_routine = NULL;
		poll_callback_array[index].context = NULL;
	}

	// Set the Dummy FD
	if (pipe(fd_poll_dummy) == -1)
	{
		log_critical(LogLevel, "pipe: fd_poll_dummy creation failed ");
		status = DOVE_STATUS_INVALID_FD;
	}
	else
	{
		if ((fcntl(fd_poll_dummy[0], F_SETFL, O_NONBLOCK) == -1) ||
		    (fcntl(fd_poll_dummy[1], F_SETFL, O_NONBLOCK) == -1))
		{
			log_critical(LogLevel, "pipe: Set to UNBLOCK failed ");
			status = DOVE_STATUS_INVALID_FD;
		}
	}

	if (status == DOVE_STATUS_OK)
	{
		pfd[POLL_DUMMY_FD_INDEX].fd = fd_poll_dummy[0];
		pfd[POLL_DUMMY_FD_INDEX].events = poll_event;
		poll_callback_array[POLL_DUMMY_FD_INDEX].poll_callback_routine = poll_dummy_info_process;
		poll_callback_array[POLL_DUMMY_FD_INDEX].context = NULL;
		fd_count_total = 1;
	}

	return status;
}


/*
 ******************************************************************************
 * poll_print_events --                                                   *//**
 *
 * \brief This routine prints the events that were set.
 *
 * \retval None
 *
 *****************************************************************************/

static void poll_print_events(int fd_index, int fd, int events)
{
	char print_str[128];

	memset(print_str, 0, 128);

	if(events & POLLIN)
	{
		strcat(print_str, "POLLIN ");
	}
	if(events & POLLPRI)
	{
		strcat(print_str, "POLLPRI ");
	}
	if(events & POLLERR)
	{
		sprintf(print_str, "POLLERR ");
	}
	if(events & POLLNVAL)
	{
		sprintf(print_str, "POLLNVAL ");
	}
	if(events & POLLHUP)
	{
		sprintf(print_str, "POLLHUP ");
	}
	log_debug(LogLevel, "Index %d, FD %d, Events [0x%x:%s]",
	          fd_index, fd, events, print_str);
	return;

}

/*
 ******************************************************************************
 * poll_process_result --                                               *//**
 *
 * \brief This routine generates the array of polling file descriptors. It will
 * return the count of file descriptors that can be polled.
 *
 * \retval ISWITCH_STATUS_OK Success
 * \retval Other The Character Device had error.
 *
 *****************************************************************************/

static dove_status poll_process_result(struct pollfd *fds,
                                             int numfds,
                                             int poll_result)
{
	dove_status status = DOVE_STATUS_OK;
	int fd_index;
	int processed_index = 0;
	int count;
	struct pollfd *currfd;
	poll_event_callback callback_routine;
	void *callback_context;
	int continue_processing = 0;

	log_info(LogLevel, "Enter");

	if (poll_result < 0)
	{
		status = DOVE_STATUS_INTERRUPT;
		log_info(LogLevel, "Poll Result Errno %d", errno);
		goto Exit;
	}
	if (poll_result == 0)
	{
		log_info(LogLevel, "Poll Timed out");
		goto Exit;
	}

	// First check only the FDs that were set
	fd_index = 0;
	count = 0;
	while((count < poll_result) && (processed_index < numfds) &&
	      (fd_index < fd_count_total))
	{

		log_debug(LogLevel,
		          "Checking Index %d, Processed Index %d, NumFds %d, "
		          "Count %d, poll_result %d",
		          fd_index, processed_index, numfds, count, poll_result);

		currfd = &fds[fd_index];

		log_debug(LogLevel,
		          "Socket %d, Revents %d",
		          currfd->fd, currfd->revents);

		currfd->events = poll_event;
		if (currfd->revents == 0)
		{
			fd_index++;
			continue;
		}

		if (LogLevel >= DPS_SERVER_LOGLEVEL_VERBOSE)
		{
			poll_print_events(fd_index, currfd->fd, currfd->revents);
		}

		callback_routine = poll_callback_array[fd_index].poll_callback_routine;
		callback_context = poll_callback_array[fd_index].context;

		if (callback_routine)
		{
			continue_processing |= callback_routine(currfd->fd,
			                                        callback_context);
		}

		currfd->revents = 0;
		processed_index++;
		fd_index++;
		count++;
	}

	// If some module needs to continue processing, then process all
	// the fds in the array
	while(continue_processing)
	{
		log_debug(LogLevel, "Continuing Processing...");
		continue_processing = 0;
		for (fd_index = 0; fd_index < fd_count_total; fd_index++)
		{
			currfd = &fds[fd_index];
			if (currfd->fd < 0)
			{
				continue;
			}
			callback_routine = poll_callback_array[fd_index].poll_callback_routine;
			callback_context = poll_callback_array[fd_index].context;

			/* TODO: TEMP fix it, call poll_dummy_info_process may block the process
			 * if there is no one write the pipe, it may happen in continue_processing
			 * maybe should set the poll_dummy_info to unblock
			 */
#if 0
			if (callback_routine && callback_routine != poll_dummy_info_process)
#endif
			if (callback_routine)
			{
				continue_processing |= callback_routine(currfd->fd,
				                                        callback_context);
			}
		}
	}

Exit:

	log_debug(LogLevel, "Exit %s",
	          DOVEStatusToString(status));

	return status;
}


/*
 ******************************************************************************
 * fd_process_add_fd --                                                     *//**
 *
 * \brief This routine adds a new file descriptor to the array of polled
 *        file descriptors
 *
 * \param[in] fd	The File Descriptor
 * \param[in] callback	The Callback to invoke when an event occurs on the fd
 * \param[in] context	The Context to invoke with the callback
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_NO_RESOURCES Polling FD Array is Full
 * \retval DOVE_STATUS_INVALID_PARAMETER Bad File Descriptor or Callback
 * \retval DOVE_STATUS_EXISTS File Descriptor Exists in the Array
 *
 * \note This routine updates the Global fd_count_total to the number of
 *       FDs that can be polled
 *
 *****************************************************************************/

int fd_process_add_fd(int fd,
                    poll_event_callback callback,
                    void *context)
{
	char dummy;
	int fd_index;
	dove_status status = DOVE_STATUS_NO_RESOURCES;

	log_info(LogLevel, "Enter");

	pthread_mutex_lock(&base_mutex);

	do
	{
		if((fd < 0) || (callback == NULL))
		{
			status = DOVE_STATUS_INVALID_PARAMETER;
			break;
		}

		for (fd_index = 0 ; fd_index < CORE_API_P1_POLL_FD_MAX ; fd_index++)
		{
			if (pfd[fd_index].fd >= 0)
			{
				continue;
			}
			if (pfd[fd_index].fd == fd)
			{
				status = DOVE_STATUS_EXISTS;
				break;
			}
			pfd[fd_index].fd = fd;
			pfd[fd_index].events = poll_event;
			poll_callback_array[fd_index].poll_callback_routine = callback;
			poll_callback_array[fd_index].context = context;
			status = DOVE_STATUS_OK;
			fd_count_total++;
			break;
		}

		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		if(fd_poll_dummy[1] >= 0)
		{
			/* Wake up poll */
			if(1 != write(fd_poll_dummy[1], &dummy, 1))
			{
				log_debug(LogLevel, "write poll dummy fd error!");
			}
		}
	} while (0);

	pthread_mutex_unlock(&base_mutex);

	log_info(LogLevel, "Exit %s, FD Count %d",
	         DOVEStatusToString(status), fd_count_total);

	return (int)status;
}

/*
 ******************************************************************************
 * fd_process_del_fd --                                                     *//**
 *
 * \brief This routine removes a file descriptor to the array of polled
 *        file descriptors
 *
 * \param[in] fd	The File Descriptor
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_INVALID_FD File Descriptor doesn't exist in Array
 *
 * \note This routine updates the Global fd_count_total to the number of
 *       FDs that can be polled
 *
 *****************************************************************************/

int fd_process_del_fd(int fd)
{
	dove_status status = DOVE_STATUS_NO_RESOURCES;
	char dummy;
	int fd_index;

	log_info(LogLevel, "Enter");

	pthread_mutex_lock(&base_mutex);
	for (fd_index = 0 ; fd_index < CORE_API_P1_POLL_FD_MAX ; fd_index++)
	{
		if (fd != pfd[fd_index].fd)
		{
			continue;
		}
		// Need to compress the array. Move the final element to
		// the current location
		pfd[fd_index].fd = pfd[fd_count_total-1].fd;
		pfd[fd_index].events = pfd[fd_count_total-1].events;
		poll_callback_array[fd_index].poll_callback_routine =
			poll_callback_array[fd_count_total-1].poll_callback_routine;
		poll_callback_array[fd_index].context =
			poll_callback_array[fd_count_total-1].context;
		status = DOVE_STATUS_OK;
		fd_count_total--;
		// NULL out the previous final value
		pfd[fd_count_total].fd = -1;
		poll_callback_array[fd_count_total].poll_callback_routine = NULL;
		poll_callback_array[fd_count_total].context = NULL;
	}
	if(fd_poll_dummy[1] >= 0)
	{
		/* Wake up poll */
		if(1 != write(fd_poll_dummy[1], &dummy, 1))
		{
			log_debug(LogLevel, "write poll dummy fd error!");
		}
	}
	pthread_mutex_unlock(&base_mutex);

	log_info(LogLevel, "Exit %s, FD Count %d",
	         DOVEStatusToString(status), fd_count_total);

	return (int)status;
}

/*
 ******************************************************************************
 * init_fd_process_p1 --                                                    *//**
 *
 * \brief This Initializes the CORE API
 *
 * \retval DOVE_STATUS_OK Success
 * \retval Other Failure
 *
 *****************************************************************************/

dove_status fd_process_init(void)
{
	dove_status status = DOVE_STATUS_NO_RESOURCES;

	do
	{
		status = poll_array_init();
		if (status != DOVE_STATUS_OK)
		{
			break;
		}
	} while(0);

	return status;
}

/*
 ******************************************************************************
 * fd_process_start --                                                   *//**
 *
 * \brief This starts the CORE API.
 *
 * \retval None
 *
 *****************************************************************************/

void fd_process_start(void)
{
	dove_status status;
	int poll_result;
	int poll_errno;

	log_notice(LogLevel, "Starting CORE API Phase 1");

	// Loop Forever
	while(1)
	{
		if (fd_count_total == 0)
		{
			log_critical(LogLevel,
			             "NO File Descriptors, Exiting...");
			break;
		}
		log_debug(LogLevel,"Polling with %d FDs", fd_count_total);
		poll_result = poll(pfd, fd_count_total, -1);
		poll_errno = errno;
		status = poll_process_result(pfd, fd_count_total, poll_result);

		if ((status != DOVE_STATUS_OK) && (poll_errno != EINTR))
		{
			log_notice(LogLevel,
			           "Process Poll Result returns [%s] Errno [%d], "
			           "Starting Over...",
			           DOVEStatusToString(status),
			           poll_errno);
		}
	}

	log_emergency(LogLevel, "EXIT");

	return;
}

/** @} */
/** @} */

