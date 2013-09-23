/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.Collection;

public interface IfOpenDoveMap {

    /**
     *
     * @param input
     * @return true if input is accepted.
     */
    /*    boolean isAcceptable(I input); */

    /*
     * @param s
     *         key for the object
     * @param openDoveObj
     *         the object to be stored
     * @return if the key does not exist, the object stored.
     *         if the key exists, the object stored at that key
     *
     */
    OpenDoveObject putIfAbsent(String s, OpenDoveObject openDoveObj);

    /*
     * @param s
     *        key for the object to be updated
     * @param openDoveObj
     *        the new version of the object to be stored
     * @return the object stored.
     */
    OpenDoveObject update(String s, OpenDoveObject openDoveObj);

    /*
     * @param s
     *        key to test
     * @return true if the key exists, false if not
     */
    boolean containsKey(String s);

    /*
     * @param s
     *        key to fetch
     * @return object stored at that key or null if the key does not exist
     */
    OpenDoveObject get(String s);
    
    /*
     * @return objects in the map
     */
	public Collection<OpenDoveObject> values();
	
	/*
	 * @param s 
	 *        key to remove
	 * @return true if object was removed from the map, false if not
	 * 
	 */
	public OpenDoveObject remove(String key);
}
