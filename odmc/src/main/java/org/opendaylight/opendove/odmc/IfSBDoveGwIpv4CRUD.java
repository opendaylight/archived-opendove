/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.List;

public interface IfSBDoveGwIpv4CRUD {

    public boolean gwIpv4Exists(String ipv4UUID);

    public void addGwIpv4(String ipv4UUID, OpenDoveGwIpv4 ipv4);

    public OpenDoveGwIpv4 getGwIpv4(String ipv4UUID);

    public List<OpenDoveGwIpv4> getGwIpv4Pool();

    public void removeGwIpv4(String ipv4UUID);
}