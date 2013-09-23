package org.opendaylight.opendove.odmc;

public class OpenDoveNetworkSubnetAssociation extends OpenDoveObject implements IfOpenDCSTrackedObject {
	
	String uuid;
	
	int openDoveNetworkVnid;
	
	String openDoveNetworkSubnetUuid;
	
    public OpenDoveNetworkSubnetAssociation() {
    	uuid = java.util.UUID.randomUUID().toString();
    	tombstoneFlag = false;
    }

	public int getOpenDoveNetworkVnid() {
		return openDoveNetworkVnid;
	}

	public void setOpenDoveNetworkVnid(int openDoveNetworkVnid) {
		this.openDoveNetworkVnid = openDoveNetworkVnid;
	}

	public String getOpenDoveNetworkSubnetUuid() {
		return openDoveNetworkSubnetUuid;
	}

	public void setOpenDoveNetworkSubnetUuid(String openDoveNetworkSubnetUuid) {
		this.openDoveNetworkSubnetUuid = openDoveNetworkSubnetUuid;
	}

	public boolean isTrackedByDCS() {
		return true;
	}

	public String getSBDcsUri() {
		return "/controller/sb/v2/opendove/odmc/networks/"+openDoveNetworkVnid+"/subnets/"+openDoveNetworkSubnetUuid;
	}

	@Override
	public String getUUID() {
		return uuid;
	}
}
