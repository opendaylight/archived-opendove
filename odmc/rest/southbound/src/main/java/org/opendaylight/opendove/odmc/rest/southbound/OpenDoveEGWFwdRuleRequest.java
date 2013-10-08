/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.southbound;

import java.util.List;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

import org.opendaylight.opendove.odmc.OpenDoveEGWFwdRule;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveEGWFwdRuleRequest {

    @XmlElement(name="egw_fwd_rule")
    OpenDoveEGWFwdRule singletonRule;

    @XmlElement(name="egw_fwd_rules")
    List<OpenDoveEGWFwdRule> bulkRules;

    OpenDoveEGWFwdRuleRequest() {
    }

    OpenDoveEGWFwdRuleRequest(List<OpenDoveEGWFwdRule> bulk) {
        bulkRules = bulk;
        singletonRule = null;
    }

    OpenDoveEGWFwdRuleRequest(OpenDoveEGWFwdRule single) {
        singletonRule = single;
    }

    public OpenDoveEGWFwdRule getSingleton() {
        return singletonRule;
    }

    public boolean isSingleton() {
        return (singletonRule != null);
    }

    public List<OpenDoveEGWFwdRule> getBulk() {
        return bulkRules;
    }
}
