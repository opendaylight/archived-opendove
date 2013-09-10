package org.opendaylight.opendove.odmc;

import java.util.List;

public interface IfSBDoveDomainCRU {
    public boolean domainExists(String domainUUID);

    public boolean domainExistsByName(String name);

    public OpenDoveDomain getDomain(String domainUUID);

    public OpenDoveDomain getDomainByName(String name);

    public void addDomain(String domainUUID, OpenDoveDomain domain);

    public void addNetworkToDomain(String domainUUID, OpenDoveNetwork network);

    public List<OpenDoveDomain> getDomains();
}
