package org.opendaylight.opendove.odmc;

public interface IfOpenDSATrackedObject {
	boolean isTrackedByDSA();
	
    public Integer getLastChangeVersion();

    public void setLastChangeVersion(Integer lastChangeVersion);

    public Integer getCreateVersion();

    public void setCreateVersion(Integer createVersion);
}
