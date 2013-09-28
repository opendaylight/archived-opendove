/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.Collection;
import java.util.HashMap;
import java.util.concurrent.ConcurrentMap;

public class OpenDoveConcurrentBackedMap implements IfOpenDoveMap
{
    private ConcurrentMap<Integer,OpenDoveObject> int2OpenDoveObjectMap;
    private HashMap<String, OpenDoveObject> str2OpenDoveObjectMap;
    private int objectIntegerCounter;
    private Object myInterestedClass;

    public OpenDoveConcurrentBackedMap(ConcurrentMap<Integer, OpenDoveObject> openDoveObjMap, Object interestedClass){
        int2OpenDoveObjectMap = openDoveObjMap;
        str2OpenDoveObjectMap = new HashMap<String, OpenDoveObject>();
        myInterestedClass = interestedClass;
        // find highest value in int2OpenDoveObjectMap
        objectIntegerCounter = 1;
        updateConcurrentMap();
    }

    /*
     * UpdateConcurrentMap is a private method that starts at counter and checks the
     * ConcurrentMap for key values until it reaches the end of any updates.  If a key
     *  exists in the ConcurrentMap, an entry is added to the HashMap for that OpenDoveObject
     *  if one is not already there.
     *
     * The string that should be used as the key in the HashMap is uuid - it's already present
     * in OpenDoveDomain and OpenDoveNetwork, but needs to be moved up to OpenDoveObject
     */
    public void updateConcurrentMap(){
        while (int2OpenDoveObjectMap.containsKey(objectIntegerCounter)) {
            OpenDoveObject object = int2OpenDoveObjectMap.get(objectIntegerCounter);
            String key = object.getUUID();
            if (object.getClass().isInstance(myInterestedClass) && !str2OpenDoveObjectMap.containsKey(key))
                str2OpenDoveObjectMap.put(key, object);
            objectIntegerCounter++;
        }
    }

    /* when containsKey(String) is called, first call UpdateConcurrentMap and then check the
     * HashMap for the presence of the given argument
     * (non-Javadoc)
     * @see org.opendaylight.opendove.odmc.IfOpenDoveMap#hasKey(java.lang.String)
     */
    public boolean containsKey(String s){
        this.updateConcurrentMap();
        return this.str2OpenDoveObjectMap.containsKey(s);
    }

    /*
     * when get(String) is called, first call UpdateConcurrentMap, and then return the
     * HashMap entry for the given argument
     * (non-Javadoc)
     * @see org.opendaylight.opendove.odmc.IfOpenDoveMap#get(java.lang.String)
     */
    public OpenDoveObject get(String s){
        this.updateConcurrentMap();
        return this.str2OpenDoveObjectMap.get(s);
    }

    /*
     * when putIfAbsent is called, first call UpdateConcurrentMap, check and see if the
     * key string exists in the HashMap. If it does, return the object that it points to.
     * If not, then add the OpenDoveObject as the next entry in the ConcurrentMap and
     * increment the integer counter
     * (non-Javadoc)
     * @see org.opendaylight.opendove.odmc.IfOpenDoveMap#putIfAbsent(java.lang.String, org.opendaylight.opendove.odmc.OpenDoveObject)
     */
    public OpenDoveObject putIfAbsent(String s, OpenDoveObject openDoveObj){
        OpenDoveObject ans;
        this.updateConcurrentMap();
        if(str2OpenDoveObjectMap.containsKey(s))
            ans = str2OpenDoveObjectMap.get(s);
        else {
            str2OpenDoveObjectMap.put(s,openDoveObj);
            int2OpenDoveObjectMap.putIfAbsent(objectIntegerCounter, openDoveObj);
            ans = openDoveObj;
            openDoveObj.setCreateVersion(objectIntegerCounter);
            openDoveObj.setLastChangeVersion(objectIntegerCounter);
            objectIntegerCounter++;
        }
        return ans;
    }

    /*
     * when update is called, first call UpdateConcurrentMap and then add the OpenDoveObject
     * as the next entry in the ConcurrentMap (by incrementing the integer counter)
     * (non-Javadoc)
     * @see org.opendaylight.opendove.odmc.IfOpenDoveMap#update(java.lang.String, org.opendaylight.opendove.odmc.OpenDoveObject)
     */
    public OpenDoveObject update(String s, OpenDoveObject openDoveObj) {
        this.updateConcurrentMap();
        int2OpenDoveObjectMap.putIfAbsent(objectIntegerCounter, openDoveObj);
        openDoveObj.setLastChangeVersion(objectIntegerCounter);
        objectIntegerCounter++;
        return openDoveObj;
    }

    /*
     * when called, first call updateConcurrentMap and then return the values in the hash map
     * (non-Javadoc)
     * @see org.opendaylight.opendove.odmc.IfOpenDoveMap#values()
     */
    public Collection<OpenDoveObject> values() {
        this.updateConcurrentMap();
        return str2OpenDoveObjectMap.values();
    }

    // needs some more thought - this just removes from the map and doesn't update the concurrent map (which,
    // while necessary, will break the assumption that the concurrentmap is dense).
    public OpenDoveObject remove(String key) {
        this.updateConcurrentMap();
        return str2OpenDoveObjectMap.remove(key);
    }
}





