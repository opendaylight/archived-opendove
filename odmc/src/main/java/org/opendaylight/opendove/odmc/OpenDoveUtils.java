/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.Random;

public class OpenDoveUtils {
    private static Random rng;

    public static void initRNG() {
        rng = new Random();    //TODO: need to seed this better
    }

    public static long getNextLong() {
        return rng.nextLong();
    }

    public static int getNextInt() {
        return rng.nextInt();
    }
}
