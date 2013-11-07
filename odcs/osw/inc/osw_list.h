/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Header File:
 *      osw_list.h
 *
 *  Abstract:
 *      Describes the List Manipulation Infrastructure needed by the
 *      DOVE appliance.
 *
 *  Author:
 *      DOVE Development Team
 *
 *  Environment:
 *      Userspace Mode
 *
 *  Revision History:
 *
 */

#ifndef _LIST_H_
#define _LIST_H_

/*
 *****************************************************************************
 * Hash List                                                             *//**
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
 *     (hlist_head)   (hlist_node)        (hlist_node)
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
typedef struct _hlist_head {
	/**
	 * \brief Pointer to the first element
	 */
	struct _hlist_node *first;
} hlist_head;

typedef struct _hlist_node {
	/**
	 * \brief Pointer to the next node
	 */
	struct _hlist_node *next;
	/**
	 * \brief Pointer (double dereference) to the previous node
	 */
	struct _hlist_node **pprevious;
} hlist_node;

/*
 ***********************************************************************
 * hlist_head_init --                                              *//**
 *
 * \ingroup HList
 * \brief Initialize a header pointer to point to an empty hash list.
 *
 * \param[in] _hlist_head    Hash list header to initialize.
 *
 ***********************************************************************
 */

#define hlist_head_init(_hlist_head)    ((_hlist_head)->first = NULL)

/*
 ***********************************************************************
 * hlist_node_init --                                              *//**
 *
 * \ingroup HList
 * \brief Initialize a hash list node pointer.
 *
 * \param[in] node    Hash list node to initialize.
 *
 ***********************************************************************
 */

static inline void hlist_node_init(hlist_node *node)
{
	node->next = NULL;
	node->pprevious = NULL;
}

/*
 ***********************************************************************
 * hlist_head_add --                                               *//**
 *
 * \ingroup HList
 * \brief Add a node to the head of a hash list.
 *
 * \param[in] head    Hash list head.
 * \param[in] node    Hash list node to be added.
 *
 ***********************************************************************
 */

static inline void hlist_head_add(hlist_head *head, hlist_node *node)
{
	hlist_node *first = head->first;
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
 * hlist_node_remove --                                            *//**
 *
 * \ingroup HList
 * \brief Remove a node from a hash list.
 *
 * \param[in] node    Hash list node to be removed.
 *
 ***********************************************************************
 */

static inline void hlist_node_remove(hlist_node *node)
{
	hlist_node *next = node->next;
	hlist_node **pprev = node->pprevious;
	*pprev = next;
	if (next)
	{
		next->pprevious = pprev;
	}
	node->next = (struct _hlist_node *)LIST_POISON1;
	node->pprevious = (struct _hlist_node **)LIST_POISON2;
}

/*
 ***********************************************************************
 * hlist_empty --                                                  *//**
 *
 * \ingroup HList
 * \brief Test if a hash list is empty.
 *
 * \param[in] head    Hash list header
 *
 ***********************************************************************
 */

static inline int hlist_empty(hlist_head *head)
{
    return (head->first == NULL);
}

/*
 ***********************************************************************
 * hlist_entry --                                                  *//**
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

#define hlist_entry(item_ptr, container_type, field_in_container) \
    ((container_type *) ((char *) (item_ptr) - ((long) &((container_type *)0)->field_in_container)))

/*
 ***********************************************************************
 * hlist_first_entry --                                            *//**
 *
 * \ingroup HList
 * \brief Get a pointer to the structure containing first list element.
 *
 * \param[in] head_ptr            The head of the hash list
 * \param[in] container_type      C type of the container
 * \param[in] field_in_container  Name of the structure field in the container 
 *                                that item_ptr is pointing to.
 *
 ***********************************************************************
 */

#define hlist_first_entry(head_ptr, container_type, field_in_container) \
    hlist_entry((head_ptr)->first, container_type, field_in_container)

/*
 ***********************************************************************
 * hlist_for_each_entry_safe --                            *//**
 *
 * \ingroup HList
 * \brief Iterate through a hash list safely. Protects against removal
 * of the current entry.
 *
 * \param[in] type      The TYPE of the element in the list
 * \param[in] tpos      The element itself (of TYPE = type) at each location
 *                      in the list. Basically the current element in each
 *                      iteration
 * \param[in] pos       A variable of type hlist_node that can
 *                      be used to iterate over the list
 * \param[in] pnext     A variable to store the next element in the list.
 *                      Protects against the current element being removed
 *                      from the list
 * \param[in] head      The head of the hash list
 * \param[in] field     The field in struct TYPE that is of type
 *                      hlist_node and represents the linkage
 *                      in the hash list
 *
 ***********************************************************************
 */

#define hlist_for_each_entry_safe(type, tpos, pos, pnext, head, field) \
    for (pos = (head)->first;                                          \
         pos && ({ pnext = pos->next; 1; }) &&                         \
             ({ tpos = hlist_entry(pos, type, field); 1;});            \
         pos = pnext)

/*
 ***********************************************************************
 * hlist_for_each_entry     --                             *//**
 *
 * \ingroup HList
 * \brief Iterate through a hash list. This does not protect again the
 * removal of the current element.
 *
 * \param[in] type      The TYPE of the element in the list
 * \param[in] tpos      The element itself (of TYPE = type) at each location
 *                      in the list. Basically the current element in each
 *                      iteration
 * \param[in] pos       A variable of type hlist_node that can
 *                      be used to iterate over the list
 * \param[in] head      The head of the hash list
 * \param[in] field     The field in struct TYPE that is of type
 *                      hlist_node and represents the linkage
 *                      in the hash list
 *
 ***********************************************************************
 */

#define hlist_for_each_entry(type, tpos, pos, head, field)          \
    for (pos = (head)->first;                                       \
         pos && ({ tpos = hlist_entry(pos, type, field); 1;});      \
         pos = pos->next)

/** @} */

#endif // _LIST_H_
