package org.opendaylight.opendove.odmc;

public interface IfSBDoveNetworkCRU {
    public boolean networkExists(String networkUUID);

    public OpenDoveNetwork getNetwork(String networkUUID);

    public void addNetwork(String networkUUID, OpenDoveNetwork network);
}
