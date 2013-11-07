/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *
 *  Header File:
 *      dps_list.h
 *
 *  Abstract:
 *      Describes the List Manipulation Infrastructure needed by the
 *      DOVE client.
 *
 *  Author:
 *      Sushma Anantharam
 *
 *  Environment:
 *      User World
 *
 *  Revision History:
 *
 */

#ifndef _DPS_LIST_
#define _DPS_LIST_


/*
 *****************************************************************************
 * Hash List                                                            *//**
 *
 * \addtogroup utilities
 * @{
 * \defgroup HList Hash List Implementation
 *
 * The head element of a hash list only contains a single next pointer i.e.
 * there is no previous pointer. It's useful in cases where two pointers are
 * wasteful. The actual elements linked to the list have both next and previous
 * pointers.
 *
 * \code
 *          header               First element                Second Element
 *      (dps_hlist_head)        (dps_hlist_node)             (dps_hlist_node)
 *      ---------------      ----------------------      ----------------------
 * ..-> |    first    | ---> |       next         | ---> |        next        |
 *      --------------- ^    |                    | ^    |                    |
 *                      |    |                    | |    |                    |
 *                      |--- |     pprevious      | |--- |     pprevious      |
 *                           |--------------------|      |--------------------|
 *                           |      rest of       |      |       rest of      |
 *                           |     structure      |      |      structure     |
 *                           |                    |      |                    |
 *                           |        ...         |      |      ...           |
 *                           ----------------------      ----------------------
 * \endcode
 *
 */

/**
 * \brief Invalid List Links
 */
#define LIST_POISON1    ((void *) 0xbad11111)
#define LIST_POISON2    ((void *) 0xbad22222)

/**
 * \brief Description of Hash List Head
 */
typedef struct _dps_hlist_head {
	/**
	 * \brief Pointer to the first element
	 */
	struct _dps_hlist_node *first;
} dps_hlist_head;

typedef struct _dps_hlist_node {
	/**
	 * \brief Pointer to the next node
	 */
	struct _dps_hlist_node *next;
	/**
	 * \brief Pointer (double dereference) to the previous node
	 */
	struct _dps_hlist_node **pprevious;
} dps_hlist_node;

/*
 ***********************************************************************
 * dps_hlist_head_init --                                          *//**
 *
 * \ingroup HList
 * \brief Initialize a header pointer to point to an empty hash list.
 *
 * \param[in] _hlist_head    Hash list header to initialize.
 *
 ***********************************************************************
 */

#define dps_hlist_head_init(_hlist_head)    ((_hlist_head)->first = NULL)

/*
 ***********************************************************************
 * dps_hlist_node_init --                                          *//**
 *
 * \ingroup HList
 * \brief Initialize a hash list node pointer.
 *
 * \param[in] node    Hash list node to initialize.
 *
 ***********************************************************************
 */

static inline void dps_hlist_node_init(dps_hlist_node *node)
{
	node->next = NULL;
	node->pprevious = NULL;
}

/*
 ***********************************************************************
 * dps_hlist_head_add --                                           *//**
 *
 * \ingroup HList
 * \brief Add a node to the head of a hash list.
 *
 * \param[in] head    Hash list head.
 * \param[in] node    Hash list node to be added.
 *
 ***********************************************************************
 */

static inline void dps_hlist_head_add(dps_hlist_head *head,
                                      dps_hlist_node *node)
{
	dps_hlist_node *first = head->first;
	node->next = first;
	if (first)
	{
		first->pprevious = &node->next;
	}
	head->first = node;
	node->pprevious = &head->first;
}

/*
 ***********************************************************************
 * dps_hlist_node_remove --                                        *//**
 *
 * \ingroup HList
 * \brief Remove a node from a hash list.
 *
 * \param[in] node    Hash list node to be removed.
 *
 ***********************************************************************
 */

static inline void dps_hlist_node_remove(dps_hlist_node *node)
{
	dps_hlist_node *next = node->next;
	dps_hlist_node **pprev = node->pprevious;
	*pprev = next;
	if (next)
	{
		next->pprevious = pprev;
	}
	node->next = (dps_hlist_node *)LIST_POISON1;
	node->pprevious = (dps_hlist_node **)LIST_POISON2;
}


/*
 ***********************************************************************
 * dps_hlist_entry --                                              *//**
 *
 * \ingroup HList
 * \brief Get a pointer to the structure containing a given list element.
 *
 * \param[in] item_ptr            List element that is contained by another structure
 * \param[in] container_type      C type of the container
 * \param[in] field_in_container  Name of the structure field in the container 
 *                                that item_ptr is pointing to.
 *
 ***********************************************************************
 */

#define dps_hlist_entry(item_ptr, container_type, field_in_container) \
    ((container_type *) ((char *) (item_ptr) - ((long) &((container_type *)0)->field_in_container)))

/*
 ***********************************************************************
 * dps_hlist_for_each_entry_safe --                                *//**
 *
 * \ingroup HList
 * \brief Iterate through a hash list safely. Protects against removal
 * of the current entry.
 *
 * \param[in] type      The TYPE of the element in the list
 * \param[in] tpos      The element itself (of TYPE = type) at each location
 *                      in the list. Basically the current element in each
 *                      iteration
 * \param[in] pos       A variable of type dps_hlist_node that can
 *                      be used to iterate over the list
 * \param[in] pnext     A variable to store the next element in the list.
 *                      Protects against the current element being removed
 *                      from the list
 * \param[in] head      The head of the hash list
 * \param[in] field     The field in struct TYPE that is of type
 *                      dps_hlist_node and represents the linkage
 *                      in the hash list
 *
 ***********************************************************************
 */

#define dps_hlist_for_each_entry_safe(type, tpos, pos, pnext, head, field) \
    for (pos = (head)->first;                                              \
         pos && ({ pnext = pos->next; 1; }) &&                             \
             ({ tpos = dps_hlist_entry(pos, type, field); 1;});             \
         pos = pnext)

/*
 ***********************************************************************
 * dps_hlist_for_each_entry     --                                 *//**
 *
 * \ingroup HList
 * \brief Iterate through a hash list. This does not protect again the
 * removal of the current element.
 *
 * \param[in] type      The TYPE of the element in the list
 * \param[in] tpos      The element itself (of TYPE = type) at each location
 *                      in the list. Basically the current element in each
 *                      iteration
 * \param[in] pos       A variable of type dps_hlist_node that can
 *                      be used to iterate over the list
 * \param[in] head      The head of the hash list
 * \param[in] field     The field in struct TYPE that is of type
 *                      dps_hlist_node and represents the linkage
 *                      in the hash list
 *
 ***********************************************************************
 */

#define dps_hlist_for_each_entry(type, tpos, pos, head, field)          \
    for (pos = (head)->first;                                           \
         pos && ({ tpos = dps_hlist_entry(pos, type, field); 1;});       \
         pos = pos->next)

/** @} */

#endif // _DPS_LIST_
