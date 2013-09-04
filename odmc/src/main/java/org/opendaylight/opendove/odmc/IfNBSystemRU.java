package org.opendaylight.opendove.odmc;

public interface IfNBSystemRU {

    OpenDoveNeutronControlBlock getSystemBlock();

    boolean updateControlBlock(OpenDoveNeutronControlBlock input);

}
