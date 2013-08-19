/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.controller.odmc;

import java.util.List;

/**
 * This interface defines the methods for CRUD of NB Router objects
 *
 */

public interface IfNBRouterCRUD {
    /**
     * Applications call this interface method to determine if a particular
     * Router object exists
     *
     * @param uuid
     *            UUID of the Router object
     * @return boolean
     */

    public boolean routerExists(String uuid);

    /**
     * Applications call this interface method to return if a particular
     * Router object exists
     *
     * @param uuid
     *            UUID of the Router object
     * @return {@link org.opendaylight.controller.opendove.OpenStackRouters}
     *          OpenStack Router class
     */

    public OpenStackRouters getRouter(String uuid);

    /**
     * Applications call this interface method to return all Router objects
     *
     * @return List of OpenStackRouters objects
     */

    public List<OpenStackRouters> getAllRouters();

    /**
     * Applications call this interface method to add a Router object to the
     * concurrent map
     *
     * @param input
     *            OpenStackRouter object
     * @return boolean on whether the object was added or not
     */

    public boolean addRouter(OpenStackRouters input);

    /**
     * Applications call this interface method to remove a Router object to the
     * concurrent map
     *
     * @param uuid
     *            identifier for the Router object
     * @return boolean on whether the object was removed or not
     */

    public boolean removeRouter(String uuid);

    /**
     * Applications call this interface method to edit a Router object
     *
     * @param uuid
     *            identifier of the Router object
     * @param delta
     *            OpenStackRouter object containing changes to apply
     * @return boolean on whether the object was updated or not
     */

    public boolean updateRouter(String uuid, OpenStackRouters delta);

    /**
     * Applications call this interface method to check if a router is in use
     *
     * @param uuid
     *            identifier of the Router object
     * @return boolean on whether the router is in use or not
     */

    public boolean routerInUse(String routerUUID);
}
