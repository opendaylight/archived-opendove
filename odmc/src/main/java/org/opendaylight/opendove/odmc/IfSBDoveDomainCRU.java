package org.opendaylight.opendove.odmc;

public interface IfSBDoveDomainCRU {
    public boolean domainExists(String domainUUID);

    public OpenDoveDomain getDomain(String domainUUID);

    public void addDomain(String domainUUID, OpenDoveDomain domain);

    public void addNetworkToDomain(String domainUUID, OpenDoveNetwork network);
}
