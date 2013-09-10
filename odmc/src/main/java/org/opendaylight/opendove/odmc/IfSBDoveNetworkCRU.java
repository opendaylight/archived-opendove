package org.opendaylight.opendove.odmc;

import java.util.List;

public interface IfSBDoveNetworkCRU {
    public boolean networkExists(String networkUUID);

    public boolean networkExistsByVnid(int vnid);

    public OpenDoveNetwork getNetwork(String networkUUID);

    public void addNetwork(String networkUUID, OpenDoveNetwork network);

    public int allocateVNID();

    public List<OpenDoveNetwork> getNetworks();
}
