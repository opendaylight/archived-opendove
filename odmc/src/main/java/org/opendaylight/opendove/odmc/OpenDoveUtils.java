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
