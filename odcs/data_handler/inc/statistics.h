/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 * File:   statistics.h
 * Author: Amitabha Biswas
 *
 * Created on Feb 25, 2012, 12:32 PM
 */

#ifndef _DPS_STATISTICS_H_
#define _DPS_STATISTICS_H_

/**
 * \ingroup DPSSTATISTICSServices
 * @{
 */

/* define a variable to set to NOT send the statistics, both to the Dove Controller and Leader */
#define DPS_STATISTICS_NOT_SEND 0

/**
 * \brief The Structure for get load balancing of a Domain
 */
typedef struct dps_object_load_balancing_s{
	/*
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
	/*
	 * \brief Sampling cycle
	 */
	uint32_t sample_interval;
	/*
	 * \brief Endpoint update count in a sampling cycle
	 */
	uint32_t endpoint_update_count;
	/*
	 * \brief Endpoint lookup count in a sampling cycle
	 */
	uint32_t endpoint_lookup_count;
	/*
	 * \brief Policy update count in a sampling cycle
	 */
	uint32_t policy_lookup_count;
	/*
	 * \brief Endpoint number
	 */
	uint32_t endpoints_count;
}dps_object_load_balancing_t;

/**
 * \brief The Structure for get count of general statistics of a
 *        Domain
 */
typedef struct dps_object_general_statistics_count_s{
	/*
	 * \brief Endpoint update count
	 */
	uint32_t endpoint_update_count;
	/*
	 * \brief Endpoint lookup count
	 */
	uint32_t endpoint_lookup_count;
	/*
	 * \brief Policy update count
	 */
	uint32_t policy_lookup_count;
	/*
	 * \brief Multicast lookup count
	 */
	uint32_t multicast_lookup_count;
	/*
	 * \brief Internal gateway lookup count
	 */
	uint32_t internal_gw_lookup_count;
}dps_object_general_statistics_count_t;

/**
 * \brief The Structure for get rate of general statistics of a 
 *        Domain
 */
typedef struct dps_object_general_statistics_rate_s{
	/*
	 * \brief Endpoint update rate
	 */
	uint32_t endpoint_update_rate;
	/*
	 * \brief Endpoint lookup rate
	 */
	uint32_t endpoint_lookup_rate;
	/*
	 * \brief Policy update rate
	 */
	uint32_t policy_lookup_rate;
	/*
	 * \brief Multicast lookup rate
	 */
	uint32_t multicast_lookup_rate;
	/*
	 * \brief Internal gateway lookup rate
	 */
	uint32_t internal_gw_lookup_rate;
}dps_object_general_statistics_rate_t;

/**
 * \brief The Structure for get general statistics of a Domain
 */
typedef struct dps_object_general_statistics_s{
	/*
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
	/*
	 * \brief Misc Counts
	 */
	dps_object_general_statistics_count_t counts;
	/*
	 * \brief Misc Rates
	 */
	dps_object_general_statistics_rate_t rates;

}dps_object_general_statistics_t;

/*
 ******************************************************************************
 * dps_statistics_domain_load_balancing_get                               *//**
 *
 * \brief - Get load balancing for a specified domain
 *
 * \param load_balancing - The Structure of the Message for load balancing
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status dps_statistics_domain_load_balancing_get(dps_object_load_balancing_t *load_balancing);

/*
 ******************************************************************************
 * dps_statistics_domain_general_statistics_get                           *//**
 *
 * \brief - Get general statistics for a specified domain
 *
 * \param general_statistics - The Structure of the Message for general_statistics
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status dps_statistics_domain_general_statistics_get(dps_object_general_statistics_t *general_statistics);

/*
 ******************************************************************************
 * dps_server_rest_init                                                    *//**
 *
 * \brief - Initializes the DPS statistics infrastructure
 *
 * \param pythonpath - Pointer to the Python Path
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status dps_statistics_init(char *pythonpath);

dove_status dps_statistics_start(void);
dove_status dps_statistics_stop(void);

int set_statistics_interval(int interval);
int get_statistics_interval(void);



/** @} */

#endif // _DPS_STATISTICS_H_
