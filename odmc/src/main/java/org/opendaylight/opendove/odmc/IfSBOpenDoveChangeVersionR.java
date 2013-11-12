/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

public interface IfSBOpenDoveChangeVersionR {
    public int versionExists(int version);

    public OpenDoveChange getNextOdcsChange(int version);

    public OpenDoveChange getNextOdgwChange(int version);
}
